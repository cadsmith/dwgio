/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/map.cpp
 *  @brief     The object map: read, written back, and noted when broken.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/map.hpp"

#include "bits/crc.hpp"
#include "bits/reader.hpp"
#include "file/locator/layout.hpp"
#include "support/fixtures.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
constexpr std::size_t MapOffset = 137708;

constexpr std::size_t MapSize = 708;

constexpr std::size_t ObjectsOffset = 21727;

auto Block() -> std::vector<std::byte>
{
  const std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  return {file.begin() + MapOffset, file.begin() + MapOffset + MapSize};
}
} // namespace

TEST_CASE("objects/map: the fixture's map lists every object by handle and offset")
{
  std::vector<Issue> issues;
  const std::vector<Mapped> entries = ReadMap(Block(), MapOffset, issues);
  REQUIRE(entries.size() == 219);
  CHECK(entries.front().Self == Handle{0x1});
  CHECK(entries.back().Self == Handle{0x416});
  const auto model = std::ranges::find(entries, Handle{0x1F}, &Mapped::Self);
  REQUIRE(model != entries.end());
  CHECK(model->Offset >= ObjectsOffset);
  CHECK(model->Offset < MapOffset);
  CHECK(std::ranges::is_sorted(entries, {}, &Mapped::Self));
  CHECK(issues.empty());
}

TEST_CASE("objects/map: a section with a wrong CRC is noted and still read")
{
  std::vector<std::byte> block = Block();
  block[MapSize - 5] ^= std::byte{0x01};
  std::vector<Issue> issues;
  const std::vector<Mapped> entries = ReadMap(block, MapOffset, issues);
  CHECK(entries.size() == 219);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() ==
        Issue{.Code = IssueCode::Checksum, .Offset = MapOffset, .What = "object map section CRC mismatch"});
}

TEST_CASE("objects/map: a truncated map is noted and gives what it had")
{
  std::vector<std::byte> block = Block();
  block.resize(100);
  std::vector<Issue> issues;
  const std::vector<Mapped> entries = ReadMap(block, MapOffset, issues);
  CHECK(entries.empty());
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() ==
        Issue{.Code = IssueCode::Damaged, .Offset = MapOffset, .What = "object map section runs past the end"});
}

TEST_CASE("objects/map: the fixture's map writes back byte for byte")
{
  std::vector<Issue> issues;
  const std::vector<std::byte> block = Block();
  CHECK(WriteMap(ReadMap(block, MapOffset, issues)) == block);
}

TEST_CASE("objects/map: each section of a long map counts its offsets from zero and writes back byte for byte")
{
  const std::vector<std::byte> file = ReadFixture("many-r2000.dwg");
  std::vector<Issue> issues;
  const std::expected<File::Locator::Layout, Failure> layout = File::Locator::ReadLayout(file, issues);
  REQUIRE(layout);
  const auto map = std::ranges::find(layout->Blocks, std::string{"map"}, &File::Locator::Located::Name);
  REQUIRE(map != layout->Blocks.end());
  const std::vector<Mapped> entries = ReadMap(map->Bytes, map->Offset, issues);
  CHECK(issues.empty());
  REQUIRE(entries.size() > 1900);
  CHECK(map->Bytes.size() > 2 * 2032); // Three sections at least, each counting from zero.
  for (const Mapped &one : entries)    // Every entry names an object whose CRC checks where the map says it is.
  {
    CAPTURE(one.Self.Value);
    Bits::Reader in{std::span{file}.subspan(one.Offset)};
    const std::uint64_t size = in.MS();
    const std::size_t start = one.Offset + (in.Position() / 8);
    REQUIRE(start + size + 2 <= file.size());
    const auto stored = static_cast<std::uint16_t>(std::to_integer<unsigned>(file[start + size]) |
                                                   (std::to_integer<unsigned>(file[start + size + 1]) << 8U));
    CHECK(Bits::Crc(std::span{file}.subspan(one.Offset, start - one.Offset + size), Bits::Seed) == stored);
  }
  CHECK(WriteMap(entries) == map->Bytes);
}

TEST_CASE("objects/map: many entries split into sections of about 2032 bytes and read back")
{
  std::vector<Mapped> entries;
  for (std::uint64_t index = 1; index <= 3000; ++index)
  {
    entries.push_back(Mapped{.Self = Handle{index * 3}, .Offset = 1000 + (index * 97)});
  }
  std::vector<Mapped> shuffled = entries;
  std::ranges::reverse(shuffled);
  const std::vector<std::byte> bytes = WriteMap(shuffled);
  std::size_t sections = 0;
  for (std::size_t at = 0; at < bytes.size();)
  {
    const auto size = (static_cast<std::size_t>(bytes[at]) << 8) | static_cast<std::size_t>(bytes[at + 1]);
    CHECK(size <= 2032 + 10); // Closed once it reaches 2032, with the entry that reached it.
    ++sections;
    at += size + 2;
  }
  CHECK(sections > 2);
  std::vector<Issue> issues;
  CHECK(ReadMap(bytes, 0, issues) == entries);
  CHECK(issues.empty());
}

TEST_CASE("objects/map: an empty map is the terminator and its CRC")
{
  const std::vector<std::byte> bytes = WriteMap({});
  REQUIRE(bytes.size() == 4);
  CHECK(bytes[0] == std::byte{0x00});
  CHECK(bytes[1] == std::byte{0x02});
  std::vector<Issue> issues;
  CHECK(ReadMap(bytes, 0, issues).empty());
  CHECK(issues.empty());
}

TEST_CASE("objects/map: a section cut inside an entry gives what it had and a missing terminator is noted")
{
  const std::vector<std::byte> cut{std::byte{0x00}, std::byte{0x04}, std::byte{0x81},
                                   std::byte{0x80}, std::byte{0x00}, std::byte{0x00}};
  std::vector<Issue> issues;
  CHECK(ReadMap(cut, 0, issues).empty());
  CHECK(issues.size() == 2);
  std::vector<std::byte> block = Block();
  block.resize(MapSize - 4);
  issues.clear();
  CHECK(ReadMap(block, MapOffset, issues).size() == 219);
  CHECK(issues.back().What == "object map section runs past the end");
}

TEST_CASE("objects/map: a section size under two ends the map")
{
  const std::vector<std::byte> bytes{std::byte{0x00}, std::byte{0x01}, std::byte{0x00}, std::byte{0x00}};
  std::vector<Issue> issues;
  CHECK(ReadMap(bytes, 0, issues).empty());
  CHECK(issues.back().What == "object map section runs past the end");
}

TEST_CASE("objects/map: an entry whose offset falls below zero is dropped")
{
  const std::vector<std::byte> bytes{std::byte{0x00}, std::byte{0x04}, std::byte{0x01}, std::byte{0x41},
                                     std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x02},
                                     std::byte{0x00}, std::byte{0x00}};
  std::vector<Issue> issues;
  CHECK(ReadMap(bytes, 0, issues).empty());
}
} // namespace DwgIO::Objects
