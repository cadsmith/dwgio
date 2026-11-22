/***********************************************************************************************************************
 *  @file      tests/dwgio/file/paged/pages.cpp
 *  @brief     The page map, the section map and a data page of the empty R2004 fixture.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/pages.hpp"

#include "file/paged/head.hpp"
#include "support/fixtures.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

namespace DwgIO::File::Paged
{
TEST_CASE("file/paged/pages: the empty R2004 fixture's page map and section map and data page read")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  const Head head = *ReadHead(file, issues);
  const Pager pager{file, issues};
  const std::expected<std::vector<Page>, Failure> pages = pager.PageMap(head);
  REQUIRE(pages);
  REQUIRE(pages->size() == 19);
  CHECK(pages->front() == Page{.Number = 1, .Address = 0x100, .Size = 160});
  CHECK(pages->back() == Page{.Number = 21, .Address = 55904, .Size = 1536});
  CHECK((*pages)[5] == Page{.Number = 6, .Address = 0x5BC0, .Size = 8864});
  const std::expected<std::vector<Description>, Failure> sections = pager.SectionMap(*pages, head);
  REQUIRE(sections);
  REQUIRE(sections->size() == 13);
  CHECK(sections->front().Name.empty());
  CHECK(sections->front().Id == 0);
  CHECK(sections->back().Name == "AcDb:Header");
  CHECK(sections->back().Id == 1);
  CHECK(sections->back().Size == 580);
  CHECK(sections->back().Compressed == 2);
  CHECK(sections->back().PageSize == 0x7400);
  REQUIRE(sections->back().Pages.size() == 1);
  CHECK(sections->back().Pages[0].Number == 17);
  CHECK(sections->back().Pages[0].Size == 646);
  const Description &objects = (*sections)[6];
  CHECK(objects.Name == "AcDb:AcDbObjects");
  CHECK(objects.Size == 154727);
  REQUIRE(objects.Pages.size() == 6);
  CHECK(objects.Pages[2].Start == 59392);
  const Description &summary = (*sections)[4];
  CHECK(summary.Name == "AcDb:SummaryInfo");
  CHECK(summary.Compressed == 1);
  CHECK(summary.PageSize == 0x80);
  CHECK(summary.Size == 72);
  std::vector<std::byte> into;
  for (const SectionPage &page : objects.Pages)
  {
    const Page &where = *std::ranges::find(*pages, static_cast<std::int32_t>(page.Number), &Page::Number);
    REQUIRE(pager.DataPage(where.Address, objects, into));
  }
  CHECK(into.size() == 6 * 0x7400);
  CHECK(std::ranges::equal(std::span{into}.first(4),
                           std::array{std::byte{0xCA}, std::byte{0x0D}, std::byte{0}, std::byte{0}}));
  std::vector<std::byte> stored;
  REQUIRE(pager.DataPage(0x100, summary, stored));
  CHECK(stored.size() == 0x80);
  CHECK(stored[0] == std::byte{1});
  CHECK(issues.empty());
  std::vector<std::byte> dented = file;
  dented[0x5BC0 + 40] ^= std::byte{1};
  std::vector<std::byte> other;
  const Pager hurt{dented, issues};
  CHECK(hurt.DataPage(0x5BC0, objects, other));
  REQUIRE(issues.size() == 1);
  CHECK(issues[0].What == "page at 0x5BC0 checksum mismatch");
  CHECK(issues[0].Offset == 0x5BC0);
}

TEST_CASE("file/paged/pages: a page that will not decompress or has a wrong checksum is refused or noted")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  const Head head = *ReadHead(file, issues);
  const Pager pager{file, issues};
  const std::vector<Page> pages = *pager.PageMap(head);
  const std::vector<Description> sections = *pager.SectionMap(pages, head);
  std::vector<std::byte> garbage = file;
  std::ranges::fill(std::span{garbage}.subspan(0x5BC0 + 0x20 + 8, 40), std::byte{0x09});
  std::vector<std::byte> into;
  CHECK_FALSE(Pager{garbage, issues}.DataPage(0x5BC0, sections[6], into));
  std::vector<std::byte> flagged = file;
  flagged[0xDA60 + 0x10] ^= std::byte{1};
  std::vector<Issue> again;
  CHECK(Pager{flagged, again}.PageMap(head));
  REQUIRE(again.size() == 1);
  CHECK(again[0].What == "page at 0xDA60 checksum mismatch");
  std::vector<std::byte> cut(file.begin(), file.begin() + 0xDA60 + 10);
  CHECK(Pager{cut, again}.PageMap(head).error().What == "page map unreadable");
}
} // namespace DwgIO::File::Paged
