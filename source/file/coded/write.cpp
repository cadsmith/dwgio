/***********************************************************************************************************************
 *  @file      source/file/coded/write.cpp
 *  @brief     An R2007 file written in its coded container.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/crc64.hpp"
#include "bits/lz21.hpp"
#include "bits/reed.hpp"
#include "file/coded/layout.hpp"
#include "file/names.hpp"
#include "file/paged/names.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <string_view>

namespace DwgIO::File::Coded
{
namespace
{
constexpr std::size_t Align = 8;
constexpr std::size_t PageAlign = 0x20;
constexpr std::size_t SmallestSystem = 0x400;
constexpr std::size_t Fields = 0x110;
constexpr std::uint64_t Interleave = 4;
constexpr std::uint64_t FirstData = 3;

/**
 *  A section R2007 writes: its name in the file, the hash the section map gives it, the size of its pages, and whether
 *  its data stays in one page.
 */
struct Hashed
{
  std::string_view Acdb;
  std::uint64_t Hash;
  std::uint64_t PageSize;
  bool Whole;
};

/**
 *  The hashes and page sizes of chapter 5.6, AcDb:AppInfoHistory's as AutoCAD's files have it.
 */
constexpr std::array Table{
    Hashed{.Acdb = Names::Security, .Hash = 0x4a0204ea, .PageSize = 0xf800, .Whole = true},
    Hashed{.Acdb = Names::FileDepList, .Hash = 0x6c4205ca, .PageSize = 0x100, .Whole = true},
    Hashed{.Acdb = Names::VbaProject, .Hash = 0x586e0544, .PageSize = 0x80, .Whole = true},
    Hashed{.Acdb = Names::AppInfo, .Hash = 0x3fa0043e, .PageSize = 0x300, .Whole = true},
    Hashed{.Acdb = Names::AppInfoHistory, .Hash = 0x96de0737, .PageSize = 0x600, .Whole = true},
    Hashed{.Acdb = Names::AcdbPreview, .Hash = 0x40aa0473, .PageSize = 0x400, .Whole = true},
    Hashed{.Acdb = Names::SummaryInfo, .Hash = 0x717a060f, .PageSize = 0x100, .Whole = true},
    Hashed{.Acdb = Names::RevHistory, .Hash = 0x60a205b3, .PageSize = 0x1000, .Whole = false},
    Hashed{.Acdb = Names::AcdbObjects, .Hash = 0x674c05a9, .PageSize = 0xf800, .Whole = false},
    Hashed{.Acdb = Names::ObjFreeSpace, .Hash = 0x77e2061f, .PageSize = 0xf800, .Whole = false},
    Hashed{.Acdb = Names::AcdbTemplate, .Hash = 0x4a1404ce, .PageSize = 0x400, .Whole = false},
    Hashed{.Acdb = Names::AcdbHandles, .Hash = 0x3f6e0450, .PageSize = 0xf800, .Whole = false},
    Hashed{.Acdb = Names::AcdbClasses, .Hash = 0x3f54045f, .PageSize = 0xf800, .Whole = false},
    Hashed{.Acdb = Names::AcdbAuxHeader, .Hash = 0x54f0050a, .PageSize = 0x800, .Whole = false},
    Hashed{.Acdb = Names::AcdbHeader, .Hash = 0x32b803d9, .PageSize = 0x800, .Whole = false},
};

/**
 *  The seeds AutoCAD's own files carry, each encoding a CRC seed of zero; AutoCAD refuses a file whose seeds it did not
 *  make, and no file dwgio writes needs another.
 */
constexpr std::uint64_t PagesMapSeed = 0x05163425c870a243;
constexpr std::uint64_t SectionsMapSeed = 0x531c70c0480945cd;
constexpr std::uint64_t SeedEncoded = 0x16451d928135c54c;
constexpr std::uint64_t RandomSeed = 0x4f2d34e7e86ef53f;

auto RoundUp(std::size_t size, std::size_t to) -> std::size_t
{
  return (size + to - 1) / to * to;
}

void Long(std::vector<std::byte> &out, std::uint64_t value)
{
  for (std::size_t one = 0; one < sizeof(value); ++one)
  {
    out.push_back(static_cast<std::byte>(value >> (8U * one)));
  }
}

/**
 *  Writes a value little-endian into as many bytes as `into` has.
 */
void Put(std::span<std::byte> into, std::uint64_t value)
{
  for (std::size_t one = 0; one < into.size(); ++one)
  {
    into[one] = static_cast<std::byte>(value >> (8U * one));
  }
}

/**
 *  The bytes compressed when that makes them smaller, else as they are.
 */
auto Smaller(std::span<const std::byte> bytes) -> std::vector<std::byte>
{
  const std::vector<std::byte> compressed = Bits::Lz21::Compress(bytes);
  return compressed.size() < bytes.size() ? compressed : std::vector<std::byte>{bytes.begin(), bytes.end()};
}

/**
 *  The stored bytes padded to a multiple of eight, as the codes take them.
 */
auto Pad(std::span<const std::byte> stored) -> std::vector<std::byte>
{
  std::vector<std::byte> padded{stored.begin(), stored.end()};
  padded.resize(RoundUp(padded.size(), Align), std::byte{0});
  return padded;
}

/**
 *  A system page of chapter 5.3, its data compressed, repeated as often as the page holds it.
 */
struct System
{
  std::vector<std::byte> Page;
  std::uint64_t Stored{};
  std::uint64_t Correction{};
  std::uint64_t Crc{};
  std::uint64_t Plain{};
};

auto SystemSize(std::size_t size) -> std::size_t
{
  const std::size_t page =
      ((RoundUp(size, Align) * 2) + Bits::Reed::System.Data - 1) / Bits::Reed::System.Data * Bits::Reed::Codeword;
  return page < SmallestSystem ? SmallestSystem : RoundUp(page, PageAlign);
}

auto MakeSystem(std::span<const std::byte> data) -> System
{
  const std::vector<std::byte> stored = Smaller(data);
  const std::vector<std::byte> padded = Pad(stored);
  const std::size_t size = SystemSize(data.size());
  const std::size_t correction = size / Bits::Reed::Codeword * Bits::Reed::System.Data / padded.size();
  std::vector<std::byte> repeated;
  for (std::size_t copy = 0; copy < correction; ++copy)
  {
    repeated.insert(repeated.end(), padded.begin(), padded.end());
  }
  std::vector<std::byte> page = Bits::Reed::Interleave(repeated, Bits::Reed::System);
  page.resize(size, std::byte{0});
  return System{.Page = std::move(page),
                .Stored = stored.size(),
                .Correction = correction,
                .Crc = Bits::Crc64::MirrorCrc(stored, Bits::Crc64::SeedOne(0, stored.size())),
                .Plain = Bits::Crc64::MirrorCrc(data, Bits::Crc64::SeedOne(0, data.size()))};
}

/**
 *  The file header page of chapter 5.2.1: its fields, a CRC over them, compressed after a check sequence of zeros,
 *  repeated as often as three codewords hold it; the check data at the end left zero, which AutoCAD does not read.
 */
auto HeaderPage(std::vector<std::byte> fields) -> std::vector<std::byte>
{
  constexpr std::size_t CrcAt = 0x108;
  constexpr std::size_t Sequence = 16;
  constexpr std::size_t Codewords = 3;
  Put(std::span{fields}.subspan(CrcAt), Bits::Crc64::NormalCrc(fields, Bits::Crc64::SeedTwo(0, fields.size())));
  const std::vector<std::byte> stored = Bits::Lz21::Compress(fields); // AutoCAD refuses some uncompressed ones.
  const std::array<std::byte, Sequence> sequence{};
  std::vector<std::byte> block;
  Long(block, Bits::Crc64::NormalCrc(sequence, Bits::Crc64::SeedOne(0, Sequence)));
  Long(block, 0);
  Long(block, Bits::Crc64::NormalCrc(stored, Bits::Crc64::SeedTwo(0, stored.size())));
  Long(block, stored.size());
  block.insert(block.end(), stored.begin(), stored.end());
  block = Pad(block);
  const std::size_t room = Codewords * Bits::Reed::System.Data;
  std::vector<std::byte> repeated;
  while (repeated.size() + block.size() <= room)
  {
    repeated.insert(repeated.end(), block.begin(), block.end());
  }
  repeated.resize(room, std::byte{0});
  std::vector<std::byte> page = Bits::Reed::Interleave(repeated, Bits::Reed::System);
  page.resize(HeaderSize, std::byte{0});
  return page;
}

/**
 *  A data page as the file lays it: its id and its coded bytes.
 */
struct Laid
{
  std::uint64_t Id{};
  std::vector<std::byte> Bytes;
};

/**
 *  The section map and the data pages it describes, in file order.
 */
struct Mapped
{
  std::vector<std::byte> Map;
  std::vector<Laid> Data;
  std::uint64_t Sections{};
  std::uint64_t Preview{}; /**< The id of the preview's first page, zero for none. */
  std::uint64_t Summary{}; /**< The id of the summary's first page, zero for none. */
};

/**
 *  A section map entry's fields before its pages.
 */
struct Entry
{
  std::uint64_t Size{};
  std::uint64_t PageSize{};
  std::uint64_t Hash{};
  std::string_view Name;
  std::uint64_t Pages{};
};

void Describe(std::vector<std::byte> &map, const Entry &entry)
{
  Long(map, entry.Size);
  Long(map, entry.PageSize);
  Long(map, 0);
  Long(map, entry.Hash);
  Long(map, entry.Name.empty() ? 0 : (entry.Name.size() + 1) * 2);
  Long(map, 0);
  Long(map, Interleave);
  Long(map, entry.Pages);
  for (const char one : entry.Name)
  {
    map.push_back(static_cast<std::byte>(one)); // AutoCAD's section names are ASCII, so UTF-16 a byte at a time.
    map.push_back(std::byte{0});
  }
  if (!entry.Name.empty())
  {
    map.insert(map.end(), 2, std::byte{0});
  }
}

auto Map(std::span<const Paged::Section> sections, std::vector<Issue> &issues) -> Mapped
{
  constexpr std::uint64_t Usual = 0xf800;
  Mapped mapped;
  std::uint64_t number = FirstData;
  for (const Paged::Section &section : sections)
  {
    const std::string acdb = Paged::AcdbOf(section.Name);
    const auto known = std::ranges::find(Table, acdb, &Hashed::Acdb);
    if (known == Table.end())
    {
      issues.push_back(Issue{.Code = IssueCode::NotKept,
                             .What = std::format("section {} left out: R2007 has no hash for it", acdb)});
      continue;
    }
    const std::span<const std::byte> bytes = section.Bytes;
    const std::uint64_t size =
        known->Whole ? std::max<std::uint64_t>(known->PageSize, RoundUp(bytes.size(), PageAlign)) : known->PageSize;
    std::vector<std::byte> records;
    std::uint64_t pages = 0;
    for (std::size_t start = 0; start < bytes.size(); start += size)
    {
      const std::span<const std::byte> chunk = bytes.subspan(start, std::min<std::size_t>(size, bytes.size() - start));
      if (start > 0 && std::ranges::all_of(chunk, [](std::byte one) -> bool { return one == std::byte{0}; }))
      {
        continue; // A page of zeros the reader puts back from the next page's start or the section's size.
      }
      const std::vector<std::byte> stored = Smaller(chunk);
      std::vector<std::byte> coded = Bits::Reed::Interleave(Pad(stored), Bits::Reed::Pages);
      coded.resize(RoundUp(coded.size(), PageAlign), std::byte{0});
      Long(records, start);
      Long(records, chunk.size());
      Long(records, number);
      Long(records, chunk.size());
      Long(records, stored.size());
      Long(records, Bits::Lz21::Checksum(0, chunk));
      Long(records, Bits::Crc64::MirrorCrc(stored, Bits::Crc64::SeedOne(0, stored.size())));
      if (start == 0 && acdb == Names::AcdbPreview)
      {
        mapped.Preview = number;
      }
      if (start == 0 && acdb == Names::SummaryInfo)
      {
        mapped.Summary = number;
      }
      mapped.Data.push_back(Laid{.Id = number++, .Bytes = std::move(coded)});
      ++pages;
    }
    Describe(mapped.Map,
             Entry{.Size = bytes.size(), .PageSize = size, .Hash = known->Hash, .Name = acdb, .Pages = pages});
    mapped.Map.insert(mapped.Map.end(), records.begin(), records.end());
    ++mapped.Sections;
  }
  // The nameless section that closes the map.
  Describe(mapped.Map, Entry{.Size = 0, .PageSize = Usual, .Hash = 0, .Name = {}, .Pages = 0});
  ++mapped.Sections;
  return mapped;
}

/**
 *  The file header's fields of chapter 5.2, in their order; the last, its CRC, filled when the page is made.
 */
struct Header
{
  std::uint64_t FileSize{};
  System PageMap;
  std::uint64_t PageMapOffset{};
  std::uint64_t PageMapId{};
  std::uint64_t PageMapCopyOffset{};
  std::uint64_t PageMapCopyId{};
  std::uint64_t CopyOffset{};
  std::uint64_t PageMapSize{};
  std::uint64_t PagesAmount{};
  System SectionMap;
  std::uint64_t SectionMapSize{};
  std::uint64_t SectionMapId{};
  std::uint64_t SectionMapCopyId{};
  std::uint64_t Sections{};
};

auto WriteHeaderFields(const Header &header) -> std::vector<std::byte>
{
  constexpr std::uint64_t Size = 0x70;
  constexpr std::uint64_t Twenty = 0x20;
  constexpr std::uint64_t Forty = 0x40;
  constexpr std::uint64_t Usual = 0xf800;
  constexpr std::uint64_t StreamVersion = 0x60100;
  const std::array<std::uint64_t, Fields / sizeof(std::uint64_t)> values{Size,
                                                                         header.FileSize,
                                                                         header.PageMap.Crc,
                                                                         header.PageMap.Correction,
                                                                         PagesMapSeed,
                                                                         header.PageMapCopyOffset,
                                                                         header.PageMapCopyId,
                                                                         header.PageMapOffset,
                                                                         header.PageMapId,
                                                                         header.CopyOffset,
                                                                         header.PageMap.Stored,
                                                                         header.PageMapSize,
                                                                         header.PagesAmount,
                                                                         header.PageMapCopyId,
                                                                         Twenty,
                                                                         Forty,
                                                                         header.PageMap.Plain,
                                                                         Usual,
                                                                         Interleave,
                                                                         1,
                                                                         header.Sections,
                                                                         header.SectionMap.Plain,
                                                                         header.SectionMap.Stored,
                                                                         header.SectionMapCopyId,
                                                                         header.SectionMapId,
                                                                         header.SectionMapSize,
                                                                         header.SectionMap.Crc,
                                                                         header.SectionMap.Correction,
                                                                         SectionsMapSeed,
                                                                         StreamVersion,
                                                                         0,
                                                                         SeedEncoded,
                                                                         RandomSeed,
                                                                         0};
  std::vector<std::byte> fields;
  for (const std::uint64_t value : values)
  {
    Long(fields, value);
  }
  return fields;
}

/**
 *  The plain first 0x80 bytes, laid out as R2004's.
 */
auto MetaBytes(const Meta &meta, std::uint32_t summary) -> std::vector<std::byte>
{
  constexpr std::uint32_t Eighty = 0x80;
  std::vector<std::byte> out(MetaSize, std::byte{0});
  const std::string_view code = AcadverOf(meta.Release);
  std::ranges::transform(code, out.begin(), [](char one) -> std::byte { return static_cast<std::byte>(one); });
  Put(std::span{out}.subspan(0x0B, 1), meta.Maintenance);
  Put(std::span{out}.subspan(0x0C, 1), meta.Flag);
  Put(std::span{out}.subspan(0x0D, 4), meta.Image);
  Put(std::span{out}.subspan(0x11, 1), meta.DwgVersion);
  Put(std::span{out}.subspan(0x12, 1), meta.MaintRelease);
  Put(std::span{out}.subspan(0x13, 2), meta.Codepage);
  Put(std::span{out}.subspan(0x16, 1), meta.DwgVersion);
  Put(std::span{out}.subspan(0x17, 1), meta.MaintRelease);
  Put(std::span{out}.subspan(0x20, 4), summary);
  Put(std::span{out}.subspan(0x28, 4), Eighty);
  return out;
}
} // namespace

auto WriteLayout(Meta &meta, std::span<const Paged::Section> sections, std::vector<Issue> &issues)
    -> std::vector<std::byte>
{
  constexpr std::uint64_t Pair = 16;
  constexpr std::uint64_t Systems = 4;
  const Mapped mapped = Map(sections, issues);
  Header header;
  header.Sections = mapped.Sections;
  header.SectionMapSize = mapped.Map.size();
  header.SectionMap = MakeSystem(mapped.Map);
  header.SectionMapId = FirstData + mapped.Data.size();
  header.SectionMapCopyId = header.SectionMapId + 1;
  header.PageMapId = header.SectionMapId + 2;
  header.PageMapCopyId = header.SectionMapId + 3;
  header.PagesAmount = mapped.Data.size() + Systems;
  header.PageMapSize = header.PagesAmount * Pair;
  const std::uint64_t pageMapPage = SystemSize(header.PageMapSize);
  // The page map: its own two pages first, the data pages, then the section map's two.
  std::vector<std::byte> pageMap;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> order{{pageMapPage, header.PageMapId},
                                                             {pageMapPage, header.PageMapCopyId}};
  for (const Laid &laid : mapped.Data)
  {
    order.emplace_back(laid.Bytes.size(), laid.Id);
  }
  order.emplace_back(header.SectionMap.Page.size(), header.SectionMapId);
  order.emplace_back(header.SectionMap.Page.size(), header.SectionMapCopyId);
  std::uint64_t offset = 0;
  std::uint32_t summary = 0;
  meta.Image = 0;
  for (const auto &[size, number] : order)
  {
    if (number == mapped.Preview)
    {
      meta.Image = static_cast<std::uint32_t>(Pages + offset);
    }
    if (number == mapped.Summary)
    {
      summary = static_cast<std::uint32_t>(Pages + offset);
    }
    Long(pageMap, size);
    Long(pageMap, number);
    offset += size;
  }
  header.PageMap = MakeSystem(pageMap);
  header.PageMapOffset = 0;
  header.PageMapCopyOffset = pageMapPage;
  header.CopyOffset = offset;
  header.FileSize = Pages + offset + HeaderSize;
  const std::vector<std::byte> page = HeaderPage(WriteHeaderFields(header));
  std::vector<std::byte> file = MetaBytes(meta, summary);
  file.insert(file.end(), page.begin(), page.end());
  file.insert(file.end(), header.PageMap.Page.begin(), header.PageMap.Page.end());
  file.insert(file.end(), header.PageMap.Page.begin(), header.PageMap.Page.end());
  for (const Laid &laid : mapped.Data)
  {
    file.insert(file.end(), laid.Bytes.begin(), laid.Bytes.end());
  }
  file.insert(file.end(), header.SectionMap.Page.begin(), header.SectionMap.Page.end());
  file.insert(file.end(), header.SectionMap.Page.begin(), header.SectionMap.Page.end());
  file.insert(file.end(), page.begin(), page.end());
  return file;
}
} // namespace DwgIO::File::Coded
