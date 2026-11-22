/***********************************************************************************************************************
 *  @file      tests/dwgio/file/paged/layout.cpp
 *  @brief     A paged file cut into its sections, and the pages that fail them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/layout.hpp"

#include "file/locator/sentinels.hpp"
#include "objects/map.hpp"
#include "objects/walk.hpp"
#include "support/fixtures.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::File::Paged
{
namespace
{
auto Names(const Layout &layout) -> std::vector<std::string>
{
  std::vector<std::string> names;
  for (const Section &section : layout.Sections)
  {
    names.push_back(section.Name);
  }
  return names;
}

auto HasMark(std::span<const std::byte> bytes, const Locator::Mark &mark) -> bool
{
  return bytes.size() == mark.size() &&
         std::ranges::equal(bytes, mark, [](std::byte one, std::uint8_t other) { return one == std::byte{other}; });
}
} // namespace

TEST_CASE("file/paged/layout: every R2004 fixture splits into its sections")
{
  for (const std::string_view name : {"foundation", "tables", "entities", "curves", "objects", "empty"})
  {
    CAPTURE(name);
    std::vector<Issue> issues;
    const std::expected<Layout, Failure> layout = ReadLayout(ReadFixture(std::format("{}-r2004.dwg", name)), issues);
    REQUIRE(layout);
    CHECK(issues.empty());
    CHECK(layout->Head.Release == Version::R2004);
    CHECK(Names(*layout) == std::vector<std::string>{"AcDb:SummaryInfo", "preview", "AcDb:AppInfo",
                                                     "AcDb:AppInfoHistory", "AcDb:RevHistory", "objects",
                                                     "AcDb:ObjFreeSpace", "measurement", "map", "classes", "auxiliary",
                                                     "header"});
    const Section &header = layout->Sections.back();
    CHECK(header.Id == 1);
    CHECK(header.Compressed);
    CHECK(header.PageSize == 0x7400);
    CHECK(HasMark(std::span{header.Bytes}.first(16), Locator::Sentinels::Header.Begin));
    CHECK(HasMark(std::span{header.Bytes}.last(16), Locator::Sentinels::Header.End));
    const Section &measurement = layout->Sections[7];
    CHECK(measurement.Id == 5);
    CHECK(measurement.Bytes == std::vector<std::byte>{std::byte{0}, std::byte{0}, std::byte{1}, std::byte{0}});
    const Section &summary = layout->Sections[0];
    CHECK(summary.Id == 9);
    CHECK(summary.Bytes.size() == 72);
    CHECK_FALSE(summary.Compressed);
    CHECK(summary.PageSize == 0x80);
    const Section &preview = layout->Sections[1];
    CHECK(preview.Id == 10);
    CHECK(preview.Bytes.size() == 19967);
    CHECK(preview.PageSize == 0x5000);
    CHECK(HasMark(std::span{preview.Bytes}.first(16), Locator::Sentinels::Preview.Begin));
  }
}

TEST_CASE("file/paged/layout: the empty fixture's objects and map agree")
{
  std::vector<Issue> issues;
  const Layout layout = *ReadLayout(ReadFixture("empty-r2004.dwg"), issues);
  const Section &objects = layout.Sections[5];
  const Section &map = layout.Sections[8];
  CHECK(objects.Name == "objects");
  CHECK(map.Name == "map");
  CHECK(objects.Bytes.size() == 154727);
  const std::vector<Objects::Mapped> entries = Objects::ReadMap(map.Bytes, 0, issues);
  CHECK(entries.size() == 207);
  CHECK(entries.front() == Objects::Mapped{.Self = Handle{1}, .Offset = 4});
  CHECK(Objects::Walk(objects.Bytes, 4, objects.Bytes.size()).size() == 207);
  CHECK(issues.empty());
}

TEST_CASE("file/paged/layout: a truncated file or undecompressable page or unmapped page fails by name")
{
  const std::vector<std::byte> file = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  const std::vector<std::byte> cut(file.begin(), file.begin() + 0x5BC0 + 100);
  CHECK(ReadLayout(cut, issues).error().What == "page map unreadable");
  std::vector<std::byte> garbage = file;
  std::ranges::fill(std::span{garbage}.subspan(0x5BC0 + 0x20 + 8, 40), std::byte{0x09});
  const std::expected<Layout, Failure> broken = ReadLayout(garbage, issues);
  REQUIRE_FALSE(broken);
  CHECK(broken.error().What == "section AcDb:AcDbObjects page 6 unreadable");
  CHECK(broken.error().Offset == 0x5BC0);
  // The page map's last entry names the page map itself; a section's page beyond the file's end is missing.
  std::vector<std::byte> shorter(file.begin(), file.begin() + 0xDA60 + 0x14 + 137 + 0x14);
  std::vector<Issue> more;
  const std::expected<Layout, Failure> lost = ReadLayout(shorter, more);
  REQUIRE(lost); // Every section's page lies before the page map; only the header copy is gone.
  CHECK(more.size() == 1);
  CHECK(more[0].What == "second header copy differs");
  CHECK(MakeMissingPage(17).What == "page 17 missing");
}
} // namespace DwgIO::File::Paged
