/***********************************************************************************************************************
 *  @file      tests/dwgio/file/locator/preview.cpp
 *  @brief     The preview block: its directory and images, written back, and broken ones.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/preview.hpp"

#include "file/locator/sentinels.hpp"
#include "support/fixtures.hpp"

#include "dwgio/issue.hpp"
#include "dwgio/preview.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace DwgIO::File::Locator
{
namespace
{
constexpr std::size_t Offset = 220;

constexpr std::size_t Size = 19967;

auto Block() -> std::vector<std::byte>
{
  const std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  return {file.begin() + Offset, file.begin() + Offset + Size};
}
} // namespace

TEST_CASE("file/locator/preview: the fixture holds a header record and a bitmap")
{
  std::vector<Issue> issues;
  const std::vector<PreviewImage> images = ReadPreview(Block(), Offset, issues);
  REQUIRE(images.size() == 2);
  CHECK(images[0].Code == 1);
  CHECK(images[0].Bytes.size() == 80);
  CHECK(images[1].Code == 2);
  CHECK(images[1].Bytes.size() == 19832);
  CHECK(images[1].Bytes[0] == std::byte{40});
  CHECK(issues.empty());
}

TEST_CASE("file/locator/preview: the preview round-trips byte for byte")
{
  std::vector<Issue> issues;
  const std::vector<std::byte> block = Block();
  CHECK(WritePreview(ReadPreview(block, Offset, issues), Offset) == block);
  CHECK(WritePreview({}, Offset).size() == 37);
}

TEST_CASE("file/locator/preview: an image outside its block is dropped and noted")
{
  std::vector<std::byte> block = Block();
  block[16 + 4 + 1 + 4] = std::byte{0xFF};
  std::vector<Issue> issues;
  const std::vector<PreviewImage> images = ReadPreview(block, Offset, issues);
  CHECK(images.size() == 1);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() ==
        Issue{.Code = IssueCode::Damaged, .Offset = Offset + 21, .What = "preview image 0 outside the preview block"});
  block[0] = std::byte{0};
  issues.clear();
  CHECK(ReadPreview(block, Offset, issues).empty());
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() == Issue{.Code = IssueCode::Damaged, .Offset = Offset, .What = "preview sentinel mismatch"});
}

TEST_CASE("file/locator/preview: a small block or truncated directory or image before the directory is noted")
{
  std::vector<Issue> issues;
  CHECK(ReadPreview(std::vector<std::byte>(10), Offset, issues).empty());
  CHECK(issues.back().What == "preview sentinel mismatch");
  std::vector<std::byte> block = Block();
  block[16 + 4] = std::byte{5};
  issues.clear();
  CHECK(ReadPreview(block, Offset, issues).size() == 2);
  CHECK(issues.size() == 3);
  block = Block();
  block.resize(16 + 4 + 1 + 9 + 4);
  Put(block, Sentinels::Preview.End);
  issues.clear();
  CHECK(ReadPreview(block, Offset, issues).empty());
  CHECK(std::ranges::any_of(issues, [](const Issue &one) { return one.What == "preview directory cut short"; }));
  block = Block();
  for (std::size_t at = 22; at < 26; ++at)
  {
    block[at] = std::byte{0};
  }
  issues.clear();
  CHECK(ReadPreview(block, Offset, issues).size() == 1);
  CHECK(issues.front().What == "preview image 0 outside the preview block");
}

TEST_CASE("file/locator/preview: an image longer than what follows it is dropped")
{
  std::vector<std::byte> block = Block();
  for (std::size_t at = 26; at < 30; ++at)
  {
    block[at] = std::byte{0xFF};
  }
  std::vector<Issue> issues;
  CHECK(ReadPreview(block, Offset, issues).size() == 1);
  CHECK(issues.front().What == "preview image 0 outside the preview block");
}
} // namespace DwgIO::File::Locator
