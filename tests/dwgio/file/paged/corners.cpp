/***********************************************************************************************************************
 *  @file      tests/dwgio/file/paged/corners.cpp
 *  @brief     The paged file's hostile corners: maps that lead nowhere, pages cut short, sizes too large.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/head.hpp"
#include "file/paged/layout.hpp"
#include "file/paged/magic.hpp"
#include "file/paged/pages.hpp"
#include "support/fixtures.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <vector>

namespace DwgIO::File::Paged
{
namespace
{
/**
 *  The empty R2004 fixture with its section map replaced by one made of the descriptions given, in the page the map
 *  had; the new page must fit the old one's extent.
 */
auto ReplaceSectionMap(const std::vector<std::byte> &file, const Head &head,
                       const std::vector<Description> &descriptions) -> std::vector<std::byte>
{
  std::vector<Issue> issues;
  const Pager pager{file, issues};
  const std::vector<Page> pages = *pager.PageMap(head);
  const Page *page = FindPage(pages, head.Record.SectionMapId);
  REQUIRE(page != nullptr);
  const std::vector<std::byte> made = WriteSystemPage(SectionMapType, WriteSectionMap(descriptions));
  REQUIRE(made.size() <= page->Size);
  std::vector<std::byte> out = file;
  std::ranges::copy(made, out.begin() + static_cast<std::ptrdiff_t>(page->Address));
  return out;
}

auto ReplaceHead(const std::vector<std::byte> &file, const Head &head) -> std::vector<std::byte>
{
  std::vector<std::byte> out = file;
  const std::vector<std::byte> written = WriteHead(head);
  std::ranges::copy(written, out.begin());
  return out;
}
} // namespace

TEST_CASE("file/paged/corners: a section map naming a missing or late page or a pageless section is noted")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  const Head head = *ReadHead(file, issues);
  const Pager pager{file, issues};
  const std::vector<Page> pages = *pager.PageMap(head);
  const std::vector<Description> sections = *pager.SectionMap(pages, head);
  const auto header = std::ranges::find(sections, "AcDb:Header", &Description::Name);
  REQUIRE(header != sections.end());
  const auto index = static_cast<std::size_t>(header - sections.begin());
  std::vector<Description> missing = sections;
  missing[index].Pages[0].Number = 999;
  const std::expected<Layout, Failure> lost = ReadLayout(ReplaceSectionMap(file, head, missing), issues);
  REQUIRE_FALSE(lost);
  CHECK(lost.error().What == "section AcDb:Header page 999 missing");
  std::vector<Description> late = sections;
  late[index].Pages[0].Start = 100;
  late[index].Size += 100;
  const std::expected<Layout, Failure> padded = ReadLayout(ReplaceSectionMap(file, head, late), issues);
  REQUIRE(padded);
  const auto shifted = std::ranges::find(padded->Sections, "header", &Section::Name);
  REQUIRE(shifted != padded->Sections.end());
  CHECK(std::ranges::all_of(std::span{shifted->Bytes}.first(100), [](std::byte one) { return one == std::byte{0}; }));
  std::vector<Description> empty = sections;
  empty[index].Pages.clear();
  const std::expected<Layout, Failure> none = ReadLayout(ReplaceSectionMap(file, head, empty), issues);
  REQUIRE(none);
  CHECK(std::ranges::find(none->Sections, "header", &Section::Name)->Bytes.empty());
}

TEST_CASE("file/paged/corners: heads and maps that lead nowhere are refused")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  const Head head = *ReadHead(file, issues);
  const Pager pager{file, issues};
  const std::vector<Page> pages = *pager.PageMap(head);
  // A file of the head's length with no release in it.
  const std::vector<std::byte> letters(Pages, std::byte{'X'});
  CHECK_FALSE(ReadLayout(letters, issues));
  // A section map id no page bears.
  Head astray = head;
  astray.Record.SectionMapId = 999;
  CHECK(pager.SectionMap(pages, astray).error().What == "section map unreadable");
  CHECK(ReadLayout(ReplaceHead(file, astray), issues).error().What == "section map unreadable");
  // A page map address at the file's end, and one on a data page.
  Head ending = head;
  ending.Record.PageMapAddress = file.size() - Pages - 5;
  CHECK(pager.PageMap(ending).error().What == "page map unreadable");
  Head mistaken = head;
  mistaken.Record.PageMapAddress = FindPage(pages, head.Record.SectionMapId)->Address - Pages;
  const std::expected<std::vector<Page>, Failure> wrong = pager.PageMap(mistaken);
  REQUIRE_FALSE(wrong);
  CHECK(wrong.error().What == "page map unreadable");
  CHECK(wrong.error().Offset == FindPage(pages, head.Record.SectionMapId)->Address);
  // A page map whose compressed bytes decode to a literal run and then an opcode no family has.
  std::vector<std::byte> garbled = file;
  const auto at = static_cast<std::size_t>(head.Record.PageMapAddress) + Pages + SystemHeader;
  std::ranges::fill(std::span{garbled}.subspan(at, 40), std::byte{0x09});
  CHECK(Pager{garbled, issues}.PageMap(head).error().What == "page map unreadable");
  // A data page asked for beyond the file, and at a system page.
  const std::vector<Description> sections = *pager.SectionMap(pages, head);
  std::vector<std::byte> into;
  CHECK_FALSE(pager.DataPage(file.size() - 10, sections.back(), into));
  CHECK_FALSE(pager.DataPage(FindPage(pages, head.Record.SectionMapId)->Address, sections.back(), into));
}

TEST_CASE("file/paged/corners: a page map with a gap or a truncated section map is read as far as it goes")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  const Head head = *ReadHead(file, issues);
  const Pager pager{file, issues};
  const std::vector<Page> pages = *pager.PageMap(head);
  // A page map of one page, a gap of 16 more bytes, and another page.
  std::vector<std::byte> entries;
  const auto word = [&entries](std::uint32_t value) -> void
  {
    for (int at = 0; at < 4; ++at)
    {
      entries.push_back(static_cast<std::byte>((value >> (8 * at)) & 0xFFU));
    }
  };
  word(1);
  word(0x40);
  word(static_cast<std::uint32_t>(-1));
  word(0x20);
  entries.resize(entries.size() + 16, std::byte{0});
  word(2);
  word(0x60);
  const std::vector<std::byte> gapped = WriteSystemPage(PageMapType, entries);
  Head small = head;
  small.Record.PageMapAddress = 0x200 - Pages;
  std::vector<std::byte> crafted = WriteHead(small);
  crafted.resize(0x200, std::byte{0});
  crafted.insert(crafted.end(), gapped.begin(), gapped.end());
  const std::expected<std::vector<Page>, Failure> mapped = Pager{crafted, issues}.PageMap(small);
  REQUIRE(mapped);
  REQUIRE(mapped->size() == 3);
  CHECK((*mapped)[1].Number == -1);
  CHECK((*mapped)[2].Address == Pages + 0x60);
  // A section map that says five sections and holds half of one.
  std::vector<std::byte> cut(4 + 0x10, std::byte{0});
  cut[0] = std::byte{5};
  cut.resize(cut.size() + 0x18, std::byte{1});
  const Page *page = FindPage(pages, head.Record.SectionMapId);
  std::vector<std::byte> broken = file;
  const std::vector<std::byte> made = WriteSystemPage(SectionMapType, cut);
  REQUIRE(made.size() <= page->Size);
  std::ranges::copy(made, broken.begin() + static_cast<std::ptrdiff_t>(page->Address));
  CHECK(Pager{broken, issues}.SectionMap(pages, head).error().What == "section map unreadable");
}

TEST_CASE("file/paged/corners: a bad page header checksum or a cut page or a long or empty name is noted")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  const Head head = *ReadHead(file, issues);
  const Pager pager{file, issues};
  const std::vector<Page> pages = *pager.PageMap(head);
  const std::vector<Description> sections = *pager.SectionMap(pages, head);
  const auto header = std::ranges::find(sections, "AcDb:Header", &Description::Name);
  REQUIRE(header != sections.end());
  const Page *page = FindPage(pages, header->Pages[0].Number);
  REQUIRE(page != nullptr);
  // The header checksum word, masked, at offset 24 of the page header.
  std::vector<std::byte> flagged = file;
  flagged[page->Address + 24] ^= std::byte{1};
  std::vector<Issue> noted;
  std::vector<std::byte> into;
  CHECK(Pager{flagged, noted}.DataPage(page->Address, *header, into));
  CHECK(std::ranges::any_of(noted, [](const Issue &one) { return one.What.contains("checksum"); }));
  // The file ending inside the page's data, and inside a system page's.
  const std::vector<std::byte> cut(file.begin(),
                                   file.begin() + static_cast<std::ptrdiff_t>(page->Address + DataHeader + 5));
  CHECK_FALSE(Pager{cut, issues}.DataPage(page->Address, *header, into));
  const std::size_t map = static_cast<std::size_t>(head.Record.PageMapAddress) + Pages;
  const std::vector<std::byte> short_(file.begin(), file.begin() + static_cast<std::ptrdiff_t>(map + SystemHeader + 5));
  CHECK(Pager{short_, issues}.PageMap(head).error().What == "page map unreadable");
  // A name longer than its 64 bytes is cut to them; a nameless section with pages is kept under its empty name.
  std::vector<Description> named = sections;
  const auto index = static_cast<std::size_t>(header - sections.begin());
  named[index].Name = std::string(70, 'N');
  const std::expected<Layout, Failure> wide = ReadLayout(ReplaceSectionMap(file, head, named), issues);
  REQUIRE(wide);
  CHECK(std::ranges::any_of(wide->Sections, [](const Section &one)
                            { return one.Name.size() >= 63 && one.Name.size() <= 64 && one.Name.starts_with("NNN"); }));
  named[index].Name.clear();
  const std::expected<Layout, Failure> bare = ReadLayout(ReplaceSectionMap(file, head, named), issues);
  REQUIRE(bare);
  CHECK(std::ranges::any_of(bare->Sections, [](const Section &one) { return one.Name.empty() && !one.Bytes.empty(); }));
}

TEST_CASE("file/paged/corners: a section map whose start or page size or size the file cannot hold is refused")
{
  // A page's start, a page size and a section size come from the file; a hostile one must fail the section by
  // name, not grow a buffer to it.
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  const Head head = *ReadHead(file, issues);
  const Pager pager{file, issues};
  const std::vector<Page> pages = *pager.PageMap(head);
  const std::vector<Description> sections = *pager.SectionMap(pages, head);
  const auto header = std::ranges::find(sections, "AcDb:Header", &Description::Name);
  REQUIRE(header != sections.end());
  const auto index = static_cast<std::size_t>(header - sections.begin());
  std::vector<Description> far = sections;
  far[index].Pages[0].Start = 0x0000FFFFFFFFFFF0;
  CHECK(ReadLayout(ReplaceSectionMap(file, head, far), issues).error().What ==
        std::format("section AcDb:Header page {} starts beyond its size", header->Pages[0].Number));
  std::vector<Description> wide = sections;
  wide[index].PageSize = 0xFFFFFFFF;
  CHECK(ReadLayout(ReplaceSectionMap(file, head, wide), issues).error().What ==
        "section AcDb:Header page size 4294967295 beyond 0x100000");
  std::vector<Description> vast = sections;
  vast[index].Size = std::uint64_t{1} << 40U;
  CHECK(ReadLayout(ReplaceSectionMap(file, head, vast), issues).error().What ==
        "section AcDb:Header size 1099511627776 beyond its pages");
  // One page named again and again would grow the section by a whole page each time.
  std::vector<Description> repeated = sections;
  repeated[index].Pages.push_back(repeated[index].Pages.back());
  const std::expected<Layout, Failure> again = ReadLayout(ReplaceSectionMap(file, head, repeated), issues);
  REQUIRE_FALSE(again);
  CHECK(again.error().What == std::format("section AcDb:Header page {} repeated", header->Pages.back().Number));
}
} // namespace DwgIO::File::Paged
