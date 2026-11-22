/***********************************************************************************************************************
 *  @file      source/releases/table/content.cpp
 *  @brief     A table as AutoCAD holds it once read, whatever the release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/table/content.hpp"

#include "encoding/encoding.hpp"
#include "encoding/units.hpp"

#include "dwgio/entities/table.hpp"
#include "dwgio/objects/tablecontent.hpp"
#include "dwgio/objects/tablegeometry.hpp"
#include "dwgio/objects/tablestyles/value.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Releases::Table
{
namespace
{
using Objects::TableContents::Cell;
using Objects::TableContents::Data;
using Objects::TableContents::Item;
using Objects::TableContents::Range;
using Objects::TableStyles::Value;

/**
 *  The value types and flags AutoCAD's tables hold: a long, a double, a string and a general value; a value with no
 *  data, an empty one as a cell of a merged range holds it, and a string.
 */
constexpr int LongType = 1;
constexpr int RealType = 2;
constexpr int TextType = 4;
constexpr int GeneralType = 512;
constexpr unsigned Bare = 1U;
constexpr int Unset = 3;
constexpr int None = 7;
constexpr unsigned Texted = 4U;

/**
 *  The flag AutoCAD sets on a cell given a text up to R2007, and the bit of a table's flags for one that flows down.
 */
constexpr int TextOverride = 0x40000;
constexpr unsigned Down = 0x10U;
/**
 *  The bit of a cell's extents that R2010 does not keep.
 */
constexpr unsigned Later = 4U;
/**
 *  The bit of the content's own cell style for a table that flows bottom to top.
 */
constexpr unsigned Upward = 0x10000U;

/**
 *  The height AutoCAD gives the format of a content it makes, and its formats' overrides of the data type and format.
 */
constexpr double Height = 0.18;
constexpr int Typed = 3;

/**
 *  The name of the custom data item a cell of a TABLECONTENT keeps its checksum in.
 */
constexpr std::string_view Summed = "ACAD_ROUNDTRIP_2008_CELL_CHECKSUM";

/**
 *  The groups of a value in a DATATABLE's XRECORD.
 */
constexpr int FlagsGroup = 93;
constexpr int TypeGroup = 90;
constexpr int StringGroup = 1;
constexpr int RealGroup = 40;
constexpr int LongGroup = 91;
constexpr int UnitGroup = 94;
constexpr int FormatGroup = 300;
constexpr int ShownGroup = 302;

auto WriteWide(const std::string &text) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes;
  for (const char16_t unit : Encoding::ToUnits(text) + u'\0')
  {
    bytes.push_back(static_cast<std::byte>(unit & 0xFFU));
    bytes.push_back(static_cast<std::byte>(unit >> 8U));
  }
  return bytes;
}

auto WriteNarrow(const std::string &text, const Encoding::Scheme &scheme) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes;
  for (const char one : Encoding::Encode(scheme, text) + '\0')
  {
    bytes.push_back(static_cast<std::byte>(one));
  }
  return bytes;
}

/**
 *  Up to the NUL.
 */
auto DecodeText(std::span<const std::byte> bytes, bool wide, const Encoding::Scheme &scheme) -> std::string
{
  if (wide)
  {
    std::u16string units;
    for (std::size_t at = 0; at + 1 < bytes.size(); at += 2)
    {
      const auto unit = static_cast<char16_t>(std::to_integer<unsigned>(bytes[at]) |
                                              (std::to_integer<unsigned>(bytes[at + 1]) << 8U));
      if (unit == u'\0')
      {
        break;
      }
      units.push_back(unit);
    }
    return Encoding::FromUnits(units);
  }
  std::string raw;
  for (const std::byte one : bytes)
  {
    if (one == std::byte{})
    {
      break;
    }
    raw.push_back(static_cast<char>(one));
  }
  return Encoding::Decode(scheme, raw);
}

auto IsString(const Value &value) -> bool
{
  return (static_cast<unsigned>(value.Flags) & Bare) == 0 && (value.Type == TextType || value.Type == GeneralType);
}

/**
 *  The fields R2007 added are dropped when downgrading.
 */
void Recode(Value &value, Version to, const Encoding::Scheme &scheme)
{
  const bool wide = to >= Version::R2007;
  if (IsString(value))
  {
    const std::string text = DecodeText(value.Bytes, !wide, scheme);
    value.Bytes = wide ? WriteWide(text) : WriteNarrow(text, scheme);
  }
  if (!wide)
  {
    value.Flags = 0;
    value.Unit = 0;
    value.Format.clear();
    value.Text.clear();
  }
}

/**
 *  Rows, columns and spans are reckoned in 64 bits, so that a broken file's span near a long's limits overflows
 *  nothing.
 */
auto Saturate(std::int64_t number) -> std::int32_t
{
  return static_cast<std::int32_t>(std::clamp<std::int64_t>(number, std::numeric_limits<std::int32_t>::min(),
                                                            std::numeric_limits<std::int32_t>::max()));
}

/**
 *  The range a cell opens, the one that covers it without opening it, or none.
 */
struct MergeSpan
{
  const Range *Opens{};
  bool Covered{};
};

auto MergeSpanOf(const Data &content, std::int32_t row, std::int32_t column) -> MergeSpan
{
  MergeSpan found;
  for (const Range &range : content.Merged)
  {
    if (row < range.Top || row > range.Bottom || column < range.Left || column > range.Right)
    {
      continue;
    }
    if (row == range.Top && column == range.Left)
    {
      found.Opens = &range;
    }
    else
    {
      found.Covered = true;
    }
  }
  return found;
}

void Extend(Cell &cell, const Objects::TableGeometries::Cell &grid)
{
  cell.Sized = 1;
  cell.Unknown1 = grid.Unknown1;
  cell.Width = grid.Width;
  cell.Height = grid.Height;
  cell.Shaped = grid.Contents.empty() ? 0 : 1;
  cell.Unknown2 = grid.Unknown2;
  cell.Geometry = grid.Contents.empty() ? Objects::TableContents::Geometry{} : grid.Contents.front();
}

/**
 *  A cell's extents as AutoCAD leaves those of a cell whose content it set again: zero but for where its content's
 *  center lies.
 */
void ClearExtents(Cell &cell)
{
  cell.Unknown1 = 0;
  cell.Width = 0;
  cell.Height = 0;
  const Objects::TableContents::Geometry kept = cell.Geometry;
  cell.Geometry = {};
  cell.Geometry.Center = kept.Center;
}

/**
 *  The content AutoCAD makes of a grid's cell where it has no other: from R2007 its value, one with no data an empty
 *  one; before R2007 its text as a string; a format that sets the type of a string.
 */
auto GridItem(const Entities::AcadTables::Cell &grid, Version from) -> Item
{
  Item item{.Type = 1, .Value = {}, .Attributes = 0, .Formatted = 1, .Format = {.Height = Height}};
  if (from >= Version::R2007)
  {
    const bool bare = (static_cast<unsigned>(grid.Value.Flags) & Bare) != 0;
    item.Value = bare ? Value{.Flags = Unset} : grid.Value;
    item.Format.Overrides = bare ? 0 : Typed;
    item.Format.Type = bare ? GeneralType : grid.Value.Type;
    return item;
  }
  item.Value = Value{.Flags = 2, .Type = TextType, .Bytes = WriteWide(grid.Text)};
  item.Format.Overrides = grid.Text.empty() ? 0 : Typed;
  return item;
}

/**
 *  The text a grid's cell shows: from R2007 its value's, before its own.
 */
auto GridText(const Entities::AcadTables::Cell &grid, Version from) -> std::string
{
  return from >= Version::R2007 ? TextOf(grid.Value) : grid.Text;
}

/**
 *  Whether a grid's cell was given another text than the one its content's checksum was taken of: AutoCAD keeps the
 *  content's, dwgio the grid's, which an application that knows only the grid may have changed.
 */
auto IsEdited(const Cell &cell, const Entities::AcadTables::Cell &grid, Version from) -> bool
{
  const auto datum = std::ranges::find(cell.Custom, std::string_view{Summed}, &Objects::TableContents::Datum::Name);
  return datum != cell.Custom.end() && datum->Value.Real != Checksum(GridText(grid, from));
}

/**
 *  A cell read from a grid as AutoCAD reads it: from R2007 one with no content given an empty one; before R2007 one
 *  with no content or an empty value given an empty string, its extents unsized. A cell whose grid was edited takes the
 *  grid's content.
 */
void Regrid(Cell &cell, const Entities::AcadTables::Cell &grid, Version from)
{
  if (IsEdited(cell, grid, from))
  {
    cell.Items = {GridItem(grid, from)};
    ClearExtents(cell);
    return;
  }
  if (from >= Version::R2007)
  {
    if (cell.Items.empty())
    {
      cell.Items.push_back(Item{
          .Type = 1, .Value = Value{.Flags = Unset}, .Attributes = 0, .Formatted = 1, .Format = {.Height = Height}});
    }
    return;
  }
  if (!cell.Items.empty() && cell.Items.front().Value.Type != 0)
  {
    return;
  }
  const std::vector<std::byte> empty{std::byte{}, std::byte{}};
  if (cell.Items.empty())
  {
    cell.Items.push_back(Item{.Type = 1,
                              .Value = Value{.Flags = 2, .Type = TextType, .Bytes = empty},
                              .Attributes = 0,
                              .Formatted = 1,
                              .Format = {.Overrides = Typed, .Type = TextType, .Height = Height}});
  }
  else
  {
    cell.Items.front().Value = Value{.Type = GeneralType, .Bytes = empty};
  }
  ClearExtents(cell);
}

/**
 *  A value as AutoCAD writes it in a TABLECONTENT it makes: one with no data with every flag, a string with its flag
 *  and its text.
 */
void Settle(Value &value)
{
  if ((static_cast<unsigned>(value.Flags) & Bare) != 0)
  {
    value.Flags = None;
    return;
  }
  if (IsString(value))
  {
    value.Flags = static_cast<int>(static_cast<unsigned>(value.Flags) | Texted);
    value.Text = TextOf(value);
  }
}

/**
 *  The value a grid cell takes from a cell of the content: its first content's, or one with nothing set.
 */
auto ValueOf(const Cell &cell) -> Value
{
  return cell.Items.empty() ? Value{.Flags = Unset} : cell.Items.front().Value;
}
} // namespace

auto Checksum(const std::string &text) -> double
{
  double sum = 0;
  double place = 1;
  for (const char16_t unit : Encoding::ToUnits(text))
  {
    sum += place * static_cast<double>(unit);
    place += 1;
  }
  return sum;
}

auto TextOf(const Value &value) -> std::string
{
  if (!value.Text.empty() || !IsString(value))
  {
    return value.Text;
  }
  return DecodeText(value.Bytes, true, Encoding::Utf8());
}

void Recode(Data &content, Version to, const Encoding::Scheme &scheme)
{
  const auto custom = [to, &scheme](std::vector<Objects::TableContents::Datum> &items) -> void
  {
    for (Objects::TableContents::Datum &datum : items)
    {
      Recode(datum.Value, to, scheme);
    }
  };
  for (Objects::TableContents::Column &column : content.Columns)
  {
    custom(column.Custom);
  }
  for (Objects::TableContents::Row &row : content.Rows)
  {
    custom(row.Custom);
    for (Cell &cell : row.Cells)
    {
      custom(cell.Custom);
      for (Item &item : cell.Items)
      {
        Recode(item.Value, to, scheme);
      }
    }
  }
}

auto ReadContent(const Entities::AcadTable &table, const Held &held, const Encoding::Scheme &scheme) -> Data
{
  Data content = *held.Content;
  if (held.From < Version::R2007)
  {
    Recode(content, Version::R2007, scheme);
  }
  std::size_t at = 0;
  for (std::size_t row = 0; row < content.Rows.size(); ++row)
  {
    std::vector<Cell> &cells = content.Rows[row].Cells;
    for (std::size_t column = 0; column < cells.size(); ++column, ++at)
    {
      Cell &cell = cells[column];
      if (held.Geometry != nullptr && at < held.Geometry->Cells.size())
      {
        Extend(cell, held.Geometry->Cells[at]);
      }
      const bool covered =
          MergeSpanOf(content, static_cast<std::int32_t>(row), static_cast<std::int32_t>(column)).Covered;
      if (held.From < Version::R2010 && !covered && at < table.Cells.size())
      {
        Regrid(cell, table.Cells[at], held.From);
      }
    }
  }
  return content;
}

void ToContentForm(Entities::AcadTable &table, Data content, Span span)
{
  const auto rows = static_cast<std::int32_t>(content.Rows.size());
  content.TableStyle = {};
  for (Objects::TableContents::Row &row : content.Rows)
  {
    for (Cell &cell : row.Cells)
    {
      // R2010 keeps no bit 4 in a cell's extents, which the geometry keeps.
      cell.Unknown1 = span.To < Version::R2013
                          ? static_cast<std::int32_t>(static_cast<unsigned>(cell.Unknown1) & ~Later)
                          : cell.Unknown1;
    }
  }
  table.Content = std::move(content);
  table.Widths.clear();
  table.Heights.clear();
  table.Cells.clear();
  table.Unknown3 = span.To < Version::R2013 ? 1 : 0;
  if (span.From >= Version::R2010)
  {
    return;
  }
  table.Flags = static_cast<int>(static_cast<unsigned>(table.Flags) & ~Down);
  table.Unknown1 = 0;
  table.Unknown2 = 0;
  table.Broken = 0;
  table.Parts = {Entities::AcadTables::Part{.Position = {}, .Start = 0, .End = rows - 1}};
}

void ToGridForm(Entities::AcadTable &table, const Data &content, Span span)
{
  table.Widths.clear();
  table.Heights.clear();
  table.Cells.clear();
  for (const Objects::TableContents::Column &column : content.Columns)
  {
    table.Widths.push_back(column.Width);
  }
  for (std::size_t row = 0; row < content.Rows.size(); ++row)
  {
    table.Heights.push_back(content.Rows[row].Height);
    for (std::size_t column = 0; column < content.Rows[row].Cells.size(); ++column)
    {
      const MergeSpan merging = MergeSpanOf(content, static_cast<std::int32_t>(row), static_cast<std::int32_t>(column));
      Entities::AcadTables::Cell &cell = table.Cells.emplace_back();
      cell.Merged = merging.Covered;
      cell.Across =
          merging.Opens != nullptr ? Saturate(std::int64_t{merging.Opens->Right} - merging.Opens->Left + 1) : 1;
      cell.Down = merging.Opens != nullptr ? Saturate(std::int64_t{merging.Opens->Bottom} - merging.Opens->Top + 1) : 1;
      const Value value = ValueOf(content.Rows[row].Cells[column]);
      const std::string text = TextOf(value);
      const bool shown = (static_cast<unsigned>(value.Flags) & Bare) == 0 && (value.Type == TextType || !text.empty());
      cell.Overridden = shown;
      cell.Overrides = shown ? TextOverride : 0;
      if (span.To >= Version::R2007)
      {
        cell.Value = value;
      }
      else
      {
        cell.Text = text;
      }
    }
  }
  table.Content = {};
  table.Unknown3 = 0;
  if (span.From < Version::R2010)
  {
    return;
  }
  const bool upward = (static_cast<unsigned>(content.Style.Merge) & Upward) != 0;
  const auto flags = static_cast<unsigned>(table.Flags);
  table.Flags = static_cast<int>(upward ? flags & ~Down : flags | Down);
  table.Unknown1 = 0;
  table.Unknown2 = 0;
  table.Broken = 0;
  table.Parts.clear();
}

auto MakeDataCells(const Data &content) -> std::vector<std::vector<Tag>>
{
  std::vector<std::vector<Tag>> cells;
  for (const Objects::TableContents::Row &row : content.Rows)
  {
    for (const Cell &cell : row.Cells)
    {
      const Value value = ValueOf(cell);
      std::vector<Tag> &groups = cells.emplace_back();
      const bool bare = cell.Items.empty() || (static_cast<unsigned>(value.Flags) & Bare) != 0;
      const std::string text = TextOf(value);
      if (bare)
      {
        groups = {Tag{.Code = FlagsGroup, .Value = std::int64_t{None}},
                  Tag{.Code = TypeGroup, .Value = std::int64_t{0}}};
      }
      else if (value.Type == RealType)
      {
        groups = {Tag{.Code = FlagsGroup, .Value = std::int64_t{RealType}},
                  Tag{.Code = TypeGroup, .Value = std::int64_t{RealType}}, Tag{.Code = RealGroup, .Value = value.Real}};
      }
      else if (value.Type == LongType || value.Type == 0)
      {
        groups = {Tag{.Code = FlagsGroup, .Value = std::int64_t{value.Flags}},
                  Tag{.Code = TypeGroup, .Value = std::int64_t{value.Type}},
                  Tag{.Code = LongGroup, .Value = std::int64_t{value.Long}}};
      }
      else
      {
        groups = {Tag{.Code = FlagsGroup, .Value = std::int64_t{static_cast<unsigned>(value.Flags) | Texted}},
                  Tag{.Code = TypeGroup, .Value = std::int64_t{value.Type}}, Tag{.Code = StringGroup, .Value = text}};
      }
      groups.push_back(Tag{.Code = UnitGroup, .Value = std::int64_t{value.Unit}});
      groups.push_back(Tag{.Code = FormatGroup, .Value = value.Format});
      groups.push_back(Tag{.Code = ShownGroup, .Value = bare ? std::string{} : text});
    }
  }
  return cells;
}

auto MakeContent(const Entities::AcadTable &table, Version from) -> Data
{
  const Objects::TableStyles::CellStyle used{
      .Type = Objects::TableContents::CellKind, .Data = 1, .Content = {.Height = Height}};
  Data content;
  content.Style = Objects::TableStyles::CellStyle{
      .Type = Objects::TableContents::DataKind, .Data = 1, .Content = {.Height = Height}};
  for (const double width : table.Widths)
  {
    content.Columns.push_back(Objects::TableContents::Column{.Width = width});
  }
  std::size_t at = 0;
  for (std::size_t row = 0; row < table.Heights.size(); ++row)
  {
    Objects::TableContents::Row &made = content.Rows.emplace_back();
    made.Height = table.Heights[row];
    made.Id = static_cast<std::int32_t>(std::min<std::size_t>(row, 2)) + 1; // The title 1, the header 2, the data 3.
    for (std::size_t column = 0; column < table.Widths.size(); ++column, ++at)
    {
      const Entities::AcadTables::Cell grid = at < table.Cells.size() ? table.Cells[at] : Entities::AcadTables::Cell{};
      made.Cells.push_back(Cell{.Items = {GridItem(grid, from)}, .Style = used});
      if (grid.Across > 1 || grid.Down > 1)
      {
        const auto top = static_cast<std::int32_t>(row);
        const auto left = static_cast<std::int32_t>(column);
        content.Merged.push_back(Range{.Top = top,
                                       .Left = left,
                                       .Bottom = Saturate(std::int64_t{top} + grid.Down - 1),
                                       .Right = Saturate(std::int64_t{left} + grid.Across - 1)});
      }
    }
  }
  return content;
}

auto MakeStoredContent(const Data &content, Handle style) -> Data
{
  Data stored = content;
  stored.TableStyle = style;
  for (std::size_t row = 0; row < stored.Rows.size(); ++row)
  {
    for (std::size_t column = 0; column < stored.Rows[row].Cells.size(); ++column)
    {
      Cell &cell = stored.Rows[row].Cells[column];
      if (MergeSpanOf(stored, static_cast<std::int32_t>(row), static_cast<std::int32_t>(column)).Covered)
      {
        cell.Items.clear();
      }
      for (Item &item : cell.Items)
      {
        Settle(item.Value);
      }
      const double sum = cell.Items.empty() ? 0.0 : Checksum(TextOf(cell.Items.front().Value));
      std::erase_if(cell.Custom,
                    [](const Objects::TableContents::Datum &datum) -> bool { return datum.Name == Summed; });
      cell.Custom.insert(cell.Custom.begin(), Objects::TableContents::Datum{
                                                  .Name = std::string{Summed},
                                                  .Value = Value{.Flags = RealType, .Type = RealType, .Real = sum}});
      cell.Sized = 0;
      cell.Unknown1 = 0;
      cell.Width = 0;
      cell.Height = 0;
      cell.Shaped = 0;
      cell.Unknown2 = {};
      cell.Geometry = {};
    }
  }
  return stored;
}

auto MakeGeometry(const Data &content) -> Objects::TableGeometry
{
  Objects::TableGeometry geometry{.Base = {},
                                  .Rows = static_cast<std::int32_t>(content.Rows.size()),
                                  .Columns = static_cast<std::int32_t>(content.Columns.size()),
                                  .Cells = {}};
  for (const Objects::TableContents::Row &row : content.Rows)
  {
    for (const Cell &cell : row.Cells)
    {
      Objects::TableGeometries::Cell &made = geometry.Cells.emplace_back();
      made.Unknown1 = cell.Unknown1;
      made.Width = cell.Width;
      made.Height = cell.Height;
      made.Unknown2 = cell.Unknown2;
      if (cell.Shaped != 0)
      {
        made.Contents.push_back(cell.Geometry);
      }
    }
  }
  return geometry;
}
} // namespace DwgIO::Releases::Table
