/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/tablestyle.cpp
 *  @brief     The TABLESTYLE against its DXF twin, across R2010, and its flags as AutoCAD saves them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/shapes/table/cellstyle.hpp"
#include "objects/shapes/table/style.hpp"
#include "objects/write.hpp"
#include "releases/object.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/class.hpp"
#include "dwgio/color.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/tablestyles/cellstyle.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
using TableStyles::Border;
using TableStyles::RowStyle;

/**
 *  The fixtures with a table, each release's, and the twin each is compared with.
 */
auto TwinOf(std::string_view fixture, std::string_view release) -> std::string
{
  return std::format("{}-{}.dxf", fixture, release == "r14" ? "r2000" : release);
}

/**
 *  The style of a loaded drawing, of which each fixture has one.
 */
auto StyleOf(const Drawing &drawing) -> const TableStyle &
{
  const TableStyle *found = nullptr;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *style = GetIf<TableStyle>(&object))
    {
      REQUIRE(found == nullptr);
      found = style;
    }
  }
  REQUIRE(found != nullptr);
  return *found;
}

auto StyleName(const Drawing &drawing, Handle style) -> std::string
{
  const Object *found = Find(drawing, style);
  REQUIRE(found != nullptr);
  return Get<Tables::Style>(*found).Record.Name;
}

/**
 *  The cells up to R2007 agree with the twin's groups, which DXF writes in every release.
 */
void Agrees(const TableStyle &style, const std::vector<Pair> &twin, std::size_t opened, const Drawing &drawing)
{
  CHECK(style.Description == GroupOf(twin, opened, 3));
  CHECK(style.Flow == ReadWhole(twin, opened, 70));
  CHECK(style.Flags == ReadWhole(twin, opened, 71));
  CHECK(style.Horizontal == doctest::Approx(ReadReal(twin, opened, 40)));
  CHECK(style.Vertical == doctest::Approx(ReadReal(twin, opened, 41)));
  CHECK(style.TitleSuppressed == (Every(twin, opened, 280).back() == "1"));
  CHECK(style.HeaderSuppressed == (ReadWhole(twin, opened, 281) == 1));
  REQUIRE(style.Cells.size() == 3);
  const std::vector<std::string> names = Every(twin, opened, 7);
  REQUIRE(names.size() == 3);
  for (std::size_t at = 0; at < 3; ++at)
  {
    CAPTURE(at);
    const RowStyle &cell = style.Cells[at];
    CHECK(SameName(StyleName(drawing, cell.Style), names[at], drawing));
    CHECK(cell.Height == doctest::Approx(std::stod(Every(twin, opened, 140)[at])));
    CHECK(cell.Alignment == std::stoi(Every(twin, opened, 170)[at]));
    CHECK(cell.Color.Index == std::stoi(Every(twin, opened, 62)[at]));
    CHECK(cell.Fill.Index == std::stoi(Every(twin, opened, 63)[at]));
    CHECK(cell.Filled == (Every(twin, opened, 283)[at] == "1"));
    const std::array<const Border *, 6> borders{&cell.Top,  &cell.Middle, &cell.Bottom,
                                                &cell.Left, &cell.Center, &cell.Right};
    for (std::size_t side = 0; side < borders.size(); ++side)
    {
      const int offset = static_cast<int>(side);
      CHECK(borders[side]->Lineweight == std::stoi(Every(twin, opened, 274 + offset)[at]));
      CHECK(borders[side]->Visible == (Every(twin, opened, 284 + offset)[at] == "1"));
      CHECK(borders[side]->Color.Index == std::stoi(Every(twin, opened, 64 + offset)[at]));
    }
    if (drawing.Release >= Version::R2007)
    {
      CHECK(cell.Type == std::stoi(Every(twin, opened, 90)[at]));
      CHECK(cell.Unit == std::stoi(Every(twin, opened, 91)[at]));
      CHECK(cell.Format == Every(twin, opened, 1)[at]);
    }
  }
}

/**
 *  A table style written and read back as that release.
 */
auto RoundTrip(const TableStyle &style, Version release, std::vector<Issue> &issues) -> std::optional<Object>
{
  const std::vector<Class> classes{
      Class{.Number = 500, .CppName = "AcDbTableStyle", .DxfName = "TABLESTYLE", .ItemClass = 0x1F3}};
  const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = classes, .Issues = &issues};
  return ReadObject(WriteObject(Object{style}, with), 0, reading);
}

/**
 *  A cell style with everything stored: data, margins and a border of each kind.
 */
auto Full() -> TableStyles::CellStyle
{
  TableStyles::CellStyle style{.Type = 5, .Data = 1, .Overrides = 0x200, .Merge = 0x8000};
  style.Background =
      Color{.Index = 0, .Rgb = static_cast<int>(0xC2102030U), .Names = ColorNames{.Name = "teal", .Book = "book"}};
  style.Content.Format = "%lu2";
  style.Content.Style = Handle{0x11};
  style.Content.Height = 2.5;
  style.Margined = 1;
  style.Margins = {.Top = 1, .Left = 2, .Bottom = 3, .Right = 4, .HorizontalSpacing = 5, .VerticalSpacing = 6};
  style.Borders = {TableStyles::Edge{},
                   TableStyles::Edge{.Side = 8, .Type = 2, .Linetype = Handle{0x14}, .Invisible = true}};
  return style;
}
} // namespace

TEST_CASE("objects/tablestyle: the Standard style of every table fixture matches its DXF twin in the R2007 form")
{
  for (const std::string_view fixture : {"table", "cells"})
  {
    for (const std::string_view release : FixtureSuffixes)
    {
      CAPTURE(fixture);
      CAPTURE(release);
      const std::vector<Pair> twin = ReadTwin(TwinOf(fixture, release));
      const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("{}-{}.dwg", fixture, release)));
      REQUIRE(loaded);
      CHECK(loaded->Issues.empty());
      const Drawing &drawing = loaded->Drawing;
      const TableStyle &style = StyleOf(drawing);
      const std::optional<std::size_t> opened = FindHandled(twin, style.Base.Self);
      REQUIRE(opened);
      CHECK(GroupOf(twin, *opened, 0).empty());
      if (drawing.Release < Version::R2010)
      {
        CHECK(style.Styles.empty());
        Agrees(style, twin, *opened, drawing);
        continue;
      }
      CHECK(style.Cells.empty());
      CHECK(style.Revision == std::stoi(Every(twin, *opened, 280).front()));
      CHECK(style.Overall.Name == "Table");
      REQUIRE(style.Styles.size() == 3);
      CHECK(style.Styles[0].Name == "_TITLE");
      CHECK(style.Styles[1].Name == "_HEADER");
      CHECK(style.Styles[2].Name == "_DATA");
      // From R2010 the file keeps cell styles; DXF writes the three cells they come to below R2010.
      const Object lowered = Releases::ConvertObject(Object{style}, drawing, Version::R2007).Object;
      Agrees(Get<TableStyle>(lowered), twin, *opened, drawing);
    }
  }
}

TEST_CASE("objects/tablestyle: saving across R2010 builds the other form like AutoCAD's own file")
{
  for (const std::string_view fixture : {"table", "cells"})
  {
    CAPTURE(fixture);
    const Drawing older = Load(ReadFixture(std::format("{}-r2007.dwg", fixture))).value().Drawing;
    const Drawing newer = Load(ReadFixture(std::format("{}-r2010.dwg", fixture))).value().Drawing;
    const Releases::Translation upgraded = Releases::ConvertObject(Object{StyleOf(older)}, older, Version::R2010);
    const Releases::Translation lowered = Releases::ConvertObject(Object{StyleOf(newer)}, newer, Version::R2007);
    CHECK(upgraded.Issues.empty());
    CHECK(lowered.Issues.empty());
    TableStyle up = Get<TableStyle>(upgraded.Object);
    TableStyle down = Get<TableStyle>(lowered.Object);
    up.Base = StyleOf(newer).Base;
    down.Base = StyleOf(older).Base;
    CHECK(up == StyleOf(newer));
    CHECK(down == StyleOf(older));
  }
}

TEST_CASE("objects/tablestyle: the flags convert across releases like AutoCAD's saves of a style")
{
  // AutoCAD saves a style of flags 1 from R2007 as 1 in R2010 and R2013 and as 9 in R2018; from R2010 or R2013 as 1 in
  // R2018; from R2018 as 1 again in R2013 and R2007.
  for (const auto &[from, to, flags] :
       {std::tuple{"cells-r2007", "cells-r2007-as-r2018", 9}, std::tuple{"table-r2007", "table-r2013", 1},
        std::tuple{"table-r2010", "table-r2018", 1}, std::tuple{"table-r2018", "table-r2013", 1},
        std::tuple{"table-r2018", "table-r2007", 1}, std::tuple{"cells-r2007-as-r2018", "cells-r2013", 1},
        std::tuple{"cells-r2007-as-r2018", "cells-r2007", 1}})
  {
    CAPTURE(from);
    CAPTURE(to);
    const Drawing older = Load(ReadFixture(std::format("{}.dwg", from))).value().Drawing;
    const Drawing newer = Load(ReadFixture(std::format("{}.dwg", to))).value().Drawing;
    TableStyle style = StyleOf(older);
    style.Flags += 1;
    TableStyle crossed = Get<TableStyle>(Releases::ConvertObject(Object{style}, older, newer.Release).Object);
    CHECK(crossed.Flags == flags);
    CHECK(crossed.Flags == StyleOf(newer).Flags + 1);
    crossed.Flags = StyleOf(newer).Flags;
    crossed.Base = StyleOf(newer).Base;
    CHECK(crossed == StyleOf(newer));
  }
}

TEST_CASE("objects/tablestyle: a style's flags saved to R2018 and back load unchanged")
{
  Drawing drawing = Load(ReadFixture("table-r2007.dwg")).value().Drawing;
  for (Object &object : drawing.Objects)
  {
    if (auto *style = GetIf<TableStyle>(&object))
    {
      style->Flags = 1;
    }
  }
  const std::expected<Saved, Failure> up = Save(drawing, SaveOptions{.Release = Version::R2018});
  REQUIRE(up);
  const std::expected<Loaded, Failure> upgraded = Load(up->Bytes);
  REQUIRE(upgraded);
  CHECK(upgraded->Issues.empty());
  CHECK(StyleOf(upgraded->Drawing).Flags == 9);
  QueueAudit(up->Bytes, "dwgio-table-flagged-as-r2018.dwg");
  const std::expected<Saved, Failure> down = Save(upgraded->Drawing, SaveOptions{.Release = Version::R2007});
  REQUIRE(down);
  const std::expected<Loaded, Failure> back = Load(down->Bytes);
  REQUIRE(back);
  CHECK(StyleOf(back->Drawing).Flags == 1);
  QueueAudit(down->Bytes, "dwgio-table-flagged-as-r2018-as-r2007.dwg");
}

TEST_CASE("objects/tablestyle: suppressed rows are noted on upgrading to R2010")
{
  Drawing drawing;
  drawing.Release = Version::R2004;
  TableStyle style;
  style.Base.Self = Handle{0x7F};
  style.Flow = 1;
  style.Cells = {RowStyle{.Fill = Color{.Index = 3}, .Filled = true}};
  style.Cells.front().Top.Visible = false;
  for (const auto &[untitled, headless, flags] : {std::tuple{true, false, 0}, std::tuple{false, true, 0},
                                                  std::tuple{false, false, 2}, std::tuple{false, false, 0}})
  {
    style.TitleSuppressed = untitled;
    style.HeaderSuppressed = headless;
    style.Flags = flags;
    const Releases::Translation upgraded = Releases::ConvertObject(Object{style}, drawing, Version::R2013);
    CHECK(upgraded.Issues.size() == (untitled || headless ? 1 : 0));
    const auto &up = Get<TableStyle>(upgraded.Object);
    CHECK(up.Flags == flags);
    REQUIRE(up.Styles.size() == 3);
    CHECK(up.Overall.Style.Merge == 0x10000);
    CHECK(up.Styles[2].Style.Background == Color{.Index = 3});
    CHECK(up.Styles[2].Style.Borders.front().Invisible);
    Drawing newer;
    newer.Release = Version::R2013;
    const TableStyle down = Get<TableStyle>(Releases::ConvertObject(Object{up}, newer, Version::R2004).Object);
    CHECK(down.Flow == 1);
    CHECK(down.Flags == flags);
    REQUIRE(down.Cells.size() == 3);
    CHECK(down.Cells.front().Filled);
    CHECK(down.Cells.front().Fill == Color{.Index = 3});
    CHECK_FALSE(down.Cells.front().Top.Visible);
    CHECK(down.Cells[1] == RowStyle{});
  }
}

TEST_CASE("objects/tablestyle: a style without margins takes the default margins below R2010")
{
  Drawing drawing;
  drawing.Release = Version::R2018;
  TableStyle style;
  style.Overall.Style.Margined = 0;
  style.Overall.Style.Margins.Left = 9;
  style.Styles = {TableStyles::NamedCellStyle{.Id = 101, .Style = Full(), .Class = 1, .Name = "mine"}};
  const TableStyle down = Get<TableStyle>(Releases::ConvertObject(Object{style}, drawing, Version::R2000).Object);
  CHECK(down.Horizontal == 1.5);
  CHECK(down.Vertical == 1.5);
  CHECK(down.Cells == std::vector<RowStyle>(3));
}

TEST_CASE("objects/tablestyle: a style of each form round-trips in each release of its form")
{
  TableStyle older;
  older.Base.Self = Handle{0x7F};
  older.Description = "mine";
  older.Flow = 1;
  older.Flags = 2;
  older.Horizontal = 0.5;
  older.TitleSuppressed = true;
  older.Cells = {RowStyle{.Style = Handle{0x11}, .Fill = Color{.Index = 5}, .Filled = true}, RowStyle{}, RowStyle{}};
  older.Cells[1].Right = Border{.Lineweight = 30, .Visible = false, .Color = Color{.Index = 1}};
  TableStyle newer;
  newer.Base.Self = Handle{0x7F};
  newer.Revision = 1;
  newer.Description = "mine";
  newer.Flags = 3;
  newer.Unknown1 = Handle{0x90};
  newer.Overall = TableStyles::NamedCellStyle{.Id = 4, .Style = Full(), .Class = 2, .Name = "Table"};
  newer.Styles = {
      TableStyles::NamedCellStyle{.Id = 1, .Style = TableStyles::CellStyle{.Type = 5}, .Class = 1, .Name = "_TITLE"},
      TableStyles::NamedCellStyle{.Id = 3, .Style = Full(), .Class = 2, .Name = "_DATA"}};
  for (const Version release : FixtureReleases)
  {
    CAPTURE(static_cast<int>(release));
    const TableStyle &style = release < Version::R2010 ? older : newer;
    std::vector<Issue> issues;
    const std::optional<Object> back = RoundTrip(style, release, issues);
    REQUIRE(back);
    CHECK(*back == Object{style});
    CHECK(issues.empty());
  }
}

TEST_CASE("objects/tablestyle: fewer than three cells are padded with new ones and extra cells are dropped")
{
  TableStyle style;
  style.Base.Self = Handle{0x7F};
  style.Cells = {RowStyle{.Height = 9}};
  std::vector<Issue> issues;
  std::optional<Object> back = RoundTrip(style, Version::R2000, issues);
  REQUIRE(back);
  CHECK(Get<TableStyle>(*back).Cells == std::vector<RowStyle>{RowStyle{.Height = 9}, RowStyle{}, RowStyle{}});
  style.Cells.resize(4);
  back = RoundTrip(style, Version::R2000, issues);
  REQUIRE(back);
  CHECK(Get<TableStyle>(*back).Cells.size() == 3);
}

TEST_CASE("objects/tablestyle: a count of styles or borders beyond the data stops the read")
{
  const auto reading = [](Bits::Reader &in) -> Fields::Input
  {
    return Fields::Input{.In = &in, .With = Fields::Context{.Release = Version::R2010, .Text = &Latin}};
  };
  Bits::Writer styles;
  styles.BL(100000);
  const std::vector<std::byte> many = styles.Take();
  Bits::Reader in{many};
  TableStyle style;
  CHECK_FALSE(Shapes::Table::ReadStyles(style, reading(in)));
  Bits::Writer borders;
  borders.BL(5);
  borders.BS(1);
  borders.BL(0);
  borders.BL(0);
  borders.CMC(Color{}, Version::R2010);
  borders.BL(1);
  for (int at = 0; at < 4; ++at)
  {
    borders.BL(0);
  }
  borders.TV("", Version::R2010, Latin);
  borders.BD(0);
  borders.BD(1);
  borders.BL(1);
  borders.CMC(Color{}, Version::R2010);
  borders.BD(0);
  borders.BS(0);
  borders.BL(100000);
  const std::vector<std::byte> edged = borders.Take();
  Bits::Reader edges{edged};
  Bits::Writer handles;
  handles.H(Bits::Reference{.Code = 5, .Value = Handle{}});
  const std::vector<std::byte> stream = handles.Take();
  Bits::Reader handled{stream};
  Fields::Input cut = reading(edges);
  cut.Handles = &handled;
  TableStyles::CellStyle cellStyle;
  Shapes::Table::ReadCellStyle(cellStyle, cut);
  CHECK(edges.HasFailed());
  CHECK(cellStyle.Borders.empty());
  CHECK(cellStyle.Content.Alignment == 1);
  // A style read short stops the list there.
  Bits::Writer one;
  one.BL(1);
  one.BL(1);
  const std::vector<std::byte> brief = one.Take();
  Bits::Reader briefIn{brief};
  TableStyle cutStyle;
  CHECK_FALSE(Shapes::Table::ReadStyles(cutStyle, reading(briefIn)));
}

TEST_CASE("objects/tablestyle: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Objects
