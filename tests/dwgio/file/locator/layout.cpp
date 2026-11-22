/***********************************************************************************************************************
 *  @file      tests/dwgio/file/locator/layout.cpp
 *  @brief     A locator file cut into its blocks, unclaimed bytes and a map beyond the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/layout.hpp"

#include "support/fixtures.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace DwgIO::File::Locator
{
namespace
{
auto Names(const Layout &layout) -> std::vector<std::string>
{
  std::vector<std::string> names;
  for (const Located &block : layout.Blocks)
  {
    names.push_back(block.Name);
  }
  return names;
}

auto Offsets(const Layout &layout) -> std::vector<std::size_t>
{
  std::vector<std::size_t> offsets;
  for (const Located &block : layout.Blocks)
  {
    offsets.push_back(block.Offset);
  }
  return offsets;
}

auto Sizes(const Layout &layout) -> std::vector<std::size_t>
{
  std::vector<std::size_t> sizes;
  for (const Located &block : layout.Blocks)
  {
    sizes.push_back(block.Bytes.size());
  }
  return sizes;
}
} // namespace

TEST_CASE("file/locator/layout: an R2000 file splits into its blocks in file order")
{
  std::vector<Issue> issues;
  const std::expected<Layout, Failure> layout = ReadLayout(ReadFixture("foundation-r2000.dwg"), issues);
  REQUIRE(layout);
  CHECK(layout->Head.Release == Version::R2000);
  CHECK(Names(*layout) == std::vector<std::string>{"auxiliary", "preview", "header", "classes", "pad", "objects", "map",
                                                   "second", "measurement"});
  CHECK(Offsets(*layout) == std::vector<std::size_t>{97, 220, 20207, 20695, 21723, 21727, 137708, 138416, 138565});
  CHECK(Sizes(*layout) == std::vector<std::size_t>{123, 19967, 450, 1010, 4, 115981, 708, 149, 4});
  CHECK(issues.empty());
}

TEST_CASE("file/locator/layout: an R14 file has no auxiliary header")
{
  std::vector<Issue> issues;
  const std::expected<Layout, Failure> layout = ReadLayout(ReadFixture("foundation-r14.dwg"), issues);
  REQUIRE(layout);
  CHECK(Names(*layout) ==
        std::vector<std::string>{"preview", "header", "classes", "pad", "objects", "map", "second", "measurement"});
  CHECK(Offsets(*layout) == std::vector<std::size_t>{88, 20075, 20451, 21570, 21574, 143340, 144346, 144492});
  CHECK(Sizes(*layout) == std::vector<std::size_t>{19967, 338, 1101, 4, 121766, 1006, 146, 4});
  CHECK(issues.empty());
}

TEST_CASE("file/locator/layout: bytes no block claims are kept as unknown and noted")
{
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  file.push_back(std::byte{0x00});
  file.push_back(std::byte{0x00});
  std::vector<Issue> issues;
  const std::expected<Layout, Failure> layout = ReadLayout(file, issues);
  REQUIRE(layout);
  CHECK(layout->Blocks.back().Name == "unknown");
  CHECK(layout->Blocks.back().Offset == 138569);
  CHECK(layout->Blocks.back().Bytes.size() == 2);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() == Issue{.Code = IssueCode::Damaged, .Offset = 138569, .What = "2 bytes in no block"});
}

TEST_CASE("file/locator/layout: a map past the end of the file is noted and the objects run to the end")
{
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  file.resize(60000);
  std::vector<Issue> issues;
  const std::expected<Layout, Failure> layout = ReadLayout(file, issues);
  REQUIRE(layout);
  CHECK(Names(*layout) == std::vector<std::string>{"auxiliary", "preview", "header", "classes", "objects"});
  CHECK(layout->Blocks.back().Offset == 21723);
  CHECK(layout->Blocks.back().Bytes.size() == 60000 - 21723);
  CHECK(std::ranges::any_of(
      issues, [](const Issue &one)
      { return one == Issue{.Code = IssueCode::Damaged, .Offset = 137708, .What = "map block beyond the file"}; }));
  CHECK(std::ranges::any_of(issues, [](const Issue &one) { return one.What == "measurement block beyond the file"; }));
}

TEST_CASE("file/locator/layout: a missing locator record fails")
{
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  file[0x19 + 9] = std::byte{7};
  std::vector<Issue> issues;
  const std::expected<Layout, Failure> layout = ReadLayout(file, issues);
  REQUIRE_FALSE(layout);
  CHECK(layout.error() == Failure{.Code = FailureCode::Damaged, .Offset = 0x15, .What = "locator record 1 missing"});
}

namespace
{
/**
 *  The R2000 fixture with one locator record's seeker or size changed.
 */
auto PatchLocator(std::uint8_t record, bool size, std::uint32_t value) -> std::vector<std::byte>
{
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  const std::size_t at = 0x19 + (9 * static_cast<std::size_t>(record)) + 1 + (size ? 4 : 0);
  for (std::size_t index = 0; index < 4; ++index)
  {
    file[at + index] = static_cast<std::byte>(value >> (8 * index));
  }
  return file;
}
} // namespace

TEST_CASE("file/locator/layout: a header or classes frame past the end of the file fails")
{
  std::vector<Issue> issues;
  CHECK(ReadLayout(PatchLocator(0, false, 200000), issues).error().What == "header section beyond the file");
  CHECK(ReadLayout(PatchLocator(1, false, 200000), issues).error().What == "classes section beyond the file");
}

TEST_CASE("file/locator/layout: overlapping blocks fail")
{
  std::vector<Issue> issues;
  const auto layout = ReadLayout(PatchLocator(1, false, 20187), issues);
  REQUIRE_FALSE(layout);
  CHECK(layout.error().What == "classes block overlaps the one before");
}

TEST_CASE("file/locator/layout: bytes between two blocks are kept as unknown and noted")
{
  std::vector<Issue> issues;
  const auto layout = ReadLayout(PatchLocator(5, true, 113), issues);
  REQUIRE(layout);
  CHECK(layout->Blocks[1].Name == "unknown");
  CHECK(layout->Blocks[1].Offset == 210);
  CHECK(layout->Blocks[1].Bytes.size() == 10);
  CHECK(std::ranges::any_of(
      issues, [](const Issue &one)
      { return one == Issue{.Code = IssueCode::Damaged, .Offset = 210, .What = "10 bytes in no block"}; }));
}

TEST_CASE("file/locator/layout: empty records and broken measurement or preview records are noted")
{
  std::vector<Issue> issues;
  const auto noAux = ReadLayout(PatchLocator(5, true, 0), issues);
  REQUIRE(noAux);
  CHECK(noAux->Blocks.front().Name == "unknown");
  const auto noMeasure = ReadLayout(PatchLocator(4, true, 0), issues);
  REQUIRE(noMeasure);
  CHECK(noMeasure->Blocks.back().Name == "unknown");
  const auto longMeasure = ReadLayout(PatchLocator(4, true, 0xFFFF), issues);
  REQUIRE(longMeasure);
  CHECK(std::ranges::any_of(issues, [](const Issue &one) { return one.What == "measurement block beyond the file"; }));
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  file[220] = std::byte{0};
  const auto noPreview = ReadLayout(file, issues);
  REQUIRE(noPreview);
  CHECK(std::ranges::any_of(issues, [](const Issue &one) { return one.What == "preview sentinel missing"; }));
  file[0x0D] = std::byte{0xFF};
  file[0x10] = std::byte{0xFF};
  CHECK(ReadLayout(file, issues));
  file[0x0D] = std::byte{0};
  file[0x0E] = std::byte{0};
  file[0x0F] = std::byte{0};
  file[0x10] = std::byte{0};
  const auto noImage = ReadLayout(file, issues);
  REQUIRE(noImage);
  CHECK(noImage->Blocks[1].Name == "unknown");
}

TEST_CASE("file/locator/layout: a file ending with its classes has no objects block")
{
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  file.resize(21723);
  std::vector<Issue> issues;
  const auto layout = ReadLayout(file, issues);
  REQUIRE(layout);
  CHECK(layout->Blocks.back().Name == "classes");
}

TEST_CASE("file/locator/layout: a file without a measurement record keeps those bytes as unknown")
{
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  file[0x19 + (9 * 4)] = std::byte{7};
  std::vector<Issue> issues;
  const auto layout = ReadLayout(file, issues);
  REQUIRE(layout);
  CHECK(layout->Blocks.back().Name == "unknown");
}

TEST_CASE("file/locator/layout: a preview inside a located block is noted and dropped")
{
  std::vector<Issue> issues;
  const auto layout = ReadLayout(PatchLocator(5, true, 130), issues);
  REQUIRE(layout);
  CHECK(Names(*layout) == std::vector<std::string>{"auxiliary", "unknown", "header", "classes", "pad", "objects", "map",
                                                   "second", "measurement"});
  CHECK(std::ranges::any_of(
      issues, [](const Issue &one)
      { return one == Issue{.Code = IssueCode::Damaged, .Offset = 220, .What = "preview block overlaps another"}; }));
  CHECK(std::ranges::any_of(
      issues, [](const Issue &one)
      { return one == Issue{.Code = IssueCode::Damaged, .Offset = 227, .What = "19960 bytes in no block"}; }));
}
} // namespace DwgIO::File::Locator
