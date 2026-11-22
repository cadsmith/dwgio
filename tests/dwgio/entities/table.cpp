/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/table.cpp
 *  @brief     The ACAD_TABLE against its DXF twin in each release, written back in each form.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/table.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/table.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/shapes/table/content.hpp"
#include "objects/shapes/table/value.hpp"
#include "objects/write.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/class.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/tablecontent.hpp"
#include "dwgio/objects/tablestyles/value.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
using Objects::Latin;
namespace TableContents = Objects::TableContents;

/**
 *  The table of a loaded drawing, of which each fixture has one.
 */
auto TableOf(const Drawing &drawing) -> const AcadTable &
{
  const AcadTable *found = nullptr;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *table = GetIf<AcadTable>(&object))
    {
      REQUIRE(found == nullptr);
      found = table;
    }
  }
  REQUIRE(found != nullptr);
  return *found;
}

/**
 *  A table written and read back as that release.
 */
auto RoundTrip(const AcadTable &table, Version release, std::vector<Issue> &issues) -> std::optional<Object>
{
  const std::vector<Class> classes{
      Class{.Number = 500, .CppName = "AcDbTable", .DxfName = "ACAD_TABLE", .ItemClass = 0x1F2}};
  const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = classes, .Issues = &issues};
  return Objects::ReadObject(Objects::WriteObject(Object{table}, with), 0, reading);
}

/**
 *  A table's entity data as the writer wants it.
 */
auto Bare() -> AcadTable
{
  AcadTable table;
  table.Base.Self = Handle{0x2CC};
  table.Base.Owner = Handle{0x1F};
  table.Common.Layer = Handle{0x10};
  table.Block = Handle{0x2CF};
  table.Style = Handle{0x7F};
  return table;
}

/**
 *  A table of the releases up to R2007 with two columns and two rows.
 */
auto Older(Version release) -> AcadTable
{
  AcadTable table = Bare();
  table.Flags = 22;
  table.Widths = {10, 20};
  table.Heights = {5, 6};
  table.Cells.resize(4);
  table.Cells[0].Across = 2;
  table.Cells[0].Overridden = true;
  table.Cells[0].Overrides = AcadTables::Texted;
  table.Cells[1].Merged = true;
  table.Cells[2].Edges = 5;
  table.Cells[3].Rotation = 0.5;
  table.Cells[3].Autofit = true;
  if (release >= Version::R2007)
  {
    table.Cells[0].Value = {
        .Flags = 4, .Type = 4, .Bytes = {std::byte{0x41}, std::byte{0}, std::byte{0}, std::byte{0}}, .Text = "A"};
    table.Cells[1].Value = {.Flags = 7};
    table.Cells[2].Value = {.Type = 2, .Real = 1.5, .Unit = 1, .Format = "%lu2", .Text = "1.5"};
    table.Cells[3].Value = {.Type = 1, .Long = 7};
    table.Cells[1].Value = {.Type = 0, .Long = 2};
  }
  else
  {
    table.Cells[0].Text = "A";
  }
  return table;
}

/**
 *  A table of the releases from R2010 with one column and one row of one cell.
 */
auto Newer() -> AcadTable
{
  AcadTable table = Bare();
  table.Unknown3 = 1;
  table.Flags = 6;
  table.Parts = {AcadTables::Part{.Position = {.X = 1}, .Start = 0, .End = 0}};
  TableContents::Data &content = table.Content;
  content.Columns = {TableContents::Column{.Name = "a", .Width = 10}};
  TableContents::Cell cell{.Tooltip = "tip"};
  TableContents::Item item{.Value = {.Type = 512, .Bytes = {std::byte{1}, std::byte{2}}}, .Formatted = 1};
  item.Format.Style = Handle{0x11};
  cell.Items = {item, TableContents::Item{.Value = {.Type = 8, .Bytes = {std::byte{3}}}}};
  cell.Sized = 1;
  cell.Unknown1 = 3;
  cell.Shaped = 1;
  cell.Geometry = TableContents::Geometry{.Corner = {.X = 1}, .Width = 2, .Flags = 1};
  TableContents::Cell bare;
  TableContents::Cell unshaped{.Sized = 1, .Unknown1 = 3, .Width = 4};
  content.Rows = {TableContents::Row{.Cells = {cell, bare, unshaped}, .Id = 3, .Height = 5}};
  content.Style.Data = 1;
  content.Merged = {TableContents::Range{.Top = 0, .Left = 0, .Bottom = 0, .Right = 1}};
  return table;
}
} // namespace

TEST_CASE("entities/table: every table fixture's table matches its DXF twin in each release")
{
  for (const std::string_view fixture : {"table", "cells"})
  {
    for (const std::string_view release : FixtureSuffixes)
    {
      CAPTURE(fixture);
      CAPTURE(release);
      const std::vector<Pair> twin = ReadTwin(std::format("{}-{}.dxf", fixture, release == "r14" ? "r2000" : release));
      const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("{}-{}.dwg", fixture, release)));
      REQUIRE(loaded);
      CHECK(loaded->Issues.empty());
      const Drawing &drawing = loaded->Drawing;
      const AcadTable &table = TableOf(drawing);
      const std::optional<std::size_t> opened = Entity(twin, table.Base.Self);
      REQUIRE(opened);
      const Object *block = Find(drawing, table.Block);
      REQUIRE(block != nullptr);
      // The file names the anonymous block *T, which DXF numbers.
      CHECK(Get<Tables::BlockRecord>(*block).Record.Name == "*T");
      CHECK(GroupOf(twin, *opened, 2) == "*T1");
      CHECK(table.Block == ParseHandle(GroupOf(twin, *opened, 343)));
      CHECK(table.Style == ParseHandle(GroupOf(twin, *opened, 342)));
      CHECK(Near(table.Position, ReadPoint(twin, *opened, 10)));
      CHECK(Near(table.Scale, Vector{.X = 1, .Y = 1, .Z = 1}));
      CHECK(table.Rotation == 0);
      CHECK(Near(table.Extrusion, Normal(twin, *opened)));
      CHECK(Near(table.Direction, Along(twin, *opened, 11)));
      const std::vector<std::string> widths = DwgIO::Every(twin, *opened, 142);
      const std::vector<std::string> heights = DwgIO::Every(twin, *opened, 141);
      const std::vector<std::string> texts = DwgIO::Every(twin, *opened, drawing.Release >= Version::R2007 ? 302 : 1);
      REQUIRE(texts.size() == widths.size() * heights.size());
      if (drawing.Release < Version::R2010)
      {
        CHECK(table.Flags == ReadWhole(twin, *opened, 90));
        REQUIRE(table.Widths.size() == widths.size());
        REQUIRE(table.Heights.size() == heights.size());
        REQUIRE(table.Cells.size() == texts.size());
        for (std::size_t at = 0; at < table.Cells.size(); ++at)
        {
          const AcadTables::Cell &cell = table.Cells[at];
          CHECK(cell.Type == std::stoi(DwgIO::Every(twin, *opened, 171)[at]));
          CHECK(cell.Merged == (DwgIO::Every(twin, *opened, 173)[at] == "1"));
          CHECK(cell.Across == std::stoi(DwgIO::Every(twin, *opened, 175)[at]));
          CHECK(cell.Down == std::stoi(DwgIO::Every(twin, *opened, 176)[at]));
          CHECK((drawing.Release >= Version::R2007 ? cell.Value.Text : cell.Text) == texts[at]);
        }
        continue;
      }
      const TableContents::Data &content = table.Content;
      REQUIRE(content.Columns.size() == widths.size());
      REQUIRE(content.Rows.size() == heights.size());
      for (std::size_t at = 0; at < widths.size(); ++at)
      {
        CHECK(content.Columns[at].Width == doctest::Approx(std::stod(widths[at])));
      }
      std::size_t cell = 0;
      for (std::size_t at = 0; at < heights.size(); ++at)
      {
        CHECK(content.Rows[at].Height == doctest::Approx(std::stod(heights[at])));
        CHECK(content.Rows[at].Id == (at == 0 ? 1 : at == 1 ? 2 : 3));
        for (const TableContents::Cell &one : content.Rows[at].Cells)
        {
          CHECK((one.Items.empty() ? std::string{} : one.Items.front().Value.Text) == texts[cell++]);
        }
      }
      REQUIRE_FALSE(content.Merged.empty());
      CHECK(content.Merged.back().Right == 2);
    }
  }
}

TEST_CASE("entities/table: a table of each form round-trips in each release of its form")
{
  for (const Version release : FixtureReleases)
  {
    CAPTURE(static_cast<int>(release));
    AcadTable table = release < Version::R2010 ? Older(release) : Newer();
    if (release == Version::R2000)
    {
      table.Scale = Vector{.X = 2, .Y = 3, .Z = 4};
    }
    if (release == Version::R2004)
    {
      table.Scale = Vector{.X = 1, .Y = 3, .Z = 4};
    }
    if (release == Version::R2007)
    {
      table.Scale = Vector{.X = 2, .Y = 2, .Z = 2};
    }
    if (release == Version::R14)
    {
      table.Scale = Vector{.X = 2, .Y = 2, .Z = 5};
    }
    std::vector<Issue> issues;
    const std::optional<Object> back = RoundTrip(table, release, issues);
    REQUIRE(back);
    table.Common = Get<AcadTable>(*back).Common; // What the release's common data adds, tested on its own.
    CHECK(*back == Object{table});
    CHECK(issues.empty());
  }
}

TEST_CASE("entities/table: a table up to R2007 with missing cells is written with new ones")
{
  AcadTable table = Older(Version::R2000);
  table.Cells.resize(1);
  std::vector<Issue> issues;
  const std::optional<Object> back = RoundTrip(table, Version::R2000, issues);
  REQUIRE(back);
  CHECK(Get<AcadTable>(*back).Cells.size() == 4);
  CHECK(Get<AcadTable>(*back).Cells[3] == AcadTables::Cell{});
}

TEST_CASE("entities/table: a table holding what dwgio does not read is kept as bytes")
{
  const std::vector<std::pair<Version, std::function<void(AcadTable &)>>> cases{
      {Version::R2000, [](AcadTable &table) { table.Follows = true; }},
      {Version::R2000, [](AcadTable &table) { table.Cells[1].Type = 2; }},
      {Version::R2000,
       [](AcadTable &table)
       {
         table.Cells[1].Overridden = true;
         table.Cells[1].Overrides = 1;
       }},
      {Version::R2000, [](AcadTable &table) { table.Overridden = true; }},
      {Version::R2000, [](AcadTable &table) { table.Colored = true; }},
      {Version::R2000, [](AcadTable &table) { table.Weighted = true; }},
      {Version::R2000, [](AcadTable &table) { table.Shown = true; }},
      {Version::R2007, [](AcadTable &table) { table.Cells[1].Value = {.Type = 16}; }},
      {Version::R2010, [](AcadTable &table) { table.Content.Columns[0].Custom = {{.Value = {.Type = 64}}}; }},
      {Version::R2010, [](AcadTable &table) { table.Content.Rows[0].Cells[0].Custom = {{.Value = {.Type = 64}}}; }},
      {Version::R2010, [](AcadTable &table) { table.Content.Rows[0].Cells[0].Linked = 1; }},
      {Version::R2010, [](AcadTable &table) { table.Content.Rows[0].Cells[0].Items[0].Type = 2; }},
      {Version::R2010, [](AcadTable &table) { table.Content.Rows[0].Cells[0].Items[0].Value = {.Type = 64}; }},
      {Version::R2010, [](AcadTable &table) { table.Content.Rows[0].Cells[0].Items[0].Attributes = 1; }},
      {Version::R2010, [](AcadTable &table) { table.Content.Rows[0].Custom = {{.Value = {.Type = 64}}}; }},
      {Version::R2010, [](AcadTable &table) { table.Content.Fields = 1; }},
      {Version::R2010, [](AcadTable &table) { table.Broken = 1; }},
  };
  for (std::size_t at = 0; at < cases.size(); ++at)
  {
    CAPTURE(at);
    const auto &[release, change] = cases[at];
    AcadTable table = release < Version::R2010 ? Older(release) : Newer();
    change(table);
    std::vector<Issue> issues;
    const std::optional<Object> back = RoundTrip(table, release, issues);
    REQUIRE(back);
    CHECK(Holds<Unknown>(*back));
    CHECK(TypeOf(*back) == "ACAD_TABLE");
  }
}

TEST_CASE("entities/table: negative or oversized counts stop the read")
{
  const std::vector<std::byte> none;
  Bits::Reader handles{none};
  const auto reading = [&handles](Bits::Reader &in) -> Fields::Input
  {
    return Fields::Input{
        .In = &in, .Handles = &handles, .With = Fields::Context{.Release = Version::R2010, .Text = &Latin}};
  };
  for (const auto &[columns, rows] : {std::pair{-1, 0}, std::pair{0, -1}, std::pair{100000, 0}, std::pair{2, 100000}})
  {
    Bits::Writer grid;
    grid.BL(columns);
    grid.BL(rows);
    const std::vector<std::byte> bytes = grid.Take();
    Bits::Reader in{bytes};
    AcadTable table;
    CHECK_FALSE(Shapes::ReadTableGrid(table, reading(in)));
    CHECK(in.HasFailed());
  }
  const std::vector<std::byte> nothing;
  Bits::Reader empty{nothing};
  AcadTable blank;
  CHECK_FALSE(Shapes::ReadTableGrid(blank, reading(empty)));
  Bits::Writer parts;
  parts.BL(0);
  parts.BL(100000);
  const std::vector<std::byte> many = parts.Take();
  Bits::Reader partsIn{many};
  AcadTable table;
  CHECK_FALSE(Shapes::ReadTableParts(table, reading(partsIn)));
  Bits::Writer value;
  value.BL(0);
  value.BL(4);
  value.BL(100000);
  const std::vector<std::byte> sized = value.Take();
  Bits::Reader valueIn{sized};
  Objects::TableStyles::Value read;
  CHECK_FALSE(Objects::Shapes::Table::ReadValue(read, reading(valueIn)));
  for (const bool negative : {true, false})
  {
    Bits::Writer bytes;
    bytes.BL(0);
    bytes.BL(4);
    if (negative)
    {
      bytes.BL(-1);
    }
    const std::vector<std::byte> cut = bytes.Take();
    Bits::Reader cutIn{cut};
    CHECK_FALSE(Objects::Shapes::Table::ReadValue(read, reading(cutIn)));
  }
  // The content's lists: columns, rows and merged ranges, each counted beyond the data.
  for (int stop = 0; stop < 3; ++stop)
  {
    Bits::Writer content;
    content.TV("", Version::R2010, Latin);
    content.TV("", Version::R2010, Latin);
    content.BL(stop == 0 ? 100000 : 0);
    content.BL(stop == 1 ? 100000 : 0);
    content.BL(0);
    content.BL(4);
    content.BS(0);
    content.BL(100000);
    const std::vector<std::byte> bytes = content.Take();
    Bits::Reader in{bytes};
    TableContents::Data made;
    CHECK_FALSE(Objects::Shapes::Table::ReadTableContent(made, reading(in)));
  }
  // A row whose cell count outruns the data.
  Bits::Writer row;
  row.BL(100000);
  const std::vector<std::byte> cells = row.Take();
  Bits::Reader rowIn{cells};
  TableContents::Row made;
  CHECK_FALSE(Objects::Shapes::Table::ReadRow(made, reading(rowIn)));
}
} // namespace DwgIO::Entities
