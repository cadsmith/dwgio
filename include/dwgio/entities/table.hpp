/***********************************************************************************************************************
 *  @file      dwgio/entities/table.hpp
 *  @brief     The ACAD_TABLE entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_TABLE_HPP
#define DWGIO_ENTITIES_TABLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/tablecontent.hpp"
#include "dwgio/objects/tablestyles/value.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO::Entities::AcadTables
{
/**
 *  The cell override bit AutoCAD sets on a cell given a text, which stores nothing.
 */
inline constexpr int Texted = 0x40000;

/**
 *  A table cell up to R2007.
 */
struct DWGIO_API Cell
{
  int Type{1}; /**< 1 a text cell, 2 a block cell: a table with a block cell is kept as bytes. Group 171. */
  int Edges{}; /**< Which edges have overrides: 1 top, 2 right, 4 bottom, 8 left. Group 172.              */
  bool Merged{};
  bool Autofit{};
  std::int32_t Across{1}; /**< Columns spanned. Group 175.                                    */
  std::int32_t Down{1};   /**< Rows spanned. Group 176.                                       */
  double Rotation{};      /**< Of the content, in radians. Group 145.                         */
  std::string Text{};     /**< The text, up to R2004; from R2007 the value holds it. Group 1. */
  bool Overridden{};
  /**
   *  The overrides: Texted alone is read; a table with another is kept as bytes. Group 177.
   */
  std::int32_t Overrides{};
  int Virtual{};                       /**< Which edges are virtual: 1 top, 2 right, 4 bottom, 8 left. Group 178. */
  std::int32_t Unknown1{};             /**< Unnamed in the specification; 0 as AutoCAD writes it. From R2007.     */
  Objects::TableStyles::Value Value{}; /**< From R2007.                                                           */
  Handle Content{};                    /**< The text's MTEXT; null as AutoCAD writes it. Group 344.               */

  auto operator==(const Cell &) const -> bool = default;
};

/**
 *  A range of rows of a broken table, from R2010.
 */
struct DWGIO_API Part
{
  Vector Position{};
  std::int32_t Start{}; /**< The first row. */
  std::int32_t End{};   /**< The last row.  */

  auto operator==(const Part &) const -> bool = default;
};
} // namespace DwgIO::Entities::AcadTables

namespace DwgIO::Entities
{
/**
 *  An ACAD_TABLE: rows and cells, drawn through an anonymous block it references.
 *
 *  Up to R2007 it keeps its widths, heights and cells itself, from R2010 in its content. A table with what the library
 *  does not read, such as a block cell or an override, is kept as bytes, an Unknown.
 */
struct DWGIO_API AcadTable
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Handle Block{}; /**< The anonymous block that draws the table; DXF names it. Group 2. */
  Vector Position{};
  Vector Scale{.X = 1, .Y = 1, .Z = 1};
  double Rotation{}; /**< In radians. */
  Vector Extrusion{Up};
  bool Follows{}; /**< False: a table with attributes is kept as bytes. Group 66. */
  /**
   *  The flags: 2 and 4 a block, 16 the table flowing down, 32 the title suppressed; from R2010 a short the
   *  specification leaves unknown. Group 90.
   */
  int Flags{};
  Vector Direction{.X = 1}; /**< The direction of the rows. Groups 11, 21, 31. */
  Handle Style{};
  std::vector<double> Widths{};          /**< Up to R2007. Group 142.                                                */
  std::vector<double> Heights{};         /**< Up to R2007. Group 141.                                                */
  std::vector<AcadTables::Cell> Cells{}; /**< Row by row, up to R2007.                                               */
  bool Overridden{};                     /**< False: a table that overrides its style is kept as bytes. Up to R2007. */
  bool Colored{};                        /**< False, as Overridden; up to R2007.                                     */
  bool Weighted{};                       /**< False, as Overridden; up to R2007.                                     */
  bool Shown{};                          /**< False, as Overridden; up to R2007.                                     */
  int Unknown1{};                        /**< Unnamed in the specification; 0 as AutoCAD writes it. From R2010.      */
  std::int32_t Unknown2{};               /**< Unnamed in the specification; 0 as AutoCAD writes it. From R2010.      */
  /**
   *  Unnamed in the specification: a bit in R2010 and a long from R2013; 1 and 0 as AutoCAD writes them.
   */
  std::int32_t Unknown3{};
  Objects::TableContents::Data Content{}; /**< From R2010.                                              */
  std::int32_t Broken{};                  /**< 0: a table with break data is kept as bytes. From R2010. */
  std::vector<AcadTables::Part> Parts{};  /**< From R2010; one as AutoCAD writes it.                    */

  auto operator==(const AcadTable &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_TABLE_HPP */
