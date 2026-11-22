/***********************************************************************************************************************
 *  @file      tests/dwgio/file/coded/layout.cpp
 *  @brief     The coded file of R2007: its header, maps and sections read, written and refused.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/coded/layout.hpp"

#include "bits/lz21.hpp"
#include "bits/reed.hpp"
#include "file/paged/layout.hpp"
#include "support/fixtures.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::File::Coded
{
namespace
{
constexpr std::size_t HeaderAt = 0x80;
constexpr std::size_t PagesAt = 0x480;
constexpr std::size_t Fields = 0x110;

auto Long(std::span<const std::byte> bytes, std::size_t at) -> std::uint64_t
{
  std::uint64_t value = 0;
  for (std::size_t one = 0; one < 8; ++one)
  {
    value |= std::to_integer<std::uint64_t>(bytes[at + one]) << (8U * one);
  }
  return value;
}

void Put(std::span<std::byte> bytes, std::size_t at, std::uint64_t value)
{
  for (std::size_t one = 0; one < 8; ++one)
  {
    bytes[at + one] = static_cast<std::byte>(value >> (8U * one));
  }
}

auto RoundUpTo(std::size_t size, std::size_t to) -> std::size_t
{
  return (size + to - 1) / to * to;
}

/**
 *  The data of interleaved codewords of the system code.
 */
auto Unwoven(std::span<const std::byte> coded, std::size_t blocks) -> std::vector<std::byte>
{
  std::vector<std::byte> data;
  for (std::size_t block = 0; block < blocks; ++block)
  {
    for (std::size_t at = 0; at < Bits::Reed::System.Data; ++at)
    {
      data.push_back(coded[(blocks * at) + block]);
    }
  }
  return data;
}

/**
 *  A file taken apart: the header's fields, and the page map and the section map with where their pages sit.
 */
struct Apart
{
  std::vector<std::byte> File;
  std::vector<std::byte> Fields;
  std::vector<std::byte> PageMap;
  std::vector<std::byte> SectionMap;
  std::vector<std::size_t> SectionMapAt;
  std::size_t SectionMapSize{};
  /**
   *  Whether the section map is put back as it is, not compressed.
   */
  bool Raw{};
};

auto System(std::span<const std::byte> file, std::size_t address, std::uint64_t stored, std::uint64_t size,
            std::uint64_t correction) -> std::vector<std::byte>
{
  const std::size_t blocks =
      (RoundUpTo(stored, 8) * correction + Bits::Reed::System.Data - 1) / Bits::Reed::System.Data;
  const std::vector<std::byte> data = Unwoven(file.subspan(address, blocks * Bits::Reed::Codeword), blocks);
  const std::span<const std::byte> first = std::span{data}.first(stored);
  return stored < size ? *Bits::Lz21::Decompress(first, size) : std::vector<std::byte>{first.begin(), first.end()};
}

auto Taken(std::vector<std::byte> file) -> Apart
{
  Apart apart;
  const std::vector<std::byte> header = Unwoven(std::span{file}.subspan(HeaderAt, 3 * Bits::Reed::Codeword), 3);
  const auto length = static_cast<std::int64_t>(Long(header, 0x18));
  const std::span<const std::byte> body =
      std::span{header}.subspan(0x20, static_cast<std::size_t>(length < 0 ? -length : length));
  apart.Fields = length < 0 ? std::vector<std::byte>{body.begin(), body.end()} : *Bits::Lz21::Decompress(body, Fields);
  const std::span<const std::byte> fields = apart.Fields;
  apart.PageMap =
      System(file, PagesAt + Long(fields, 0x38), Long(fields, 0x50), Long(fields, 0x58), Long(fields, 0x18));
  std::uint64_t offset = 0;
  for (std::size_t at = 0; at + 16 <= apart.PageMap.size(); at += 16)
  {
    const std::uint64_t size = Long(apart.PageMap, at);
    const std::uint64_t id = Long(apart.PageMap, at + 8);
    if (id == Long(fields, 0xC0) || id == Long(fields, 0xB8))
    {
      apart.SectionMapAt.push_back(PagesAt + offset);
      apart.SectionMapSize = size;
    }
    offset += size;
  }
  apart.SectionMap =
      System(file, apart.SectionMapAt.front(), Long(fields, 0xB0), Long(fields, 0xC8), Long(fields, 0xD8));
  apart.File = std::move(file);
  return apart;
}

/**
 *  A system page of `size` bytes holding the data compressed, repeated; the stored size and the repeat count.
 */
auto MakeSystemPage(std::span<const std::byte> data, std::size_t size, bool raw, std::uint64_t &stored,
                    std::uint64_t &correction) -> std::vector<std::byte>
{
  std::vector<std::byte> compressed =
      raw ? std::vector<std::byte>{data.begin(), data.end()} : Bits::Lz21::Compress(data);
  stored = compressed.size();
  compressed.resize(RoundUpTo(compressed.size(), 8), std::byte{0});
  correction = size / Bits::Reed::Codeword * Bits::Reed::System.Data / compressed.size();
  std::vector<std::byte> repeated;
  for (std::uint64_t copy = 0; copy < correction; ++copy)
  {
    repeated.insert(repeated.end(), compressed.begin(), compressed.end());
  }
  std::vector<std::byte> page = Bits::Reed::Interleave(repeated, Bits::Reed::System);
  page.resize(size, std::byte{0});
  return page;
}

/**
 *  The file put back together: the section map coded again in its pages, the header with its fields stored as they are,
 *  the negative length saying so.
 */
auto Together(Apart apart) -> std::vector<std::byte>
{
  std::uint64_t stored = 0;
  std::uint64_t correction = 0;
  const std::vector<std::byte> sections =
      MakeSystemPage(apart.SectionMap, apart.SectionMapSize, apart.Raw, stored, correction);
  for (const std::size_t at : apart.SectionMapAt)
  {
    std::ranges::copy(sections, apart.File.begin() + static_cast<std::ptrdiff_t>(at));
  }
  Put(apart.Fields, 0xB0, stored);
  Put(apart.Fields, 0xC8, apart.SectionMap.size());
  Put(apart.Fields, 0xD8, correction);
  std::vector<std::byte> block(0x20, std::byte{0});
  Put(block, 0x18, static_cast<std::uint64_t>(-static_cast<std::int64_t>(apart.Fields.size())));
  block.insert(block.end(), apart.Fields.begin(), apart.Fields.end());
  block.resize(3 * Bits::Reed::System.Data, std::byte{0});
  const std::vector<std::byte> header = Bits::Reed::Interleave(block, Bits::Reed::System);
  std::ranges::copy(header, apart.File.begin() + HeaderAt);
  return apart.File;
}

auto Section(std::string name, std::vector<std::byte> bytes) -> Paged::Section
{
  return Paged::Section{.Name = std::move(name), .Bytes = std::move(bytes), .Compressed = true, .PageSize = 0, .Id = 0};
}

auto Counting(std::size_t size) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes(size);
  std::uint32_t seed = 7;
  for (std::byte &one : bytes)
  {
    seed = (seed * 1103515245U) + 12345U;
    one = static_cast<std::byte>(seed >> 16U);
  }
  return bytes;
}

/**
 *  A small file of dwgio's: a header section, a preview, a summary and an objects section of three pages, its middle
 *  page all zeros.
 */
auto Small() -> std::vector<std::byte>
{
  std::vector<std::byte> objects = Counting(0xf800 * 3);
  std::fill(objects.begin() + 0xf800, objects.begin() + (2 * 0xf800), std::byte{0});
  const std::vector<Paged::Section> sections{Section("header", Counting(0x300)), Section("preview", Counting(0x500)),
                                             Section("AcDb:SummaryInfo", Counting(0x40)),
                                             Section("objects", std::move(objects))};
  Meta meta;
  meta.Release = Version::R2007;
  std::vector<Issue> issues;
  return WriteLayout(meta, sections, issues);
}

/**
 *  Where a section's entry starts in the section map, by name.
 */
auto EntryAt(std::span<const std::byte> map, std::string_view name) -> std::size_t
{
  for (std::size_t at = 0; at + 0x40 <= map.size();)
  {
    const std::uint64_t named = Long(map, at + 0x20);
    std::string units;
    for (std::size_t unit = 0; unit + 1 < named; unit += 2)
    {
      if (map[at + 0x40 + unit] != std::byte{0})
      {
        units.push_back(static_cast<char>(map[at + 0x40 + unit]));
      }
    }
    if (units == name)
    {
      return at;
    }
    at += 0x40 + named + (Long(map, at + 0x38) * 56);
  }
  FAIL("no section " << name);
  return 0;
}

auto ReadFailure(const std::vector<std::byte> &file) -> std::string
{
  std::vector<Issue> issues;
  const std::expected<Layout, Failure> layout = ReadLayout(file, issues);
  REQUIRE_FALSE(layout);
  return layout.error().What;
}
} // namespace

TEST_CASE("file/coded/layout: the empty R2007 fixture's metadata and sections and descriptions read")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2007.dwg");
  std::vector<Issue> issues;
  const std::expected<Layout, Failure> layout = ReadLayout(file, issues);
  REQUIRE(layout);
  CHECK(issues.empty());
  CHECK(layout->Meta.Release == Version::R2007);
  CHECK(layout->Meta.Maintenance == 50);
  CHECK(layout->Meta.Flag == 3);
  CHECK(layout->Meta.Image == 0xDA0);
  CHECK(layout->Meta.Codepage == 30);
  std::vector<std::string> names;
  for (const Paged::Section &section : layout->Sections)
  {
    names.push_back(section.Name);
  }
  CHECK(names == std::vector<std::string>{"AcDb:AppInfoHistory", "AcDb:AppInfo", "preview", "AcDb:SummaryInfo",
                                          "AcDb:RevHistory", "objects", "AcDb:ObjFreeSpace", "measurement", "map",
                                          "classes", "auxiliary", "header"});
  CHECK(layout->Descriptions.at(1) == Described{.Size = 0x2c0,
                                                .PageSize = 0x300,
                                                .Encryption = 0,
                                                .Hash = 0x3fa0043e,
                                                .Unknown1 = 0,
                                                .Encoding = 1,
                                                .Name = "AcDb:AppInfo"});
  CHECK(layout->Descriptions.back().Encoding == 4);
  CHECK(layout->Sections.at(5).Compressed);
  CHECK_FALSE(layout->Sections.at(1).Compressed);
}

TEST_CASE("file/coded/layout: a file shorter than its header or of another release or encrypted fails")
{
  std::vector<std::byte> file = ReadFixture("empty-r2007.dwg");
  CHECK(ReadMeta(std::span{file}.first(0x47F)).error().What == "coded header unreadable");
  CHECK(ReadMeta(ReadFixture("empty-r2004.dwg")).error().What == "coded header unreadable");
  file[0x18] = std::byte{4};
  CHECK(ReadMeta(file).error() ==
        Failure{.Code = FailureCode::Encrypted, .Offset = 0x18, .What = "encrypted drawing not read"});
  std::vector<Issue> issues;
  CHECK(ReadLayout(file, issues).error().What == "encrypted drawing not read");
}

TEST_CASE("file/coded/layout: a file dwgio writes reads back without the sections R2007 cannot name")
{
  std::vector<Paged::Section> sections{Section("header", Counting(0x300)), Section("AcDb:Unheard", Counting(0x10)),
                                       Section("AcDb:AppInfo", Counting(0x400))};
  Meta meta;
  meta.Release = Version::R2007;
  meta.Maintenance = 50;
  std::vector<Issue> issues;
  const std::vector<std::byte> file = WriteLayout(meta, sections, issues);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "section AcDb:Unheard left out: R2007 has no hash for it");
  CHECK(meta.Image == 0);
  issues.clear();
  const std::expected<Layout, Failure> layout = ReadLayout(file, issues);
  REQUIRE(layout);
  CHECK(issues.empty());
  REQUIRE(layout->Sections.size() == 2);
  CHECK(layout->Sections.at(0).Bytes == sections.at(0).Bytes);
  CHECK(layout->Sections.at(1).Bytes == sections.at(2).Bytes);
  CHECK(layout->Descriptions.at(1).PageSize == 0x400); // A whole section in one page, its own size rounded.
  CHECK(layout->Meta.Maintenance == 50);

  const std::vector<std::byte> small = Small();
  issues.clear();
  const std::expected<Layout, Failure> again = ReadLayout(small, issues);
  REQUIRE(again);
  CHECK(issues.empty());
  REQUIRE(again->Sections.size() == 4);
  CHECK(again->Sections.at(3).Bytes.size() == 0xf800 * 3);
  CHECK(std::ranges::all_of(std::span{again->Sections.at(3).Bytes}.subspan(0xf800, 0xf800),
                            [](std::byte one) -> bool { return one == std::byte{0}; }));
  CHECK(again->Meta.Image != 0);
  CHECK(Long(small, 0x20) != 0); // The summary's address.
}

TEST_CASE("file/coded/layout: a section whose last page is all zeros reads back whole")
{
  std::vector<std::byte> objects = Counting(0xf800 * 3);
  std::fill(objects.begin() + (2 * 0xf800), objects.end(), std::byte{0});
  const std::vector<Paged::Section> sections{Section("header", Counting(0x300)), Section("objects", objects)};
  Meta meta;
  meta.Release = Version::R2007;
  std::vector<Issue> issues;
  const std::vector<std::byte> file = WriteLayout(meta, sections, issues);
  const std::expected<Layout, Failure> layout = ReadLayout(file, issues);
  REQUIRE(layout);
  CHECK(issues.empty());
  REQUIRE(layout->Sections.size() == 2);
  REQUIRE(layout->Sections.at(1).Bytes.size() == objects.size());
  CHECK(layout->Sections.at(1).Bytes == objects);
}

TEST_CASE("file/coded/layout: a stored header reads and one with a wrong length or data does not")
{
  const std::vector<std::byte> small = Small();
  std::vector<Issue> issues;
  CHECK(ReadLayout(Together(Taken(small)), issues));
  Apart raw = Taken(small);
  raw.Raw = true; // A section map stored as it is.
  const std::expected<Layout, Failure> plain = ReadLayout(Together(std::move(raw)), issues);
  REQUIRE(plain);
  CHECK(plain->Sections.size() == 4);

  Apart broken = Taken(small);
  std::vector<std::byte> block(3 * Bits::Reed::System.Data, std::byte{0});
  std::ranges::copy(Bits::Reed::Interleave(block, Bits::Reed::System), broken.File.begin() + HeaderAt);
  CHECK(ReadFailure(broken.File) == "coded file header unreadable"); // A length of zero.
  Put(block, 0x18, 0x40);
  block[0x20] = std::byte{0x2F}; // A reference with nothing before it.
  std::ranges::copy(Bits::Reed::Interleave(block, Bits::Reed::System), broken.File.begin() + HeaderAt);
  CHECK(ReadFailure(broken.File) == "coded file header unreadable");
  Put(block, 0x18, 0x1000); // Longer than the codewords hold.
  std::ranges::copy(Bits::Reed::Interleave(block, Bits::Reed::System), broken.File.begin() + HeaderAt);
  CHECK(ReadFailure(broken.File) == "coded file header unreadable");
  Put(block, 0x18, 9); // A run of eight bytes, short of the fields.
  block[0x20] = std::byte{0};
  std::ranges::copy(Bits::Reed::Interleave(block, Bits::Reed::System), broken.File.begin() + HeaderAt);
  CHECK(ReadFailure(broken.File) == "coded file header unreadable");
}

TEST_CASE("file/coded/layout: a page or section map that does not decode or names what is missing fails")
{
  const std::vector<std::byte> small = Small();
  const auto edited = [&small](const std::function<void(Apart &)> &edit) -> std::vector<std::byte>
  {
    Apart apart = Taken(small);
    edit(apart);
    return Together(std::move(apart));
  };
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.Fields, 0x50, 0); })) == "page map unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.Fields, 0x38, 0x1000000); })) == "page map unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.Fields, 0x18, 0); })) == "page map unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.Fields, 0x58, 0x2000000); })) == "page map unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.Fields, 0x50, 0x2000000); })) == "page map unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.Fields, 0x18, 0x2000000); })) == "page map unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.Fields, 0xC0, 99); })) == "section map unreadable");
  // A first entry naming more bytes of name than the map has.
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.SectionMap, 0x20, 0x10000); })) == "section map unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.SectionMap, 0x38, 0x10000); })) == "section map unreadable");
}

TEST_CASE("file/coded/layout: a missing or undecodable section page or a wrong checksum fails")
{
  const std::vector<std::byte> small = Small();
  const auto edited = [&small](const std::function<void(Apart &)> &edit) -> std::vector<std::byte>
  {
    Apart apart = Taken(small);
    edit(apart);
    return Together(std::move(apart));
  };
  // The header section's first page record follows its 0x40 bytes and the name of 12 units.
  constexpr std::size_t Record = 0x40 + 24;
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.SectionMap, Record + 0x10, 99); })) ==
        "section AcDb:Header page 99 missing");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.SectionMap, Record + 0x20, 0); })) ==
        "section AcDb:Header page 3 unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.SectionMap, Record + 0x20, 0x100000); })) ==
        "section AcDb:Header page 3 unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.SectionMap, Record + 0x20, 0x2000000); })) ==
        "section AcDb:Header page 3 unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.SectionMap, Record + 0x18, 0x2000000); })) ==
        "section AcDb:Header page 3 unreadable");
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.SectionMap, Record, 0x10000); })) ==
        "section AcDb:Header page 3 unreadable");
  // Compressed bytes that do not decompress.
  CHECK(ReadFailure(edited([](Apart &apart) { Put(apart.SectionMap, Record + 0x18, 0x400); })) ==
        "section AcDb:Header page 3 unreadable");
  std::vector<Issue> issues;
  const std::vector<std::byte> summed = edited([](Apart &apart) { Put(apart.SectionMap, Record + 0x28, 1); });
  REQUIRE(ReadLayout(summed, issues));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "section AcDb:Header page 3 checksum mismatch");
}

TEST_CASE("file/coded/layout: a section larger than its pages can hold or naming one page twice fails")
{
  const std::vector<std::byte> small = Small();
  const auto edited = [&small](const std::function<void(Apart &)> &edit) -> std::vector<std::byte>
  {
    Apart apart = Taken(small);
    edit(apart);
    return Together(std::move(apart));
  };
  // A size and a page start the file cannot hold must fail before a buffer grows to them.
  constexpr std::size_t Record = 0x40 + 24;
  CHECK(ReadFailure(edited(
            [](Apart &apart)
            {
              Put(apart.SectionMap, 0, 0x2000000);
              Put(apart.SectionMap, Record, 0x1F00000);
            })) == "section AcDb:Header size 33554432 beyond its pages");
  CHECK(ReadFailure(edited(
            [](Apart &apart)
            {
              Put(apart.SectionMap, 0, std::uint64_t{1} << 40U);
              Put(apart.SectionMap, Record, std::uint64_t{1} << 40U);
            })) == "section AcDb:Header size 1099511627776 beyond its pages");
  CHECK(ReadFailure(edited(
            [](Apart &apart)
            {
              const std::size_t objects = EntryAt(apart.SectionMap, "AcDb:AcDbObjects") + 0x40 + 34;
              Put(apart.SectionMap, objects + 56 + 0x10, Long(apart.SectionMap, objects + 0x10));
            })) == "section AcDb:AcDbObjects page 6 repeated");
}
} // namespace DwgIO::File::Coded
