/***********************************************************************************************************************
 *  @file      dwgio/objects/tablecontent.hpp
 *  @brief     The TABLECONTENT object: a table's rows, columns and cells.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_TABLECONTENT_HPP
#define DWGIO_OBJECTS_TABLECONTENT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/tablestyles/cellstyle.hpp"
#include "dwgio/objects/tablestyles/format.hpp"
#include "dwgio/objects/tablestyles/value.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO::Objects::TableContents
{
/**
 *  The kinds of cell style a table content's parts carry.
 */
inline constexpr int CellKind = 1;
inline constexpr int RowKind = 2;
inline constexpr int ColumnKind = 3;
inline constexpr int DataKind = 4;

/**
 *  An item of the custom data a column, row or cell carries.
 *
 *  AutoCAD gives each cell of a TABLECONTENT the item ACAD_ROUNDTRIP_2008_CELL_CHECKSUM, a double.
 */
struct DWGIO_API Datum
{
  std::string Name{};
  TableStyles::Value Value{};

  auto operator==(const Datum &) const -> bool = default;
};

struct DWGIO_API Column
{
  std::string Name{};
  std::int32_t Data{}; /**< Custom data. Group 91. */
  std::vector<Datum> Custom{};
  TableStyles::CellStyle Style{.Type = ColumnKind}; /**< Overrides of the table style. */
  std::int32_t Id{}; /**< The table style's cell style the column takes; 0 for none. Group 90. */
  double Width{};

  auto operator==(const Column &) const -> bool = default;
};

/**
 *  One content of a cell.
 */
struct DWGIO_API Item
{
  /**
   *  1 a value, 2 a field, 4 a block: a table with a field or a block is kept as bytes. Group 90.
   */
  std::int32_t Type{1};
  TableStyles::Value Value{};
  std::int32_t Attributes{}; /**< How many attributes follow, 0: a table with any is kept as bytes. Group 91. */
  int Formatted{};           /**< Whether the content formats itself, then by Format. Group 170.              */
  TableStyles::ContentFormat Format{};

  auto operator==(const Item &) const -> bool = default;
};

/**
 *  Where a cell's content lies.
 */
struct DWGIO_API Geometry
{
  Vector Corner{}; /**< From the cell's top left corner to the content's.        */
  Vector Center{}; /**< From the cell's top left corner to the content's center. */
  double ContentWidth{};
  double ContentHeight{};
  double Width{};
  double Height{};
  std::int32_t Flags{}; /**< Unnamed in the specification. */

  auto operator==(const Geometry &) const -> bool = default;
};

struct DWGIO_API Cell
{
  /**
   *  The state: 1 content locked, 2 content read only, 4 linked, 8 content modified after an update, 16 format locked,
   *  32 format read only, 64 format modified after an update. Group 90.
   */
  std::int32_t State{};
  std::string Tooltip{};
  std::int32_t Data{}; /**< Custom data. Group 91. */
  std::vector<Datum> Custom{};
  /**
   *  Whether the cell is linked to a data link, 0: a table with a linked cell is kept as bytes. Group 92.
   */
  std::int32_t Linked{};
  std::vector<Item> Items{};
  TableStyles::CellStyle Style{.Type = CellKind}; /**< Overrides of the table style. */
  std::int32_t Id{}; /**< The table style's cell style the cell takes; 0 for none. Group 90. */
  /**
   *  Whether the cell's extents follow, which AutoCAD writes for every cell. Group 91.
   */
  std::int32_t Sized{};
  std::int32_t Unknown1{};            /**< Unnamed in the specification; 3 or 7 as AutoCAD writes it. Group 91. */
  double Width{};                     /**< As drawn. Group 40.                                                  */
  double Height{};                    /**< As drawn. Group 41.                                                  */
  std::int32_t Shaped{};              /**< Whether Geometry follows.                                            */
  Handle Unknown2{};                  /**< Unnamed in the specification; null as AutoCAD writes it.             */
  TableContents::Geometry Geometry{}; /**< When Shaped is set.                                                  */

  auto operator==(const Cell &) const -> bool = default;
};

struct DWGIO_API Row
{
  std::vector<Cell> Cells{}; /**< Left to right.         */
  std::int32_t Data{};       /**< Custom data. Group 91. */
  std::vector<Datum> Custom{};
  TableStyles::CellStyle Style{.Type = RowKind}; /**< Overrides of the table style. */
  /**
   *  The table style's cell style the row takes: 1 the title, 2 the header, 3 the data. Group 90.
   */
  std::int32_t Id{};
  double Height{};

  auto operator==(const Row &) const -> bool = default;
};

/**
 *  A range of merged cells.
 */
struct DWGIO_API Range
{
  std::int32_t Top{};
  std::int32_t Left{};
  std::int32_t Bottom{};
  std::int32_t Right{};

  auto operator==(const Range &) const -> bool = default;
};

/**
 *  What a table's content holds, which a table holds itself from R2010 and its TABLECONTENT in every release.
 */
struct DWGIO_API Data
{
  std::string Name{};
  std::string Description{};
  std::vector<Column> Columns{}; /**< Left to right.                                                     */
  std::vector<Row> Rows{};       /**< Top to bottom.                                                     */
  std::int32_t Fields{};         /**< How many cells hold a field, 0: a table with any is kept as bytes. */
  /**
   *  Overrides of the table style.
   */
  TableStyles::CellStyle Style{.Type = DataKind};
  std::vector<Range> Merged{};
  Handle TableStyle{}; /**< Null as AutoCAD writes it. Group 340. */

  auto operator==(const Data &) const -> bool = default;
};
} // namespace DwgIO::Objects::TableContents

namespace DwgIO::Objects
{
/**
 *  A TABLECONTENT: a table's rows, columns and cells.
 *
 *  AutoCAD keeps one beside every table in every release, a hard owner in the record under ACAD_XREC_ROUNDTRIP in the
 *  table's extension dictionary.
 */
struct DWGIO_API TableContent
{
  DwgIO::Base Base{};
  TableContents::Data Data{};

  auto operator==(const TableContent &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_TABLECONTENT_HPP */
