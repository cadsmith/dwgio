/***********************************************************************************************************************
 *  @file      tests/dwgio/file/locator/second.cpp
 *  @brief     The second file header of R14 and R2000: read, written back, and noted when wrong.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/second.hpp"

#include "file/locator/head.hpp"
#include "file/locator/sentinels.hpp"
#include "support/fixtures.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace DwgIO::File::Locator
{
namespace
{
auto Block(std::string_view fixture, std::size_t offset, std::size_t size) -> std::vector<std::byte>
{
  const std::vector<std::byte> file = ReadFixture(fixture);
  return {file.begin() + static_cast<std::ptrdiff_t>(offset),
          file.begin() + static_cast<std::ptrdiff_t>(offset + size)};
}
} // namespace

TEST_CASE("file/locator/second: the R2000 fixture's second header reads")
{
  std::vector<Issue> issues;
  const std::optional<Second> second = ReadSecond(Block("foundation-r2000.dwg", 138416, 149), 138416, issues);
  REQUIRE(second);
  CHECK(second->Location == 138416);
  CHECK(second->Version == "AC1015");
  CHECK(second->Constants.size() == 13);
  REQUIRE(second->Records.size() == 6);
  CHECK(second->Records[0] == Record{.Number = 0, .Seeker = 20187, .Size = 488});
  CHECK(second->Records[2] == Record{.Number = 2, .Seeker = 137708, .Size = 708});
  CHECK(second->Records[5] == Record{.Number = 5, .Seeker = 97, .Size = 123});
  REQUIRE(second->Handles.size() == 14);
  CHECK(second->Handles[0] == Named{.Number = 0, .Value = Handle{0x417}});
  CHECK(second->Handles[1] == Named{.Number = 1, .Value = Handle{0x1}});
  CHECK(second->Handles[10] == Named{.Number = 10, .Value = Handle{0x2C9}});
  CHECK(second->Handles[13] == Named{.Number = 13, .Value = Handle{0xD}});
  CHECK(second->Junk.size() == 8);
  CHECK(issues.empty());
}

TEST_CASE("file/locator/second: the second header round-trips byte for byte for R2000 and R14")
{
  std::vector<Issue> issues;
  const std::vector<std::byte> newer = Block("foundation-r2000.dwg", 138416, 149);
  const std::optional<Second> second = ReadSecond(newer, 138416, issues);
  REQUIRE(second);
  CHECK(WriteSecond(*second) == newer);
  const std::vector<std::byte> older = Block("foundation-r14.dwg", 144346, 146);
  const std::optional<Second> old = ReadSecond(older, 144346, issues);
  REQUIRE(old);
  CHECK(old->Version == "AC1014");
  CHECK(old->Records.size() == 5);
  CHECK(old->Handles.size() == 14);
  CHECK(WriteSecond(*old) == older);
  CHECK(issues.empty());
}

TEST_CASE("file/locator/second: a wrong sentinel gives nothing and a wrong CRC is noted")
{
  std::vector<std::byte> block = Block("foundation-r2000.dwg", 138416, 149);
  std::vector<Issue> issues;
  block[16 + 107] ^= std::byte{0x01};
  CHECK(ReadSecond(block, 138416, issues));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() ==
        Issue{.Code = IssueCode::Checksum, .Offset = 138416 + 16 + 107, .What = "second header CRC mismatch"});
  block[3] = std::byte{0xFF};
  issues.clear();
  CHECK_FALSE(ReadSecond(block, 138416, issues));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() ==
        Issue{.Code = IssueCode::Damaged, .Offset = 138416, .What = "second header sentinel mismatch"});
  block[3] = std::byte{0xCE};
  block.resize(40);
  issues.clear();
  CHECK_FALSE(ReadSecond(block, 138416, issues));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() ==
        Issue{.Code = IssueCode::Truncated, .Offset = 138416 + 16, .What = "second header cut short"});
  block.resize(20);
  issues.clear();
  CHECK_FALSE(ReadSecond(block, 138416, issues));
  CHECK(issues.front() == Issue{.Code = IssueCode::Truncated, .Offset = 138416, .What = "second header cut short"});
}

TEST_CASE("file/locator/second: a null handle takes one byte and a short version and constants are filled in")
{
  std::vector<Issue> issues;
  std::optional<Second> second = ReadSecond(Block("foundation-r2000.dwg", 138416, 149), 138416, issues);
  REQUIRE(second);
  second->Handles.front().Value = Handle{};
  second->Version = "AC10";
  second->Constants.resize(2);
  const std::vector<std::byte> bytes = WriteSecond(*second);
  const std::optional<Second> back = ReadSecond(bytes, 0, issues);
  REQUIRE(back);
  CHECK(back->Handles.front() == Named{.Number = 0, .Value = Handle{}});
  CHECK(back->Version == std::string("AC10\0\0", 6));
  CHECK(back->Constants.size() == 13);
  CHECK(issues.empty());
}

TEST_CASE("file/locator/second: a small size or a truncated body or a wrong end sentinel is refused or noted")
{
  std::vector<Issue> issues;
  std::vector<std::byte> block = Block("foundation-r2000.dwg", 138416, 149);
  block[16] = std::byte{2};
  CHECK_FALSE(ReadSecond(block, 138416, issues));
  CHECK(issues.back().What == "second header cut short");
  block = Block("foundation-r2000.dwg", 138416, 149);
  block[16] = std::byte{40};
  block.resize(16 + 40);
  Put(block, Sentinels::Second.End);
  issues.clear();
  CHECK_FALSE(ReadSecond(block, 138416, issues));
  CHECK(issues.back().What == "second header cut short");
  block = Block("foundation-r2000.dwg", 138416, 149);
  block.back() = std::byte{0};
  issues.clear();
  CHECK(ReadSecond(block, 138416, issues));
  CHECK(issues.back().What == "second header end sentinel mismatch");
}

TEST_CASE("file/locator/second: a body cut inside its handles fails")
{
  std::vector<std::byte> block = Block("foundation-r2000.dwg", 138416, 149);
  block[16] = std::byte{70};
  block.resize(16 + 70);
  Put(block, Sentinels::Second.End);
  std::vector<Issue> issues;
  CHECK_FALSE(ReadSecond(block, 138416, issues));
  CHECK(issues.back().What == "second header cut short");
}
} // namespace DwgIO::File::Locator
