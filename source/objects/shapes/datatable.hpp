/***********************************************************************************************************************
 *  @file      source/objects/shapes/datatable.hpp
 *  @brief     How a DATATABLE lies in the file: its columns and their cells.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_DATATABLE_HPP
#define DWGIO_OBJECTS_SHAPES_DATATABLE_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "objects/shape.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/objects/datatable.hpp"
#include "dwgio/tag.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace DwgIO::Objects::Shapes
{
using DataTables::Column;

/**
 *  The column types the library reads, and the DXF group each type's cells take.
 */
namespace Data
{
constexpr int Integer = 1;
constexpr int Real = 2;
constexpr int Owner = 6;
constexpr int IntegerGroup = 93;
constexpr int RealGroup = 40;
constexpr int OwnerGroup = 360;
constexpr int HardOwner = 3;
} // namespace Data

/**
 *  The three types AutoCAD's tables keep.
 */
inline auto IsKnownColumn(const Column &column) -> bool
{
  return column.Kind == Data::Integer || column.Kind == Data::Real || column.Kind == Data::Owner;
}

inline auto ReadCell(int kind, const Fields::Input &in) -> DwgIO::Tag
{
  if (kind == Data::Integer)
  {
    return DwgIO::Tag{.Code = Data::IntegerGroup, .Value = std::int64_t{in.In->BL()}};
  }
  if (kind == Data::Real)
  {
    return DwgIO::Tag{.Code = Data::RealGroup, .Value = in.In->BD()};
  }
  Handle owned;
  static_cast<void>(Fields::ReadLink(owned, Data::HardOwner, in));
  return DwgIO::Tag{.Code = Data::OwnerGroup, .Value = owned};
}

/**
 *  A value not of the column's type is written as that type's zero.
 */
inline void WriteCell(int kind, const DwgIO::Tag &cell, const Fields::Output &to)
{
  if (kind == Data::Integer)
  {
    const auto *value = std::get_if<std::int64_t>(&cell.Value);
    to.To->BL(static_cast<std::int32_t>(value != nullptr ? *value : 0));
  }
  else if (kind == Data::Real)
  {
    const auto *value = std::get_if<double>(&cell.Value);
    to.To->BD(value != nullptr ? *value : 0.0);
  }
  else
  {
    const auto *value = std::get_if<Handle>(&cell.Value);
    static_cast<void>(Fields::WriteLink(value != nullptr ? *value : Handle{}, Data::HardOwner, to));
  }
}

/**
 *  A column of a type the library does not read stops the read, and the object is kept as bytes.
 */
inline auto ReadDataTable(DataTable &table, const Fields::Input &in) -> bool
{
  const std::int32_t columns = in.In->BL();
  table.Rows = in.In->BL();
  table.Name = Fields::TextCode::Read(in);
  const std::size_t room = Fields::Room(in);
  if (in.In->HasFailed() || columns < 0 || table.Rows < 0 || std::cmp_greater(columns, room) ||
      std::cmp_greater(table.Rows, room))
  {
    in.In->Seek(in.In->Size() + 1);
    return false;
  }
  for (std::int32_t at = 0; at < columns; ++at)
  {
    Column &column = table.Columns.emplace_back();
    column.Kind = in.In->BL();
    column.Name = Fields::TextCode::Read(in);
    if (!IsKnownColumn(column))
    {
      return false;
    }
    for (std::int32_t row = 0; row < table.Rows; ++row)
    {
      column.Cells.push_back(ReadCell(column.Kind, in));
    }
  }
  return !in.In->HasFailed();
}

/**
 *  A missing cell is written as its type's zero.
 */
inline auto WriteDataTable(const DataTable &table, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(table.Columns.size()));
  to.To->BL(table.Rows);
  Fields::TextCode::Write(table.Name, to);
  for (const Column &column : table.Columns)
  {
    to.To->BL(column.Kind);
    Fields::TextCode::Write(column.Name, to);
    for (std::size_t row = 0; std::cmp_less(row, table.Rows); ++row)
    {
      WriteCell(column.Kind, row < column.Cells.size() ? column.Cells[row] : DwgIO::Tag{}, to);
    }
  }
  return true;
}
} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
/**
 *  No chapter; owner cells are in the handle stream. The same in every release from R14 to R2004, the last that keeps
 *  one.
 */
template <>
struct Shape<Objects::DataTable>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "DATATABLE";

  static auto IsReadable(const Objects::DataTable &table) -> bool
  {
    return std::ranges::all_of(table.Columns, Shapes::IsKnownColumn);
  }

  static auto Layout()
  {
    return Fields::Layout(Fields::Short(&DataTable::Version),
                          Fields::Custom(Shapes::ReadDataTable, Shapes::WriteDataTable));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_DATATABLE_HPP */
