/***********************************************************************************************************************
 *  @file      dwgio/objects/datatable.hpp
 *  @brief     The DATATABLE object: values in named columns.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_DATATABLE_HPP
#define DWGIO_OBJECTS_DATATABLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/tag.hpp"

#include <string>
#include <vector>

namespace DwgIO::Objects::DataTables
{
/**
 *  One column of a data table, its cells as the groups DXF writes them in.
 */
struct DWGIO_API Column
{
  /**
   *  The type of the column's values: 1 integers, a BL each under group 93; 2 reals, a BD each under group 40; 6 hard
   *  owners, a handle each under group 360. Group 92.
   */
  int Kind{};
  std::string Name{};
  std::vector<DwgIO::Tag> Cells{}; /**< Top to bottom, one per row. */

  auto operator==(const Column &) const -> bool = default;
};
} // namespace DwgIO::Objects::DataTables

namespace DwgIO::Objects
{
/**
 *  A DATATABLE: values in named columns, row by row, which AutoCAD keeps beside a table up to R2004.
 *
 *  DXF names it ACDBDATATABLE in R2000; a DWG stores it under the class DATATABLE in every release.
 */
struct DWGIO_API DataTable
{
  DwgIO::Base Base{};
  int Version{2}; /**< 2 as AutoCAD writes it. Group 70. */
  int Rows{};
  std::string Name{};
  std::vector<DataTables::Column> Columns{}; /**< Left to right. */

  auto operator==(const DataTable &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_DATATABLE_HPP */
