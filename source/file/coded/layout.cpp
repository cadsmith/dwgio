/***********************************************************************************************************************
 *  @file      source/file/coded/layout.cpp
 *  @brief     An R2007 file cut into its sections.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/coded/layout.hpp"

#include "bits/lz21.hpp"
#include "file/paged/names.hpp"
#include "octets.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <utility>

namespace DwgIO::File::Coded
{
namespace
{
constexpr std::size_t Codeword = 255;
constexpr std::size_t SystemData = 239;
constexpr std::size_t PageData = 251;
constexpr std::size_t Align = 8;
constexpr std::size_t HeaderBlocks = 3;
constexpr std::size_t Decompressed = 0x110;
constexpr std::uint64_t Interleaved = 4;
/**
 *  The most bytes a page may hold, stored or plain.
 */
constexpr std::uint64_t MostPage = 0x1000000;

auto Long(std::span<const std::byte> bytes, std::size_t at) -> std::uint64_t
{
  std::uint64_t value = 0;
  for (std::size_t one = 0; one < sizeof(std::uint64_t); ++one)
  {
    value |= std::to_integer<std::uint64_t>(bytes[at + one]) << (8U * one);
  }
  return value;
}

auto AlignSize(std::uint64_t size) -> std::uint64_t
{
  return (size + Align - 1) & ~std::uint64_t{Align - 1};
}

/**
 *  How codewords lie: how many, how many data bytes each, and whether interleaved.
 */
struct Weave
{
  std::size_t Blocks{};
  std::size_t Data{};
  bool Interleaved{};
};

/**
 *  The data bytes of Reed-Solomon codewords: interleaved, block b's byte i at blocks * i + b; else each block's data
 *  bytes one after another, the parity after all of them.
 */
auto Unweave(std::span<const std::byte> coded, const Weave &weave) -> std::vector<std::byte>
{
  if (!weave.Interleaved)
  {
    return {coded.begin(), coded.begin() + static_cast<std::ptrdiff_t>(weave.Blocks * weave.Data)};
  }
  std::vector<std::byte> out(weave.Blocks * weave.Data);
  for (std::size_t block = 0; block < weave.Blocks; ++block)
  {
    for (std::size_t at = 0; at < weave.Data; ++at)
    {
      out[(block * weave.Data) + at] = coded[(weave.Blocks * at) + block];
    }
  }
  return out;
}

/**
 *  The first `stored` bytes of the data: as they are when no smaller than `size`, else decompressed into it.
 */
auto Expand(std::span<const std::byte> data, std::uint64_t stored, std::uint64_t size)
    -> std::optional<std::vector<std::byte>>
{
  const std::span<const std::byte> first = data.first(static_cast<std::size_t>(stored));
  if (stored >= size)
  {
    return std::vector<std::byte>{first.begin(), first.begin() + static_cast<std::ptrdiff_t>(size)};
  }
  return Bits::Lz21::Decompress(first, static_cast<std::size_t>(size));
}

/**
 *  The fields of the file header the reader needs.
 */
struct Header
{
  std::uint64_t PagesMapCorrection{};
  std::uint64_t PagesMapOffset{};
  std::uint64_t PagesMapCompressed{};
  std::uint64_t PagesMapSize{};
  std::uint64_t SectionsMapCompressed{};
  std::uint64_t SectionsMapId{};
  std::uint64_t SectionsMapSize{};
  std::uint64_t SectionsMapCorrection{};
};

auto MakeUnreadable(std::string what) -> std::unexpected<Failure>
{
  return std::unexpected(Failure{.Code = FailureCode::Damaged, .Offset = MetaSize, .What = std::move(what)});
}

auto ReadHeader(std::span<const std::byte> file) -> std::expected<Header, Failure>
{
  constexpr std::size_t LengthAt = 0x18;
  constexpr std::size_t DataAt = 0x20;
  const std::vector<std::byte> plain = Unweave(file.subspan(MetaSize, HeaderBlocks * Codeword),
                                               Weave{.Blocks = HeaderBlocks, .Data = SystemData, .Interleaved = true});
  const auto length = static_cast<std::int32_t>(static_cast<std::uint32_t>(Long(plain, LengthAt)));
  const std::size_t stored =
      length < 0 ? static_cast<std::size_t>(-static_cast<std::int64_t>(length)) : static_cast<std::size_t>(length);
  if (stored == 0 || stored > plain.size() - DataAt)
  {
    return MakeUnreadable("coded file header unreadable");
  }
  const std::span<const std::byte> data = std::span{plain}.subspan(DataAt, stored);
  std::optional<std::vector<std::byte>> body = length < 0
                                                   ? std::optional{std::vector<std::byte>{data.begin(), data.end()}}
                                                   : Bits::Lz21::Decompress(data, Decompressed);
  if (!body || body->size() < Decompressed)
  {
    return MakeUnreadable("coded file header unreadable");
  }
  const std::span<const std::byte> fields = *body;
  return Header{.PagesMapCorrection = Long(fields, 0x18),
                .PagesMapOffset = Long(fields, 0x38),
                .PagesMapCompressed = Long(fields, 0x50),
                .PagesMapSize = Long(fields, 0x58),
                .SectionsMapCompressed = Long(fields, 0xB0),
                .SectionsMapId = Long(fields, 0xC0),
                .SectionsMapSize = Long(fields, 0xC8),
                .SectionsMapCorrection = Long(fields, 0xD8)};
}

/**
 *  Where a system page lies, its data's sizes stored and plain, and how often the data repeats.
 */
struct Stored
{
  std::uint64_t Address{};
  std::uint64_t Compressed{};
  std::uint64_t Size{};
  std::uint64_t Correction{};
};

/**
 *  A system page of chapter 5.3: its data repeated `correction` times, coded in interleaved blocks of 239 bytes, the
 *  first copy decompressed when smaller than the size.
 */
auto SystemPage(std::span<const std::byte> file, const Stored &stored) -> std::optional<std::vector<std::byte>>
{
  if (stored.Compressed == 0 || stored.Compressed > MostPage || stored.Size > MostPage || stored.Correction == 0 ||
      stored.Correction > MostPage)
  {
    return std::nullopt;
  }
  const std::uint64_t blocks = ((AlignSize(stored.Compressed) * stored.Correction) + SystemData - 1) / SystemData;
  if (blocks * Codeword > file.size() - std::min<std::uint64_t>(stored.Address, file.size()))
  {
    return std::nullopt;
  }
  const std::vector<std::byte> plain =
      Unweave(file.subspan(stored.Address, blocks * Codeword),
              Weave{.Blocks = static_cast<std::size_t>(blocks), .Data = SystemData, .Interleaved = true});
  return Expand(plain, stored.Compressed, stored.Size);
}

/**
 *  A page of the page map: its size and where it starts in the stream after the file header.
 */
struct Placed
{
  std::uint64_t Size{};
  std::uint64_t Offset{};
};

auto PageMap(std::span<const std::byte> body) -> std::map<std::uint64_t, Placed>
{
  constexpr std::size_t Pair = 16;
  std::map<std::uint64_t, Placed> pages;
  std::uint64_t offset = 0;
  for (std::size_t at = 0; at + Pair <= body.size(); at += Pair)
  {
    const std::uint64_t size = Long(body, at);
    // A gap's number is negative; its magnitude taken unsigned holds the most negative one too.
    const std::uint64_t number = Long(body, at + Align);
    const std::uint64_t magnitude = static_cast<std::int64_t>(number) < 0 ? 0 - number : number;
    pages[magnitude] = Placed{.Size = size, .Offset = offset};
    offset += size;
  }
  return pages;
}

/**
 *  A data page of a section, as the section map describes it.
 */
struct Page
{
  std::uint64_t Start{};
  std::uint64_t Id{};
  std::uint64_t Size{};
  std::uint64_t Compressed{};
  std::uint64_t Checksum{};
};

struct Entry
{
  Described Description;
  std::vector<Page> Pages;
};

auto SectionMap(std::span<const std::byte> body) -> std::optional<std::vector<Entry>>
{
  constexpr std::size_t Fixed = 0x40;
  constexpr std::size_t PageBytes = 56;
  std::vector<Entry> entries;
  std::size_t at = 0;
  while (at + Fixed <= body.size())
  {
    Entry entry;
    Described &one = entry.Description;
    one.Size = Long(body, at);
    one.PageSize = Long(body, at + 0x08);
    one.Encryption = Long(body, at + 0x10);
    one.Hash = Long(body, at + 0x18);
    const std::uint64_t name = Long(body, at + 0x20);
    one.Unknown1 = Long(body, at + 0x28);
    one.Encoding = Long(body, at + 0x30);
    const std::uint64_t pages = Long(body, at + 0x38);
    at += Fixed;
    if (name > body.size() - at || pages > (body.size() - at - name) / PageBytes)
    {
      return std::nullopt;
    }
    for (std::size_t unit = 0; unit + 1 < name; unit += 2)
    {
      const auto code =
          std::to_integer<unsigned>(body[at + unit]) | (std::to_integer<unsigned>(body[at + unit + 1]) << 8U);
      if (code != 0)
      {
        one.Name.push_back(static_cast<char>(code)); // AutoCAD's section names are ASCII.
      }
    }
    at += name;
    for (std::uint64_t page = 0; page < pages; ++page, at += PageBytes)
    {
      entry.Pages.push_back(Page{.Start = Long(body, at),
                                 .Id = Long(body, at + 0x10),
                                 .Size = Long(body, at + 0x18),
                                 .Compressed = Long(body, at + 0x20),
                                 .Checksum = Long(body, at + 0x28)});
    }
    entries.push_back(std::move(entry));
  }
  return entries;
}

/**
 *  Reads the sections' data pages from the file the page map places them in.
 */
class PageReader
{
public:
  PageReader(std::span<const std::byte> file, const std::map<std::uint64_t, Placed> &placed, std::vector<Issue> &issues)
      : m_File{file}, m_Placed{&placed}, m_Issues{&issues}
  {
  }

  /**
   *  A section's bytes from its pages, the zero pages the file leaves out put back; a page the page map lacks or that
   *  does not decode fails, a checksum that differs is noted.
   */
  [[nodiscard]] auto Section(const Entry &entry) const -> std::expected<std::vector<std::byte>, Failure>
  {
    const Described &one = entry.Description;
    if (const std::optional<Failure> unfit = Unfit(entry))
    {
      return std::unexpected(*unfit);
    }
    std::vector<std::byte> bytes;
    for (const Page &page : entry.Pages)
    {
      const auto found = m_Placed->find(page.Id);
      if (found == m_Placed->end())
      {
        return std::unexpected(Failure{.Code = FailureCode::Damaged,
                                       .What = std::format("section {} page {} missing", one.Name, page.Id)});
      }
      std::optional<std::vector<std::byte>> data = DataPage(found->second, page, one.Encoding == Interleaved);
      if (!data || page.Start > one.Size)
      {
        return std::unexpected(Failure{.Code = FailureCode::Damaged,
                                       .What = std::format("section {} page {} unreadable", one.Name, page.Id)});
      }
      if (Bits::Lz21::Checksum(0, *data) != page.Checksum)
      {
        m_Issues->push_back(Issue{.Code = IssueCode::Checksum,
                                  .Offset = static_cast<std::size_t>(Pages + found->second.Offset),
                                  .What = std::format("section {} page {} checksum mismatch", one.Name, page.Id)});
      }
      if (page.Start > bytes.size())
      {
        bytes.resize(static_cast<std::size_t>(page.Start), std::byte{0}); // A page of zeros the file leaves out.
      }
      bytes.insert(bytes.end(), data->begin(), data->end());
      bytes.resize(std::min<std::size_t>(bytes.size(), static_cast<std::size_t>(one.Size)));
    }
    bytes.resize(static_cast<std::size_t>(one.Size), std::byte{0}); // The pages of zeros the file leaves out last.
    return bytes;
  }

private:
  /**
   *  The failure of a section larger than its pages can hold or naming one page twice; none when it fits. A hostile
   *  figure is refused before any buffer grows to it.
   */
  [[nodiscard]] static auto Unfit(const Entry &entry) -> std::optional<Failure>
  {
    const Described &one = entry.Description;
    if (one.Size > entry.Pages.size() * MostPage)
    {
      return Failure{.Code = FailureCode::Damaged,
                     .What = std::format("section {} size {} beyond its pages", one.Name, one.Size)};
    }
    std::set<std::uint64_t> seen;
    for (const Page &page : entry.Pages)
    {
      if (!seen.insert(page.Id).second)
      {
        return Failure{.Code = FailureCode::Damaged,
                       .What = std::format("section {} page {} repeated", one.Name, page.Id)};
      }
    }
    return std::nullopt;
  }

  /**
   *  One data page decoded: its data in blocks of 251 bytes, interleaved or not, decompressed when smaller.
   */
  [[nodiscard]] auto DataPage(const Placed &placed, const Page &page, bool interleaved) const
      -> std::optional<std::vector<std::byte>>
  {
    if (page.Compressed == 0 || page.Compressed > MostPage || page.Size > MostPage)
    {
      return std::nullopt;
    }
    const std::uint64_t blocks = (AlignSize(page.Compressed) + PageData - 1) / PageData;
    const std::uint64_t address = Pages + placed.Offset;
    if (blocks * Codeword > m_File.size() - std::min<std::uint64_t>(address, m_File.size()))
    {
      return std::nullopt;
    }
    const std::vector<std::byte> plain =
        Unweave(m_File.subspan(address, blocks * Codeword),
                Weave{.Blocks = static_cast<std::size_t>(blocks), .Data = PageData, .Interleaved = interleaved});
    return Expand(plain, page.Compressed, page.Size);
  }

  std::span<const std::byte> m_File;
  const std::map<std::uint64_t, Placed> *m_Placed;
  std::vector<Issue> *m_Issues;
};
} // namespace

auto ReadMeta(std::span<const std::byte> file) -> std::expected<Meta, Failure>
{
  constexpr std::size_t SecurityAt = 0x18;
  if (file.size() < Pages)
  {
    return std::unexpected(Failure{.Code = FailureCode::Damaged, .What = "coded header unreadable"});
  }
  Meta meta;
  std::ranges::copy(file.first(MetaSize), meta.Bytes.begin());
  const std::optional<Version> release = ReleaseOf(TextOf(file.first(6)));
  if (release != Version::R2007)
  {
    return std::unexpected(Failure{.Code = FailureCode::Damaged, .What = "coded header unreadable"});
  }
  if (static_cast<std::uint32_t>(Long(file, SecurityAt)) != 0)
  {
    return std::unexpected(
        Failure{.Code = FailureCode::Encrypted, .Offset = SecurityAt, .What = "encrypted drawing not read"});
  }
  meta.Release = *release;
  meta.Maintenance = std::to_integer<std::uint8_t>(file[0x0B]);
  meta.Flag = std::to_integer<std::uint8_t>(file[0x0C]);
  meta.Image = static_cast<std::uint32_t>(Long(file, 0x0D));
  meta.DwgVersion = std::to_integer<std::uint8_t>(file[0x11]);
  meta.MaintRelease = std::to_integer<std::uint8_t>(file[0x12]);
  meta.Codepage =
      static_cast<std::uint16_t>(std::to_integer<unsigned>(file[0x13]) | (std::to_integer<unsigned>(file[0x14]) << 8U));
  return meta;
}

auto ReadLayout(std::span<const std::byte> file, std::vector<Issue> &issues) -> std::expected<Layout, Failure>
{
  std::expected<Meta, Failure> meta = ReadMeta(file);
  if (!meta)
  {
    return std::unexpected(meta.error());
  }
  const std::expected<Header, Failure> header = ReadHeader(file);
  if (!header)
  {
    return std::unexpected(header.error());
  }
  const std::optional<std::vector<std::byte>> pageMap =
      SystemPage(file, Stored{.Address = Pages + header->PagesMapOffset,
                              .Compressed = header->PagesMapCompressed,
                              .Size = header->PagesMapSize,
                              .Correction = header->PagesMapCorrection});
  if (!pageMap)
  {
    return MakeUnreadable("page map unreadable");
  }
  const std::map<std::uint64_t, Placed> placed = PageMap(*pageMap);
  const auto sections = placed.find(header->SectionsMapId);
  const std::optional<std::vector<std::byte>> sectionMap =
      sections == placed.end() ? std::nullopt
                               : SystemPage(file, Stored{.Address = Pages + sections->second.Offset,
                                                         .Compressed = header->SectionsMapCompressed,
                                                         .Size = header->SectionsMapSize,
                                                         .Correction = header->SectionsMapCorrection});
  const std::optional<std::vector<Entry>> entries = sectionMap ? SectionMap(*sectionMap) : std::nullopt;
  if (!entries)
  {
    return MakeUnreadable("section map unreadable");
  }
  Layout layout{.Meta = *meta, .Sections = {}, .Descriptions = {}};
  const PageReader reading{file, placed, issues};
  for (const Entry &entry : *entries)
  {
    if (entry.Description.Name.empty())
    {
      continue; // The nameless section that closes the map.
    }
    std::expected<std::vector<std::byte>, Failure> bytes = reading.Section(entry);
    if (!bytes)
    {
      return std::unexpected(bytes.error());
    }
    layout.Sections.push_back(Paged::Section{.Name = Paged::SectionNameOf(entry.Description.Name),
                                             .Bytes = std::move(*bytes),
                                             .Compressed = entry.Description.Encoding == Interleaved,
                                             .PageSize = static_cast<std::uint32_t>(entry.Description.PageSize),
                                             .Id = static_cast<std::int32_t>(layout.Sections.size())});
    layout.Descriptions.push_back(entry.Description);
  }
  return layout;
}
} // namespace DwgIO::File::Coded
