/***********************************************************************************************************************
 *  @file      source/file/paged/pages.cpp
 *  @brief     The pages, page map and section map of the paged container.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/pages.hpp"

#include "bits/checksum.hpp"
#include "bits/lz.hpp"
#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "file/paged/magic.hpp"

#include <algorithm>
#include <array>
#include <format>

namespace DwgIO::File::Paged
{
namespace
{
constexpr std::uint32_t Mask = 0x4164536B;
constexpr std::uint32_t Packed = 2;
constexpr std::size_t NameSize = 64;
constexpr std::size_t GapTail = 16;
constexpr std::size_t Preamble = 16;
constexpr std::size_t Words = 8;
constexpr std::size_t HeaderChecksumWord = 6;
constexpr std::size_t Entry = 8;
constexpr std::uint32_t Compression = 2;

auto MakeUnreadable(std::size_t address) -> std::unexpected<Failure>
{
  return std::unexpected(Failure{
      .Code = FailureCode::Damaged, .Offset = address, .What = std::format("page at 0x{:X} unreadable", address)});
}

void NoteMismatch(std::size_t address, std::vector<Issue> &issues)
{
  issues.push_back(Issue{.Code = IssueCode::Checksum,
                         .Offset = address,
                         .What = std::format("page at 0x{:X} checksum mismatch", address)});
}

/**
 *  A system page's header.
 */
struct SystemHead
{
  std::uint32_t Type{};
  std::uint32_t Decompressed{};
  std::uint32_t Compressed{};
  std::uint32_t Checksum{};
};

auto ReadSystemHead(std::span<const std::byte> header) -> SystemHead
{
  Bits::Reader in{header};
  SystemHead head;
  head.Type = in.RL();
  head.Decompressed = in.RL();
  head.Compressed = in.RL();
  static_cast<void>(in.RL());
  head.Checksum = in.RL();
  return head;
}

/**
 *  A header's checksum: the header with its own checksum field zeroed, seeded as given.
 */
auto HeaderChecksum(std::span<const std::byte> header, std::size_t field, std::uint32_t seed) -> std::uint32_t
{
  std::vector<std::byte> zeroed(header.begin(), header.end());
  std::ranges::fill(std::span{zeroed}.subspan(field, 4), std::byte{0});
  return Bits::Checksum(seed, zeroed);
}

auto Text(std::span<const std::byte> bytes) -> std::string
{
  std::string text;
  for (const std::byte one : bytes)
  {
    if (one == std::byte{0})
    {
      break;
    }
    text.push_back(static_cast<char>(one));
  }
  return text;
}

auto ReadDescription(Bits::Reader &in) -> Description
{
  Description section;
  section.Size = in.RLL();
  const std::uint32_t pageCount = in.RL();
  section.PageSize = in.RL();
  section.Unknown1 = in.RL();
  section.Compressed = in.RL();
  section.Id = static_cast<std::int32_t>(in.RL());
  section.Encrypted = in.RL();
  section.Name = Text(in.Bytes(NameSize));
  for (std::uint32_t at = 0; at < pageCount && !in.HasFailed(); ++at)
  {
    SectionPage one;
    one.Number = in.RL();
    one.Size = in.RL();
    one.Start = in.RLL();
    section.Pages.push_back(one);
  }
  return section;
}

/**
 *  A data page's header, unmasked.
 */
struct DataHead
{
  std::array<std::uint32_t, Words> Fields{};

  [[nodiscard]] auto Type() const -> std::uint32_t { return Fields[0]; }

  [[nodiscard]] auto Size() const -> std::uint32_t { return Fields[2]; }

  [[nodiscard]] auto HeaderChecksum() const -> std::uint32_t { return Fields[HeaderChecksumWord]; }

  [[nodiscard]] auto DataChecksum() const -> std::uint32_t { return Fields[Words - 1]; }

  /**
   *  The header's bytes with its own checksum zero, as its checksum covers them.
   */
  [[nodiscard]] auto ZeroChecksum() const -> std::vector<std::byte>
  {
    Bits::Writer plain;
    for (std::size_t at = 0; at < Words; ++at)
    {
      plain.RL(at == HeaderChecksumWord ? 0 : Fields.at(at));
    }
    return plain.Take();
  }
};

auto ReadDataHead(std::span<const std::byte> header, std::size_t address) -> DataHead
{
  const std::uint32_t mask = Mask ^ static_cast<std::uint32_t>(address);
  DataHead head;
  Bits::Reader in{header};
  for (std::uint32_t &word : head.Fields)
  {
    word = in.RL() ^ mask;
  }
  return head;
}

auto Checks(const DataHead &head, std::span<const std::byte> data) -> bool
{
  return Bits::Checksum(0, data) == head.DataChecksum() &&
         Bits::Checksum(head.DataChecksum(), head.ZeroChecksum()) == head.HeaderChecksum();
}

/**
 *  A page's data onto the section's bytes: decompressed or stored, then zeros to the page size.
 */
auto AppendPage(std::span<const std::byte> data, const Description &section, std::vector<std::byte> &into) -> bool
{
  const std::size_t before = into.size();
  if (section.Compressed == Packed)
  {
    if (!Bits::Lz::Decompress(data, section.PageSize, into))
    {
      return false;
    }
  }
  else
  {
    const std::span<const std::byte> kept = data.first(std::min<std::size_t>(data.size(), section.PageSize));
    into.insert(into.end(), kept.begin(), kept.end());
  }
  into.resize(before + section.PageSize, std::byte{0});
  return true;
}

/**
 *  Pads to the next 0x20 boundary from the start of the magic sequence.
 */
void PadPage(std::vector<std::byte> &page)
{
  const std::size_t rest = page.size() % Alignment;
  if (rest != 0)
  {
    const std::vector<std::byte> magic = MakeMagic(Alignment - rest);
    page.insert(page.end(), magic.begin(), magic.end());
  }
}

/**
 *  The page's bytes as stored: compressed, or whole to the page size.
 */
auto Squeeze(const Description &section, std::span<const std::byte> page) -> std::vector<std::byte>
{
  if (section.Compressed == Packed)
  {
    return Bits::Lz::Compress(page);
  }
  std::vector<std::byte> whole(page.begin(), page.end());
  whole.resize(section.PageSize, std::byte{0});
  return whole;
}
} // namespace

auto WriteDataPage(const Placement &where, const Description &section, std::span<const std::byte> page) -> Stored
{
  const std::vector<std::byte> data = Squeeze(section, page);
  const std::size_t extent = ((DataHeader + data.size() + Alignment - 1) / Alignment) * Alignment;
  DataHead head;
  head.Fields = {DataPageType,
                 static_cast<std::uint32_t>(section.Id),
                 static_cast<std::uint32_t>(data.size()),
                 static_cast<std::uint32_t>(extent),
                 static_cast<std::uint32_t>(where.Start & 0xFFFFFFFFU),
                 static_cast<std::uint32_t>(where.Start >> 32U),
                 0,
                 Bits::Checksum(0, data)};
  head.Fields[HeaderChecksumWord] = Bits::Checksum(head.DataChecksum(), head.ZeroChecksum());
  const std::uint32_t mask = Mask ^ static_cast<std::uint32_t>(where.Address);
  Bits::Writer out;
  for (const std::uint32_t word : head.Fields)
  {
    out.RL(word ^ mask);
  }
  out.Raw(data);
  Stored stored{.Bytes = out.Take(), .Size = static_cast<std::uint32_t>(data.size())};
  PadPage(stored.Bytes);
  return stored;
}

auto WriteSystemPage(std::uint32_t type, std::span<const std::byte> bytes) -> std::vector<std::byte>
{
  const std::vector<std::byte> data = Bits::Lz::Compress(bytes);
  Bits::Writer header;
  header.RL(type);
  header.RL(static_cast<std::uint32_t>(bytes.size()));
  header.RL(static_cast<std::uint32_t>(data.size()));
  header.RL(Compression);
  header.RL(0);
  const std::uint32_t checksum = Bits::Checksum(Bits::Checksum(0, header.Bytes()), data);
  Bits::Writer out;
  out.Raw(header.Bytes().first(SystemHeader - 4));
  out.RL(checksum);
  out.Raw(data);
  out.RL(type);
  out.RL(0);
  out.RL(0);
  out.RL(Compression);
  out.RL(0);
  std::vector<std::byte> page = out.Take();
  PadPage(page);
  return page;
}

auto WritePageMap(std::span<const Page> pages) -> std::vector<std::byte>
{
  Bits::Writer out;
  for (const Page &page : pages)
  {
    out.RL(static_cast<std::uint32_t>(page.Number));
    out.RL(static_cast<std::uint32_t>(page.Size));
  }
  return out.Take();
}

auto WriteSectionMap(std::span<const Description> sections) -> std::vector<std::byte>
{
  Bits::Writer out;
  out.RL(static_cast<std::uint32_t>(sections.size()));
  out.RL(Compression);
  out.RL(UsualPage);
  out.RL(0);
  out.RL(static_cast<std::uint32_t>(sections.size()));
  for (const Description &section : sections)
  {
    out.RLL(section.Size);
    out.RL(static_cast<std::uint32_t>(section.Pages.size()));
    out.RL(section.PageSize);
    out.RL(section.Unknown1);
    out.RL(section.Compressed);
    out.RL(static_cast<std::uint32_t>(section.Id));
    out.RL(section.Encrypted);
    std::vector<std::byte> name(NameSize, std::byte{0});
    for (std::size_t at = 0; at < section.Name.size() && at < NameSize; ++at)
    {
      name[at] = static_cast<std::byte>(section.Name[at]);
    }
    out.Raw(name);
    for (const SectionPage &page : section.Pages)
    {
      out.RL(page.Number);
      out.RL(page.Size);
      out.RLL(page.Start);
    }
  }
  return out.Take();
}

auto FindPage(std::span<const Page> pages, std::uint32_t number) -> const Page *
{
  const auto page = std::ranges::find(pages, static_cast<std::int32_t>(number), &Page::Number);
  return page == pages.end() ? nullptr : &*page;
}

auto Pager::SystemPage(std::size_t address, std::uint32_t type) const -> std::expected<std::vector<std::byte>, Failure>
{
  const std::span<const std::byte> file = m_File;
  std::vector<Issue> &issues = *m_Issues;
  if (address + SystemHeader > file.size())
  {
    return MakeUnreadable(address);
  }
  const SystemHead head = ReadSystemHead(file.subspan(address, SystemHeader));
  if (head.Type != type || address + SystemHeader + head.Compressed > file.size())
  {
    return MakeUnreadable(address);
  }
  const std::span<const std::byte> data = file.subspan(address + SystemHeader, head.Compressed);
  const std::uint32_t first = HeaderChecksum(file.subspan(address, SystemHeader), SystemHeader - 4, 0);
  if (Bits::Checksum(first, data) != head.Checksum)
  {
    NoteMismatch(address, issues);
  }
  std::vector<std::byte> out;
  if (!Bits::Lz::Decompress(data, head.Decompressed, out))
  {
    return MakeUnreadable(address);
  }
  return out;
}

auto Pager::PageMap(const Head &head) const -> std::expected<std::vector<Page>, Failure>
{
  const std::uint64_t address = head.Record.PageMapAddress + Pages;
  const std::expected<std::vector<std::byte>, Failure> map =
      address > m_File.size()
          ? MakeUnreadable(static_cast<std::size_t>(std::min<std::uint64_t>(address, m_File.size())))
          : SystemPage(static_cast<std::size_t>(address), PageMapType);
  if (!map)
  {
    return std::unexpected(
        Failure{.Code = FailureCode::Damaged, .Offset = map.error().Offset, .What = "page map unreadable"});
  }
  std::vector<Page> pages;
  Bits::Reader in{*map};
  std::size_t at = Pages;
  while ((in.Position() / 8) + Entry <= map->size())
  {
    const auto number = static_cast<std::int32_t>(in.RL());
    const std::uint32_t size = in.RL();
    if (number < 0)
    {
      static_cast<void>(in.Bytes(GapTail));
    }
    pages.push_back(Page{.Number = number, .Address = at, .Size = size});
    at += size;
  }
  return pages;
}

auto Pager::SectionMap(std::span<const Page> pages, const Head &head) const
    -> std::expected<std::vector<Description>, Failure>
{
  const Page *page = FindPage(pages, head.Record.SectionMapId);
  const std::expected<std::vector<std::byte>, Failure> map =
      page == nullptr ? MakeUnreadable(0) : SystemPage(page->Address, SectionMapType);
  if (!map)
  {
    return std::unexpected(
        Failure{.Code = FailureCode::Damaged, .Offset = map.error().Offset, .What = "section map unreadable"});
  }
  Bits::Reader in{*map};
  const std::uint32_t count = in.RL();
  static_cast<void>(in.Bytes(Preamble));
  std::vector<Description> sections;
  for (std::uint32_t index = 0; index < count && !in.HasFailed(); ++index)
  {
    sections.push_back(ReadDescription(in));
  }
  if (in.HasFailed())
  {
    return std::unexpected(
        Failure{.Code = FailureCode::Damaged, .Offset = page->Address, .What = "section map unreadable"});
  }
  return sections;
}

auto Pager::DataPage(std::size_t address, const Description &section, std::vector<std::byte> &into) const -> bool
{
  const std::span<const std::byte> file = m_File;
  if (address + DataHeader > file.size())
  {
    return false;
  }
  const DataHead head = ReadDataHead(file.subspan(address, DataHeader), address);
  if (head.Type() != DataPageType || address + DataHeader + head.Size() > file.size())
  {
    return false;
  }
  const std::span<const std::byte> data = file.subspan(address + DataHeader, head.Size());
  if (!Checks(head, data))
  {
    NoteMismatch(address, *m_Issues);
  }
  return AppendPage(data, section, into);
}
} // namespace DwgIO::File::Paged
