/***********************************************************************************************************************
 *  @file      source/objects/shapes/table/content.hpp
 *  @brief     How a TABLECONTENT lies in the file, chapter 20.4.97.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_TABLE_CONTENT_HPP
#define DWGIO_OBJECTS_SHAPES_TABLE_CONTENT_HPP

#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/table/cellstyle.hpp"
#include "objects/shapes/table/value.hpp"

#include "dwgio/objects/tablecontent.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

/**
 *  What the library does not read, custom data, linked cells, fields, blocks and attributes, stops the read where its
 *  count or type is, which then says why the table is kept as bytes.
 */
namespace DwgIO::Objects::Shapes::Table
{
namespace TableContents = Objects::TableContents;

/**
 *  A count beyond the data fails the reader and leaves the list empty.
 */
template <typename ItemT>
void ReadSize(std::vector<ItemT> &items, const Fields::Input &in)
{
  std::size_t count = 0;
  static_cast<void>(Fields::ReadHandleCount(in, count));
  items.resize(count);
}

/**
 *  False where a value is of a type the library does not read.
 */
inline auto ReadCustom(std::vector<TableContents::Datum> &custom, const Fields::Input &in) -> bool
{
  ReadSize(custom, in);
  return std::ranges::all_of(custom,
                             [&in](TableContents::Datum &datum) -> bool
                             {
                               datum.Name = Fields::TextCode::Read(in);
                               return ReadValue(datum.Value, in);
                             });
}

inline void WriteCustom(const std::vector<TableContents::Datum> &custom, const Fields::Output &to)
{
  to.To->BL(static_cast<std::int32_t>(custom.size()));
  for (const TableContents::Datum &datum : custom)
  {
    Fields::TextCode::Write(datum.Name, to);
    WriteValue(datum.Value, to);
  }
}

inline auto ReadColumn(TableContents::Column &column, const Fields::Input &in) -> bool
{
  column.Name = Fields::TextCode::Read(in);
  column.Data = in.In->BL();
  if (!ReadCustom(column.Custom, in))
  {
    return false;
  }
  ReadCellStyle(column.Style, in);
  column.Id = in.In->BL();
  column.Width = in.In->BD();
  return !in.In->HasFailed();
}

inline void WriteColumn(const TableContents::Column &column, const Fields::Output &to)
{
  Fields::TextCode::Write(column.Name, to);
  to.To->BL(column.Data);
  WriteCustom(column.Custom, to);
  WriteCellStyle(column.Style, to);
  to.To->BL(column.Id);
  to.To->BD(column.Width);
}

/**
 *  A value, with no attributes, and its format when it has one of its own.
 */
inline auto ReadItem(TableContents::Item &item, const Fields::Input &in) -> bool
{
  item.Type = in.In->BL();
  if (item.Type != 1 || !ReadValue(item.Value, in))
  {
    return false;
  }
  item.Attributes = in.In->BL();
  if (item.Attributes != 0)
  {
    return false;
  }
  item.Formatted = in.In->BS();
  if (item.Formatted != 0)
  {
    ReadContent(item.Format, in);
  }
  return !in.In->HasFailed();
}

inline void WriteItem(const TableContents::Item &item, int null, const Fields::Output &to)
{
  to.To->BL(item.Type);
  WriteValue(item.Value, to);
  to.To->BL(item.Attributes);
  to.To->BS(item.Formatted);
  if (item.Formatted != 0)
  {
    WriteContent(item.Format, null, to);
  }
}

inline void ReadGeometry(TableContents::Geometry &geometry, const Fields::Input &in)
{
  geometry.Corner = in.In->Point3();
  geometry.Center = in.In->Point3();
  geometry.ContentWidth = in.In->BD();
  geometry.ContentHeight = in.In->BD();
  geometry.Width = in.In->BD();
  geometry.Height = in.In->BD();
  geometry.Flags = in.In->BL();
}

inline void WriteGeometry(const TableContents::Geometry &geometry, const Fields::Output &to)
{
  to.To->Point3(geometry.Corner);
  to.To->Point3(geometry.Center);
  to.To->BD(geometry.ContentWidth);
  to.To->BD(geometry.ContentHeight);
  to.To->BD(geometry.Width);
  to.To->BD(geometry.Height);
  to.To->BL(geometry.Flags);
}

/**
 *  Only when the cell says they follow: a long, the size, and the geometry behind a handle.
 */
inline void ReadExtents(TableContents::Cell &cell, const Fields::Input &in)
{
  cell.Sized = in.In->BL();
  if (cell.Sized == 0)
  {
    return;
  }
  cell.Unknown1 = in.In->BL();
  cell.Width = in.In->BD();
  cell.Height = in.In->BD();
  cell.Shaped = in.In->BL();
  ReadPointer(cell.Unknown2, in);
  if (cell.Shaped != 0)
  {
    ReadGeometry(cell.Geometry, in);
  }
}

inline void WriteExtents(const TableContents::Cell &cell, const Fields::Output &to)
{
  to.To->BL(cell.Sized);
  if (cell.Sized == 0)
  {
    return;
  }
  to.To->BL(cell.Unknown1);
  to.To->BD(cell.Width);
  to.To->BD(cell.Height);
  to.To->BL(cell.Shaped);
  WritePointer(cell.Unknown2, cell.Shaped != 0 ? HardPointer : SoftPointer, to);
  if (cell.Shaped != 0)
  {
    WriteGeometry(cell.Geometry, to);
  }
}

inline auto ReadCell(TableContents::Cell &cell, const Fields::Input &in) -> bool
{
  cell.State = in.In->BL();
  cell.Tooltip = Fields::TextCode::Read(in);
  cell.Data = in.In->BL();
  if (!ReadCustom(cell.Custom, in))
  {
    return false;
  }
  cell.Linked = in.In->BL();
  if (cell.Linked != 0)
  {
    return false;
  }
  ReadSize(cell.Items, in);
  if (!std::ranges::all_of(cell.Items, [&in](TableContents::Item &item) -> bool { return ReadItem(item, in); }))
  {
    return false;
  }
  ReadCellStyle(cell.Style, in);
  cell.Id = in.In->BL();
  ReadExtents(cell, in);
  return !in.In->HasFailed();
}

inline void WriteCell(const TableContents::Cell &cell, int null, const Fields::Output &to)
{
  to.To->BL(cell.State);
  Fields::TextCode::Write(cell.Tooltip, to);
  to.To->BL(cell.Data);
  WriteCustom(cell.Custom, to);
  to.To->BL(cell.Linked);
  to.To->BL(static_cast<std::int32_t>(cell.Items.size()));
  for (const TableContents::Item &item : cell.Items)
  {
    WriteItem(item, null, to);
  }
  WriteCellStyle(cell.Style, to);
  to.To->BL(cell.Id);
  WriteExtents(cell, to);
}

inline auto ReadRow(TableContents::Row &row, const Fields::Input &in) -> bool
{
  ReadSize(row.Cells, in);
  if (!std::ranges::all_of(row.Cells, [&in](TableContents::Cell &cell) -> bool { return ReadCell(cell, in); }))
  {
    return false;
  }
  row.Data = in.In->BL();
  if (!ReadCustom(row.Custom, in))
  {
    return false;
  }
  ReadCellStyle(row.Style, in);
  row.Id = in.In->BL();
  row.Height = in.In->BD();
  return !in.In->HasFailed();
}

inline void WriteRow(const TableContents::Row &row, int null, const Fields::Output &to)
{
  to.To->BL(static_cast<std::int32_t>(row.Cells.size()));
  for (const TableContents::Cell &cell : row.Cells)
  {
    WriteCell(cell, null, to);
  }
  to.To->BL(row.Data);
  WriteCustom(row.Custom, to);
  WriteCellStyle(row.Style, to);
  to.To->BL(row.Id);
  to.To->BD(row.Height);
}

inline void ReadRange(TableContents::Range &range, const Fields::Input &in)
{
  range.Top = in.In->BL();
  range.Left = in.In->BL();
  range.Bottom = in.In->BL();
  range.Right = in.In->BL();
}

inline void WriteRange(const TableContents::Range &range, const Fields::Output &to)
{
  to.To->BL(range.Top);
  to.To->BL(range.Left);
  to.To->BL(range.Bottom);
  to.To->BL(range.Right);
}

inline auto ReadTableContent(TableContents::Data &content, const Fields::Input &in) -> bool
{
  content.Name = Fields::TextCode::Read(in);
  content.Description = Fields::TextCode::Read(in);
  ReadSize(content.Columns, in);
  if (!std::ranges::all_of(content.Columns,
                           [&in](TableContents::Column &column) -> bool { return ReadColumn(column, in); }))
  {
    return false;
  }
  ReadSize(content.Rows, in);
  if (!std::ranges::all_of(content.Rows, [&in](TableContents::Row &row) -> bool { return ReadRow(row, in); }))
  {
    return false;
  }
  content.Fields = in.In->BL();
  if (content.Fields != 0)
  {
    return false;
  }
  ReadCellStyle(content.Style, in);
  ReadSize(content.Merged, in);
  for (TableContents::Range &range : content.Merged)
  {
    ReadRange(range, in);
  }
  ReadPointer(content.TableStyle, in);
  return !in.In->HasFailed();
}

inline void WriteTableContent(const TableContents::Data &content, int null, const Fields::Output &to)
{
  Fields::TextCode::Write(content.Name, to);
  Fields::TextCode::Write(content.Description, to);
  to.To->BL(static_cast<std::int32_t>(content.Columns.size()));
  for (const TableContents::Column &column : content.Columns)
  {
    WriteColumn(column, to);
  }
  to.To->BL(static_cast<std::int32_t>(content.Rows.size()));
  for (const TableContents::Row &row : content.Rows)
  {
    WriteRow(row, null, to);
  }
  to.To->BL(content.Fields);
  WriteCellStyle(content.Style, to);
  to.To->BL(static_cast<std::int32_t>(content.Merged.size()));
  for (const TableContents::Range &range : content.Merged)
  {
    WriteRange(range, to);
  }
  WritePointer(content.TableStyle, SoftPointer, to);
}

/**
 *  No custom data, linked cell, field, block or attribute stopped the read. The parts that stop it are counted rather
 *  than tested in turn.
 */
inline auto IsReadableContent(const TableContents::Data &content) -> bool
{
  const auto item = [](const TableContents::Item &one) -> int
  {
    return static_cast<int>(one.Type != 1) + static_cast<int>(!IsKnownValue(one.Value)) +
           static_cast<int>(one.Attributes != 0);
  };
  const auto cell = [&item](const TableContents::Cell &one) -> int
  {
    int odd = static_cast<int>(one.Linked != 0);
    for (const TableContents::Item &each : one.Items)
    {
      odd += item(each);
    }
    return odd;
  };
  const auto custom = [](const std::vector<TableContents::Datum> &items) -> int
  {
    return static_cast<int>(std::ranges::count_if(items, [](const TableContents::Datum &datum) -> bool
                                                  { return !IsKnownValue(datum.Value); }));
  };
  int odd = static_cast<int>(content.Fields != 0);
  for (const TableContents::Column &column : content.Columns)
  {
    odd += custom(column.Custom);
  }
  for (const TableContents::Row &row : content.Rows)
  {
    odd += custom(row.Custom);
    for (const TableContents::Cell &each : row.Cells)
    {
      odd += cell(each) + custom(each.Custom);
    }
  }
  return odd == 0;
}

inline auto ReadContentObject(TableContent &content, const Fields::Input &in) -> bool
{
  return ReadTableContent(content.Data, in);
}

inline auto WriteContentObject(const TableContent &content, const Fields::Output &to) -> bool
{
  WriteTableContent(content.Data, HardPointer, to);
  return true;
}
} // namespace DwgIO::Objects::Shapes::Table

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.97; alike in every release but for where its text lies.
 */
template <>
struct Shape<Objects::TableContent>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "TABLECONTENT";

  static auto IsReadable(const Objects::TableContent &content) -> bool
  {
    return Shapes::Table::IsReadableContent(content.Data);
  }

  static auto Layout()
  {
    namespace Table = Shapes::Table;
    return Fields::Layout(Fields::Custom(Table::ReadContentObject, Table::WriteContentObject));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_TABLE_CONTENT_HPP */
