/***********************************************************************************************************************
 *  @file      dwgio/objects/tablegeometry.hpp
 *  @brief     The TABLEGEOMETRY object: where a table's cells lie.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_TABLEGEOMETRY_HPP
#define DWGIO_OBJECTS_TABLEGEOMETRY_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/tablecontent.hpp"

#include <cstdint>
#include <vector>

namespace DwgIO::Objects::TableGeometries
{
/**
 *  Where a table cell lies as AutoCAD last drew it.
 */
struct DWGIO_API Cell
{
  /**
   *  Unnamed in the specification; 3 or 7, as the long before a cell's extents in the table. Group 93.
   */
  std::int32_t Unknown1{};
  double Width{};    /**< Margins included.                                                   */
  double Height{};   /**< Margins included.                                                   */
  Handle Unknown2{}; /**< Unnamed in the specification; null as AutoCAD writes it. Group 330. */
  std::vector<TableContents::Geometry> Contents{};

  auto operator==(const Cell &) const -> bool = default;
};
} // namespace DwgIO::Objects::TableGeometries

namespace DwgIO::Objects
{
/**
 *  A TABLEGEOMETRY: where a table's cells lie.
 *
 *  AutoCAD keeps one beside every table, a hard owner in the record under ACAD_XREC_ROUNDTRIP in the table's extension
 *  dictionary.
 */
struct DWGIO_API TableGeometry
{
  DwgIO::Base Base{};
  std::int32_t Rows{};
  std::int32_t Columns{};
  std::vector<TableGeometries::Cell> Cells{}; /**< Row by row, rows times columns. */

  auto operator==(const TableGeometry &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_TABLEGEOMETRY_HPP */
