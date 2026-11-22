/***********************************************************************************************************************
 *  @file      tests/dwgio/file/locator/head.cpp
 *  @brief     The locator file header of R14 and R2000: read, written back, and noted when wrong.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/head.hpp"

#include "support/fixtures.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <span>
#include <vector>

namespace DwgIO::File::Locator
{
TEST_CASE("file/locator/head: the R2000 file header reads")
{
  const std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  std::vector<Issue> issues;
  const std::expected<Head, Failure> head = ReadHead(file, issues);
  REQUIRE(head);
  CHECK(head->Release == Version::R2000);
  CHECK(head->Maintenance == 15);
  CHECK(head->Flag == 1);
  CHECK(head->Image == 220);
  CHECK(head->DwgVersion == 0x21);
  CHECK(head->MaintRelease == 255);
  CHECK(head->Codepage == 30);
  const std::vector<Record> records{
      {.Number = 0, .Seeker = 20187, .Size = 488},  {.Number = 1, .Seeker = 20675, .Size = 1048},
      {.Number = 2, .Seeker = 137708, .Size = 708}, {.Number = 3, .Seeker = 0, .Size = 0},
      {.Number = 4, .Seeker = 138565, .Size = 4},   {.Number = 5, .Seeker = 97, .Size = 123}};
  CHECK(head->Records == records);
  CHECK(SizeOf(*head) == 0x61);
  CHECK(issues.empty());
}

TEST_CASE("file/locator/head: the R14 file header has five records")
{
  const std::vector<std::byte> file = ReadFixture("foundation-r14.dwg");
  std::vector<Issue> issues;
  const std::expected<Head, Failure> head = ReadHead(file, issues);
  REQUIRE(head);
  CHECK(head->Release == Version::R14);
  CHECK(head->Maintenance == 10);
  CHECK(head->Image == 88);
  CHECK(head->Records.size() == 5);
  CHECK(SizeOf(*head) == 88);
  CHECK(issues.empty());
}

TEST_CASE("file/locator/head: the R2000 header round-trips byte for byte")
{
  const std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  std::vector<Issue> issues;
  const std::expected<Head, Failure> head = ReadHead(file, issues);
  REQUIRE(head);
  CHECK(WriteHead(*head) == std::vector<std::byte>{file.begin(), file.begin() + 0x61});
}

TEST_CASE("file/locator/head: a wrong CRC or sentinel is noted and the header read")
{
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  file[0x4F] ^= std::byte{0x01};
  std::vector<Issue> issues;
  REQUIRE(ReadHead(file, issues));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() == Issue{.Code = IssueCode::Checksum, .Offset = 0x4F, .What = "file header CRC mismatch"});
  file[0x4F] ^= std::byte{0x01};
  file[0x55] ^= std::byte{0x01};
  issues.clear();
  REQUIRE(ReadHead(file, issues));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() == Issue{.Code = IssueCode::Damaged, .Offset = 0x51, .What = "file header sentinel mismatch"});
}

TEST_CASE("file/locator/head: non-DWG or truncated bytes or an unsupported release fail")
{
  std::vector<Issue> issues;
  const std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  CHECK(ReadHead(std::span{file}.first(0x18), issues).error() ==
        Failure{.Code = FailureCode::NotDwg, .What = "file too short"});
  std::vector<std::byte> other = file;
  other[0] = std::byte{'X'};
  CHECK(ReadHead(other, issues).error() == Failure{.Code = FailureCode::NotDwg, .What = "not a DWG file"});
  CHECK(ReadHead(ReadFixture("foundation-r2004.dwg"), issues).error() ==
        Failure{.Code = FailureCode::UnsupportedRelease, .What = "release R2004 is not a locator file"});
  std::vector<std::byte> none = file;
  none[0x15] = std::byte{0};
  CHECK(ReadHead(none, issues).error() ==
        Failure{.Code = FailureCode::Damaged, .Offset = 0x15, .What = "no locator records"});
  std::vector<std::byte> many = file;
  many[0x18] = std::byte{0x01};
  CHECK(ReadHead(many, issues).error() ==
        Failure{.Code = FailureCode::Damaged, .Offset = 0x15, .What = "locator records run past the end"});
  CHECK(issues.empty());
}

TEST_CASE("file/locator/head: non-zero bytes 6 to 10 are noted")
{
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  file[6] = std::byte{1};
  std::vector<Issue> issues;
  REQUIRE(ReadHead(file, issues));
  CHECK(std::ranges::any_of(issues, [](const Issue &one) { return one.What == "file header bytes 6 to 10 not zero"; }));
}
} // namespace DwgIO::File::Locator
