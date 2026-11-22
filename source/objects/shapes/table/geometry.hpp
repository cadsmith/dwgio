/***********************************************************************************************************************
 *  @file      source/objects/shapes/table/geometry.hpp
 *  @brief     How a TABLEGEOMETRY lies in the file, chapter 20.4.103.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_TABLE_GEOMETRY_HPP
#define DWGIO_OBJECTS_SHAPES_TABLE_GEOMETRY_HPP

#include "fields/context.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/table/cellstyle.hpp"
#include "objects/shapes/table/content.hpp"

#include "dwgio/objects/tablegeometry.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace DwgIO::Objects::Shapes::Table
{
inline void ReadGridCell(TableGeometries::Cell &cell, const Fields::Input &in)
{
  cell.Unknown1 = in.In->BL();
  cell.Width = in.In->BD();
  cell.Height = in.In->BD();
  ReadPointer(cell.Unknown2, in);
  ReadSize(cell.Contents, in);
  for (TableContents::Geometry &geometry : cell.Contents)
  {
    ReadGeometry(geometry, in);
  }
}

inline void WriteGridCell(const TableGeometries::Cell &cell, const Fields::Output &to)
{
  to.To->BL(cell.Unknown1);
  to.To->BD(cell.Width);
  to.To->BD(cell.Height);
  WritePointer(cell.Unknown2, SoftPointer, to);
  to.To->BL(static_cast<std::int32_t>(cell.Contents.size()));
  for (const TableContents::Geometry &geometry : cell.Contents)
  {
    WriteGeometry(geometry, to);
  }
}

inline auto ReadGrid(TableGeometry &geometry, const Fields::Input &in) -> bool
{
  geometry.Rows = in.In->BL();
  geometry.Columns = in.In->BL();
  ReadSize(geometry.Cells, in);
  for (TableGeometries::Cell &cell : geometry.Cells)
  {
    ReadGridCell(cell, in);
  }
  return !in.In->HasFailed();
}

inline auto WriteGrid(const TableGeometry &geometry, const Fields::Output &to) -> bool
{
  to.To->BL(geometry.Rows);
  to.To->BL(geometry.Columns);
  to.To->BL(static_cast<std::int32_t>(geometry.Cells.size()));
  for (const TableGeometries::Cell &cell : geometry.Cells)
  {
    WriteGridCell(cell, to);
  }
  return true;
}
} // namespace DwgIO::Objects::Shapes::Table

namespace DwgIO::Objects
{
template <>
struct Shape<Objects::TableGeometry>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "TABLEGEOMETRY";

  static auto Layout()
  {
    namespace Table = Shapes::Table;
    return Fields::Layout(Fields::Custom(Table::ReadGrid, Table::WriteGrid));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_TABLE_GEOMETRY_HPP */
