/***********************************************************************************************************************
 *  @file      tests/dwgio/file/paged/write.cpp
 *  @brief     Sections written into pages as AutoCAD writes them, and read back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/lz.hpp"
#include "file/paged/layout.hpp"
#include "file/paged/pages.hpp"
#include "support/fixtures.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <format>
#include <string_view>
#include <vector>

namespace DwgIO::File::Paged
{
TEST_CASE("file/paged/write: the fixture's sections written again read back the same")
{
  for (const std::string_view name : {"empty", "entities", "curves"})
  {
    CAPTURE(name);
    std::vector<Issue> issues;
    const Layout layout = *ReadLayout(ReadFixture(std::format("{}-r2004.dwg", name)), issues);
    Head head = layout.Head;
    const std::vector<std::byte> file = WriteLayout(head, layout.Sections);
    const std::expected<Layout, Failure> back = ReadLayout(file, issues);
    REQUIRE(back);
    CHECK(issues.empty());
    REQUIRE(back->Sections.size() == layout.Sections.size());
    for (std::size_t at = 0; at < layout.Sections.size(); ++at)
    {
      CAPTURE(layout.Sections[at].Name);
      CHECK(back->Sections[at].Name == layout.Sections[at].Name);
      CHECK(back->Sections[at].Bytes == layout.Sections[at].Bytes);
      CHECK(back->Sections[at].Compressed == layout.Sections[at].Compressed);
      CHECK(back->Sections[at].PageSize == layout.Sections[at].PageSize);
      CHECK(back->Sections[at].Id == layout.Sections[at].Id);
    }
    CHECK(back->Head.Release == Version::R2004);
    CHECK(back->Head.Maintenance == layout.Head.Maintenance);
    CHECK(back->Head.Codepage == layout.Head.Codepage);
    CHECK(back->Head.Extra == layout.Head.Extra);
    CHECK(back->Head.Summary == 0x120);
    CHECK(back->Head.Image == 0x100 + 0xA0 + 0x20);
    CHECK(back->Head.Record.PageMapId == back->Head.Record.LastPageId);
    CHECK(back->Head.Record.SectionMapId + 1 == back->Head.Record.PageMapId);
    CHECK(back->Head.Record.PageCount == layout.Head.Record.PageCount);
    CHECK(back->Head.Record.PageArraySize == back->Head.Record.PageMapId);
    CHECK(back->Head.Record.GapCount == 0);
    CHECK(file.size() == back->Head.Record.SecondHeader + RecordSize);
    CHECK(file.size() == back->Head.Record.LastPageEnd + RecordSize);
    CHECK(file.size() % Alignment == RecordSize % Alignment);
  }
}

TEST_CASE("file/paged/write: a section spans several pages of the right sizes")
{
  std::vector<std::byte> big((3 * 0x7400) + 5);
  for (std::size_t at = 0; at < big.size(); ++at)
  {
    big[at] = static_cast<std::byte>(at * 7 % 251);
  }
  const std::vector<Section> sections{
      Section{.Name = "AcDb:SummaryInfo",
              .Bytes = std::vector<std::byte>(10, std::byte{1}),
              .Compressed = false,
              .PageSize = 0x80,
              .Id = 9},
      Section{.Name = "objects", .Bytes = big, .Compressed = true, .PageSize = 0x7400, .Id = 7},
      Section{.Name = "header",
              .Bytes = std::vector<std::byte>(600, std::byte{2}),
              .Compressed = true,
              .PageSize = 0x7400,
              .Id = 1}};
  Head head;
  head.Release = Version::R2004;
  std::vector<Issue> issues;
  const std::vector<std::byte> file = WriteLayout(head, sections);
  const std::expected<Layout, Failure> back = ReadLayout(file, issues);
  REQUIRE(back);
  CHECK(issues.empty());
  REQUIRE(back->Sections.size() == 3);
  CHECK(back->Sections[0].Bytes == sections[0].Bytes);
  CHECK(back->Sections[1].Bytes == big);
  CHECK(back->Sections[2].Bytes == sections[2].Bytes);
  CHECK(back->Head.Record.PageCount == 8);
  CHECK(back->Head.Summary == 0x120);
  CHECK(back->Head.Image == 0);
  CHECK(back->Head.Record.PageMapId == 10);
  CHECK(back->Head.Record.SectionMapId == 9);
  CHECK(file.size() < big.size());
  // The stored page holds its page size; a stored section longer than its page size takes a multiple of it.
  std::vector<Issue> more;
  const Pager pager{file, more};
  const std::vector<Page> pages = *pager.PageMap(back->Head);
  CHECK(pages.front().Size == 0x80 + DataHeader);
  const std::vector<Section> wide{Section{.Name = "AcDb:AppInfo",
                                          .Bytes = std::vector<std::byte>(0x301, std::byte{3}),
                                          .Compressed = false,
                                          .PageSize = 0x300,
                                          .Id = 11}};
  Head again;
  again.Release = Version::R2004;
  const std::expected<Layout, Failure> grown = ReadLayout(WriteLayout(again, wide), more);
  REQUIRE(grown);
  CHECK(grown->Sections[0].Bytes == wide[0].Bytes);
  CHECK(grown->Sections[0].PageSize == 0x600);
}

TEST_CASE("file/paged/write: a compressed page fills its page size and the map lists sections by descending id")
{
  // AutoCAD compresses every page whole, zeros to the page size after the section's last byte, and refuses a page
  // that decompresses short; its section map runs from the highest id down after the nameless section.
  const std::vector<Section> sections{
      Section{.Name = "header",
              .Bytes = std::vector<std::byte>(600, std::byte{2}),
              .Compressed = true,
              .PageSize = 0x7400,
              .Id = 1},
      Section{.Name = "AcDb:Template",
              .Bytes = std::vector<std::byte>{std::byte{0}, std::byte{0}, std::byte{1}, std::byte{0}},
              .Compressed = true,
              .PageSize = 0x7400,
              .Id = 5},
      Section{.Name = "objects",
              .Bytes = std::vector<std::byte>(20, std::byte{3}),
              .Compressed = true,
              .PageSize = 0x7400,
              .Id = 7}};
  Head head;
  head.Release = Version::R2004;
  const std::vector<std::byte> file = WriteLayout(head, sections);
  std::vector<Issue> issues;
  const Pager pager{file, issues};
  const std::vector<Page> pages = *pager.PageMap(head);
  const std::vector<Description> map = *pager.SectionMap(pages, head);
  REQUIRE(map.size() == 4);
  CHECK(map[0].Id == 0);
  CHECK(map[1].Id == 7);
  CHECK(map[2].Id == 5);
  CHECK(map[3].Id == 1);
  REQUIRE(map[2].Pages.size() == 1);
  const Page *page = FindPage(pages, map[2].Pages[0].Number);
  REQUIRE(page != nullptr);
  std::vector<std::byte> out;
  REQUIRE(Bits::Lz::Decompress(std::span{file}.subspan(page->Address + DataHeader, map[2].Pages[0].Size), 0x7400, out));
  CHECK(out.size() == 0x7400);
  CHECK(issues.empty());
}
} // namespace DwgIO::File::Paged
