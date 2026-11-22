/***********************************************************************************************************************
 *  @file      tests/dwgio/file/locator/section.cpp
 *  @brief     A sectioned block's body between its sentinels and its CRC.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/section.hpp"

#include "file/locator/head.hpp"
#include "file/locator/sentinels.hpp"
#include "support/fixtures.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <span>
#include <vector>

namespace DwgIO::File::Locator
{
namespace
{
constexpr Record Variables{.Number = 0, .Seeker = 20187, .Size = 488};

constexpr Record Classes{.Number = 1, .Seeker = 20675, .Size = 1048};
} // namespace

TEST_CASE("file/locator/section: a sectioned block is its body between the sentinels")
{
  const std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  const std::expected<Framed, Failure> header = Cut(file, Variables, Sentinels::Header);
  REQUIRE(header);
  CHECK(header->Body.size() == 450);
  CHECK(header->Issues.empty());
  const std::expected<Framed, Failure> classes = Cut(file, Classes, Sentinels::Classes);
  REQUIRE(classes);
  CHECK(classes->Body.size() == 1010);
  CHECK(classes->Issues.empty());
}

TEST_CASE("file/locator/section: wrapping a body again gives the original block")
{
  const std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  const std::expected<Framed, Failure> header = Cut(file, Variables, Sentinels::Header);
  REQUIRE(header);
  const std::vector<std::byte> block{file.begin() + Variables.Seeker, file.begin() + Variables.Seeker + Variables.Size};
  CHECK(Wrap(header->Body, Sentinels::Header) == block);
}

TEST_CASE("file/locator/section: a wrong sentinel or CRC is noted and the body read")
{
  std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  file[Variables.Seeker] ^= std::byte{0x01};
  file[Variables.Seeker + 20 + 450] ^= std::byte{0x01};
  file[Variables.Seeker + Variables.Size - 1] ^= std::byte{0x01};
  const std::expected<Framed, Failure> header = Cut(file, Variables, Sentinels::Header);
  REQUIRE(header);
  CHECK(header->Body.size() == 450);
  const std::vector<Issue> issues{
      Issue{.Code = IssueCode::Damaged, .Offset = 20187, .What = "header section begin sentinel mismatch"},
      Issue{.Code = IssueCode::Checksum, .Offset = 20187 + 20 + 450, .What = "header section CRC mismatch"},
      Issue{.Code = IssueCode::Damaged, .Offset = 20187 + 22 + 450, .What = "header section end sentinel mismatch"},
  };
  CHECK(header->Issues == issues);
}

TEST_CASE("file/locator/section: a block past the end of the file fails")
{
  const std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  CHECK(Cut(file, Record{.Number = 0, .Seeker = 200000, .Size = 488}, Sentinels::Header).error() ==
        Failure{.Code = FailureCode::Damaged, .Offset = 200000, .What = "header section beyond the file"});
  std::vector<std::byte> grown = file;
  grown[Variables.Seeker + 18] = std::byte{0x7F};
  CHECK(Cut(grown, Variables, Sentinels::Header).error() ==
        Failure{.Code = FailureCode::Damaged, .Offset = 20187 + 16, .What = "header section runs past the end"});
}
} // namespace DwgIO::File::Locator
