/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/table/content.cpp
 *  @brief     A table's content and its cell checksums across R2007.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/table/content.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"

#include "dwgio/entities/table.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/tablecontent.hpp"
#include "dwgio/objects/tablegeometry.hpp"
#include "dwgio/objects/tablestyles/value.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO::Releases::Table
{
namespace
{
namespace TableContents = Objects::TableContents;
using Objects::TableStyles::Value;

const Encoding::Scheme Latin = Encoding::MakeScheme(Encoding::Standard());

/**
 *  UTF-16 with its NUL as a value's bytes.
 */
auto Wide(const std::u16string &text) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes;
  for (const char16_t unit : text + u'\0')
  {
    bytes.push_back(static_cast<std::byte>(unit & 0xFFU));
    bytes.push_back(static_cast<std::byte>(unit >> 8U));
  }
  return bytes;
}

/**
 *  A string value as R2007 keeps it.
 */
auto Text(const std::u16string &text) -> Value
{
  return Value{.Flags = 4, .Type = 4, .Bytes = Wide(text)};
}

/**
 *  A content of one row of the cells given, each with one content of the value given.
 */
auto Row(const std::vector<Value> &values) -> TableContents::Data
{
  TableContents::Data content;
  TableContents::Row &row = content.Rows.emplace_back();
  for (const Value &value : values)
  {
    content.Columns.push_back(TableContents::Column{.Width = 1});
    row.Cells.push_back(TableContents::Cell{.Items = {TableContents::Item{.Value = value}}});
  }
  return content;
}

/**
 *  The checksum a stored cell keeps.
 */
auto SumOf(const TableContents::Cell &cell) -> double
{
  REQUIRE(!cell.Custom.empty());
  return cell.Custom.front().Value.Real;
}
} // namespace

TEST_CASE("releases/table/content: a cell's checksum weighs each UTF-16 unit of its text by its position")
{
  CHECK(Checksum("") == 0.0);
  CHECK(Checksum("M8") == 189.0);
  CHECK(Checksum("Parts") == 1655.0);
}

TEST_CASE("releases/table/content: a value's text is its own or else its UTF-16 string")
{
  CHECK(TextOf(Value{.Flags = 4, .Type = 4, .Text = "Own"}) == "Own");
  CHECK(TextOf(Text(u"Bolt")) == "Bolt");
  CHECK(TextOf(Value{.Flags = 2, .Type = 2, .Real = 3}).empty());
  CHECK(TextOf(Value{.Flags = 7}).empty());
  CHECK(TextOf(Value{.Flags = 4, .Type = 4, .Bytes = {std::byte{'A'}, std::byte{}}}) == "A"); // No NUL to end it.
}

TEST_CASE("releases/table/content: a content's values convert across R2007 like AutoCAD's")
{
  TableContents::Data content = Row({Text(u"Nut"), Value{.Flags = 2, .Type = 2, .Real = 1.5}});
  content.Columns.front().Custom = {TableContents::Datum{.Name = "column", .Value = Text(u"A")}};
  content.Rows.front().Custom = {TableContents::Datum{.Name = "row", .Value = Text(u"B")}};
  content.Rows.front().Cells.front().Custom = {TableContents::Datum{.Name = "cell", .Value = Text(u"C")}};
  content.Rows.front().Cells.front().Items.front().Value.Text = "Nut";
  Recode(content, Version::R2004, Latin);
  const Value &nut = content.Rows.front().Cells.front().Items.front().Value;
  CHECK(nut.Flags == 0);
  CHECK(nut.Text.empty());
  CHECK(nut.Bytes == std::vector<std::byte>{std::byte{'N'}, std::byte{'u'}, std::byte{'t'}, std::byte{}});
  CHECK(content.Columns.front().Custom.front().Value.Bytes.size() == 2);
  CHECK(content.Rows.front().Custom.front().Value.Bytes.size() == 2);
  CHECK(content.Rows.front().Cells.back().Items.front().Value.Real == 1.5);
  Recode(content, Version::R2007, Latin);
  CHECK(content.Rows.front().Cells.front().Items.front().Value.Bytes == Wide(u"Nut"));
  CHECK(content.Rows.front().Cells.front().Custom.front().Value.Bytes == Wide(u"C"));
  TableContents::Data unended =
      Row({Value{.Type = 4, .Bytes = {std::byte{'Z'}}}}); // A string in the code page, no NUL.
  Recode(unended, Version::R2007, Latin);
  CHECK(unended.Rows.front().Cells.front().Items.front().Value.Bytes == Wide(u"Z"));
}

TEST_CASE("releases/table/content: a content keeps the extents its geometry gives")
{
  const TableContents::Data content = Row({Text(u"A"), Text(u"B")});
  const Objects::TableGeometry geometry{
      .Base = {},
      .Rows = 1,
      .Columns = 2,
      .Cells = {Objects::TableGeometries::Cell{.Unknown1 = 3, .Width = 2, .Height = 4}}};
  const Entities::AcadTable table;
  const TableContents::Data read =
      ReadContent(table, Held{.Content = &content, .Geometry = &geometry, .From = Version::R2013}, Latin);
  CHECK(read.Rows.front().Cells.front().Sized == 1);
  CHECK(read.Rows.front().Cells.front().Shaped == 0);
  CHECK(read.Rows.front().Cells.back().Sized == 0);
  const TableContents::Data bare =
      ReadContent(table, Held{.Content = &content, .Geometry = nullptr, .From = Version::R2007}, Latin);
  CHECK(bare.Rows.front().Cells.front().Sized == 0);
  CHECK(bare.Rows.front().Cells.front().Items.size() == 1);
}

TEST_CASE("releases/table/content: a grid edited after its content was stored gives its own text")
{
  TableContents::Data content = Row({Text(u"Same"), Text(u"Was"), Text(u"Plain")});
  for (std::size_t at = 0; at < 2; ++at)
  {
    TableContents::Cell &cell = content.Rows.front().Cells[at];
    cell.Custom = {
        TableContents::Datum{.Name = "ACAD_ROUNDTRIP_2008_CELL_CHECKSUM",
                             .Value = Value{.Flags = 2, .Type = 2, .Real = Checksum(at == 0 ? "Same" : "Was")}}};
  }
  Entities::AcadTable table;
  table.Cells = {Entities::AcadTables::Cell{.Value = Text(u"Same")}, Entities::AcadTables::Cell{.Value = Text(u"Now")},
                 Entities::AcadTables::Cell{.Value = Text(u"Other")}};
  const TableContents::Data read =
      ReadContent(table, Held{.Content = &content, .Geometry = nullptr, .From = Version::R2007}, Latin);
  CHECK(TextOf(read.Rows.front().Cells[0].Items.front().Value) == "Same");
  CHECK(TextOf(read.Rows.front().Cells[1].Items.front().Value) == "Now");
  CHECK(TextOf(read.Rows.front().Cells[2].Items.front().Value) == "Plain");
  table.Cells = {Entities::AcadTables::Cell{.Text = "Same"}, Entities::AcadTables::Cell{.Text = ""}};
  const TableContents::Data older =
      ReadContent(table, Held{.Content = &content, .Geometry = nullptr, .From = Version::R2004}, Latin);
  CHECK(older.Rows.front().Cells[1].Items.front().Value.Flags == 2);
  CHECK(older.Rows.front().Cells[1].Items.front().Format.Overrides == 0);
}

TEST_CASE("releases/table/content: upgrading rebuilds a table from its content and downgrading from its grid")
{
  TableContents::Data content = Row({Text(u"Wide"), Value{.Flags = 0, .Type = 512, .Bytes = Wide(u"General")},
                                     Value{.Flags = 0, .Type = 512, .Bytes = Wide(u"")}});
  content.Style.Merge = 0x10000;
  content.Merged = {TableContents::Range{.Top = 1, .Left = 0, .Bottom = 1, .Right = 1},
                    TableContents::Range{.Top = 0, .Left = 0, .Bottom = 0, .Right = 0}};
  Entities::AcadTable table{.Flags = 22};
  ToGridForm(table, content, Span{.From = Version::R2018, .To = Version::R2004});
  CHECK(table.Flags == 6);
  REQUIRE(table.Cells.size() == 3);
  CHECK(table.Cells[0].Text == "Wide");
  CHECK(table.Cells[1].Overrides == 0x40000);
  CHECK(table.Cells[2].Overrides == 0);
  CHECK(table.Content == TableContents::Data{});
  content.Style.Merge = 0;
  ToGridForm(table, content, Span{.From = Version::R2018, .To = Version::R2007});
  CHECK(table.Flags == 22);
  CHECK(table.Cells[0].Value == content.Rows.front().Cells.front().Items.front().Value);
  content.Rows.front().Cells.front().Unknown1 = 7;
  ToContentForm(table, content, Span{.From = Version::R2007, .To = Version::R2010});
  CHECK(table.Flags == 6);
  CHECK(table.Unknown3 == 1);
  CHECK(table.Content.Rows.front().Cells.front().Unknown1 == 3);
  REQUIRE(table.Parts.size() == 1);
  CHECK(table.Parts.front().End == 0);
  ToContentForm(table, content, Span{.From = Version::R2013, .To = Version::R2018});
  CHECK(table.Unknown3 == 0);
  CHECK(table.Content.Rows.front().Cells.front().Unknown1 == 7);
}

TEST_CASE("releases/table/content: a merged range covers the cells right of and below its first")
{
  TableContents::Data content = Row({Text(u"A"), Text(u"B")});
  content.Rows.push_back(content.Rows.front());
  content.Merged = {TableContents::Range{.Top = 0, .Left = 0, .Bottom = 1, .Right = 0},
                    TableContents::Range{.Top = 0, .Left = 1, .Bottom = 0, .Right = 1}};
  Entities::AcadTable table;
  ToGridForm(table, content, Span{.From = Version::R2018, .To = Version::R2007});
  REQUIRE(table.Cells.size() == 4);
  CHECK(table.Cells[0].Down == 2);
  CHECK_FALSE(table.Cells[1].Merged);
  CHECK(table.Cells[2].Merged);
  CHECK_FALSE(table.Cells[3].Merged);
}

TEST_CASE("releases/table/content: a grid alone gives the content AutoCAD builds from it")
{
  Entities::AcadTable table;
  table.Widths = {1, 2};
  table.Heights = {3, 4, 5};
  table.Cells = {Entities::AcadTables::Cell{.Down = 2, .Text = "Down"}, Entities::AcadTables::Cell{.Across = 1}};
  const TableContents::Data built = MakeContent(table, Version::R2004);
  REQUIRE(built.Rows.size() == 3);
  CHECK(built.Rows[0].Id == 1);
  CHECK(built.Rows[1].Id == 2);
  CHECK(built.Rows[2].Id == 3);
  REQUIRE(built.Merged.size() == 1);
  CHECK(built.Merged.front() == TableContents::Range{.Top = 0, .Left = 0, .Bottom = 1, .Right = 0});
  CHECK(TextOf(built.Rows[0].Cells[0].Items.front().Value) == "Down");
  CHECK(built.Rows[2].Cells[1].Items.front().Format.Overrides == 0);
  table.Cells = {Entities::AcadTables::Cell{.Value = Value{.Flags = 7}},
                 Entities::AcadTables::Cell{.Value = Text(u"Set")}};
  const TableContents::Data later = MakeContent(table, Version::R2007);
  CHECK(later.Rows[0].Cells[0].Items.front().Value.Flags == 3);
  CHECK(later.Rows[0].Cells[1].Items.front().Format.Type == 4);
}

TEST_CASE("releases/table/content: a span or range near a long's limits stops at them")
{
  constexpr std::int32_t Most = std::numeric_limits<std::int32_t>::max();
  Entities::AcadTable table;
  table.Widths = {1, 2};
  table.Heights = {3, 4};
  table.Widths.push_back(3);
  table.Cells.resize(6);
  table.Cells[5] = Entities::AcadTables::Cell{.Across = Most, .Down = Most}; // From the third column, past a long.
  const TableContents::Data built = MakeContent(table, Version::R2004);
  REQUIRE(built.Merged.size() == 1);
  CHECK(built.Merged.front() == TableContents::Range{.Top = 1, .Left = 2, .Bottom = Most, .Right = Most});
  TableContents::Data content = Row({Text(u"A"), Text(u"B")});
  content.Merged = {TableContents::Range{.Top = 0, .Left = 0, .Bottom = 0, .Right = Most}};
  ToGridForm(table, content, Span{.From = Version::R2018, .To = Version::R2007});
  REQUIRE(table.Cells.size() == 2);
  CHECK(table.Cells[0].Across == Most);
  CHECK(table.Cells[0].Down == 1);
}

TEST_CASE("releases/table/content: a stored content keeps one checksum per cell and no extents")
{
  TableContents::Data content = Row({Text(u"M8"), Value{.Flags = 2, .Type = 2, .Real = 1}, Value{.Flags = 3}});
  content.Merged = {TableContents::Range{.Top = 0, .Left = 0, .Bottom = 0, .Right = 1}};
  TableContents::Cell &first = content.Rows.front().Cells.front();
  first.Custom = {TableContents::Datum{.Name = "ACAD_ROUNDTRIP_2008_CELL_CHECKSUM", .Value = {}},
                  TableContents::Datum{.Name = "Other", .Value = {}}};
  first.Sized = 1;
  first.Shaped = 1;
  first.Unknown1 = 3;
  const TableContents::Data stored = MakeStoredContent(content, Handle{0x7F});
  CHECK(stored.TableStyle == Handle{0x7F});
  const TableContents::Cell &made = stored.Rows.front().Cells.front();
  CHECK(made.Custom.size() == 2);
  CHECK(SumOf(made) == 189.0);
  CHECK(made.Sized == 0);
  CHECK(made.Items.front().Value.Text == "M8");
  CHECK(stored.Rows.front().Cells[1].Items.empty());
  CHECK(SumOf(stored.Rows.front().Cells[1]) == 0.0);
  const TableContents::Data loose =
      MakeStoredContent(Row({Value{.Flags = 2, .Type = 2, .Real = 1}, Value{.Flags = 3}}), Handle{});
  CHECK(loose.Rows.front().Cells[0].Items.front().Value.Flags == 2);
  CHECK(loose.Rows.front().Cells[1].Items.front().Value.Flags == 7);
}

TEST_CASE("releases/table/content: a geometry gives each cell's extents")
{
  TableContents::Data content = Row({Text(u"A"), Text(u"B")});
  content.Rows.front().Cells.front().Shaped = 1;
  content.Rows.front().Cells.front().Geometry.Width = 9;
  const Objects::TableGeometry geometry = MakeGeometry(content);
  CHECK(geometry.Rows == 1);
  CHECK(geometry.Columns == 2);
  REQUIRE(geometry.Cells.size() == 2);
  REQUIRE(geometry.Cells.front().Contents.size() == 1);
  CHECK(geometry.Cells.front().Contents.front().Width == 9);
  CHECK(geometry.Cells.back().Contents.empty());
}

TEST_CASE("releases/table/content: a data table's cells hold each value as R2007 keeps it")
{
  TableContents::Data content = Row({Text(u"S"), Value{.Flags = 2, .Type = 2, .Real = 2.5},
                                     Value{.Flags = 0, .Type = 1, .Long = 7}, Value{.Flags = 7}, Value{}});
  content.Rows.front().Cells.push_back(TableContents::Cell{});
  const std::vector<std::vector<Tag>> cells = MakeDataCells(content);
  REQUIRE(cells.size() == 6);
  CHECK(cells[0][2] == Tag{.Code = 1, .Value = std::string{"S"}});
  CHECK(cells[1][2] == Tag{.Code = 40, .Value = 2.5});
  CHECK(cells[2][2] == Tag{.Code = 91, .Value = std::int64_t{7}});
  CHECK(cells[3][0] == Tag{.Code = 93, .Value = std::int64_t{7}});
  CHECK(cells[4][2] == Tag{.Code = 91, .Value = std::int64_t{0}});
  CHECK(cells[5][1] == Tag{.Code = 90, .Value = std::int64_t{0}});
}
} // namespace DwgIO::Releases::Table
