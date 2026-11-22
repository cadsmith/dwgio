/***********************************************************************************************************************
 *  @file      tests/dwgio/file/paged/head.cpp
 *  @brief     The paged file's encrypted header, its checks and its magic sequence.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/head.hpp"

#include "file/paged/magic.hpp"
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

namespace DwgIO::File::Paged
{
TEST_CASE("file/paged/head: the empty R2004 fixture's header decrypts and checks")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  const std::expected<Head, Failure> head = ReadHead(file, issues);
  REQUIRE(head);
  CHECK(issues.empty());
  CHECK(head->Release == Version::R2004);
  CHECK(head->Maintenance == 104);
  CHECK(head->Flag == 3);
  CHECK(head->Image == 448);
  CHECK(head->Summary == 288);
  CHECK(head->Codepage == 30);
  CHECK(head->DwgVersion == 0x21);
  CHECK(head->MaintRelease == 0xFF);
  CHECK(head->Record.PageMapId == 21);
  CHECK(head->Record.SectionMapId == 20);
  CHECK(head->Record.PageCount == 19);
  CHECK(head->Record.LastPageId == 21);
  CHECK(head->Record.PageArraySize == 21);
  CHECK(head->Record.PageMapAddress + 0x100 == 0xDA60);
  CHECK(head->Record.SecondHeader == 56081);
  CHECK(head->Record.Crc == 0xDE4F03BD);
  CHECK(head->Extra[0] == std::byte{0xE0});
  CHECK(head->Extra[1] == std::byte{0x51});
  const std::vector<std::byte> written = WriteHead(*head);
  REQUIRE(written.size() == 0x100);
  const auto differs = std::ranges::mismatch(written, std::span{file}.first(0x100));
  CHECK(differs.in1 - written.begin() == 0x100);
  CHECK(std::ranges::equal(WriteCopy(*head), std::span{file}.subspan(56081, 0x6C)));
}

TEST_CASE("file/paged/head: bad header fields are refused or noted")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  CHECK(ReadHead(std::span{file}.first(0x80), issues).error().What == "paged header unreadable");
  std::vector<std::byte> secured = file;
  secured[0x18] = std::byte{1};
  CHECK(ReadHead(secured, issues).error().What == "encrypted drawing not read");
  std::vector<std::byte> broken = file;
  broken[0x80] ^= std::byte{0xFF};
  CHECK(ReadHead(broken, issues).error().What == "paged header unreadable");
  CHECK(issues.empty());
  std::vector<std::byte> dented = file;
  dented[0x80 + 0x18] ^= std::byte{1}; // The root gap, which nothing else reads.
  REQUIRE(ReadHead(dented, issues));
  REQUIRE(issues.size() == 2);
  CHECK(issues[0].What == "header CRC mismatch");
  CHECK(issues[1].What == "second header copy differs");
  CHECK(issues[0].Offset == 0x80);
  // A copy the file ends before is a copy that differs.
  std::vector<std::byte> cut(file.begin(), file.begin() + 56081 + 10);
  std::vector<Issue> more;
  REQUIRE(ReadHead(cut, more));
  REQUIRE(more.size() == 1);
  CHECK(more[0].What == "second header copy differs");
}

TEST_CASE("file/paged/head: the magic sequence matches")
{
  const std::vector<std::byte> magic = MakeMagic(0x100);
  CHECK(magic[0] == std::byte{0x29});
  CHECK(magic[1] == std::byte{0x23});
  // The text prints B7 as the last byte of the record's 0x6c; the generator it gives, which the files' CRCs
  // confirm, gives 02.
  CHECK(magic[0x6B] == std::byte{0x02});
  CHECK(magic[0x6A] == std::byte{0x99});
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  CHECK(std::ranges::equal(std::span{magic}.subspan(0xEC, 0x14), std::span{file}.subspan(0xEC, 0x14)));
}

TEST_CASE("file/paged/head: a long enough file that names no release is unreadable")
{
  const std::vector<std::byte> letters(Pages, std::byte{'X'});
  std::vector<Issue> issues;
  CHECK_FALSE(ReadHead(letters, issues));
}

TEST_CASE("file/paged/head: a second header address near the top of the range reads as a differing copy")
{
  Head head;
  head.Release = Version::R2004;
  head.Record.SecondHeader = 0xFFFFFFFFFFFFFFF0;
  std::vector<std::byte> file = WriteHead(head);
  file.resize(0x400, std::byte{0});
  std::vector<Issue> issues;
  REQUIRE(ReadHead(file, issues));
  CHECK(std::ranges::any_of(issues, [](const Issue &one) { return one.What == "second header copy differs"; }));
}
} // namespace DwgIO::File::Paged
