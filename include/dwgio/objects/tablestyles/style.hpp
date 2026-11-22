/***********************************************************************************************************************
 *  @file      dwgio/objects/tablestyles/style.hpp
 *  @brief     The TABLESTYLE object: how a table draws its title, header and data rows.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_TABLESTYLES_STYLE_HPP
#define DWGIO_OBJECTS_TABLESTYLES_STYLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/tablestyles/cellstyle.hpp"
#include "dwgio/objects/tablestyles/format.hpp"

#include <string>
#include <vector>

namespace DwgIO::Objects::TableStyles
{
/**
 *  One border of a cell, up to R2007.
 *
 *  Its groups depend on the border; each member gives them in the order top, inside horizontal, bottom, left, inside
 *  vertical, right.
 */
struct DWGIO_API Border
{
  /**
   *  The lineweight in hundredths of a millimeter, or -1 ByLayer, -2 ByBlock, -3 the default. Group 274 to 279 by
   *  border.
   */
  int Lineweight{-2};
  bool Visible{true};             /**< Group 284 to 289 by border. */
  DwgIO::Color Color{.Index = 0}; /**< Group 64 to 69 by border.   */

  auto operator==(const Border &) const -> bool = default;
};

/**
 *  How the cells of one kind of row are drawn, up to R2007.
 */
struct DWGIO_API RowStyle
{
  Handle Style{}; /**< DXF names it. Group 7. */
  double Height{4.5};
  int Alignment{1};               /**< 1 to 3 top left to top right, 4 to 6 middle, 7 to 9 bottom. Group 170. */
  DwgIO::Color Color{.Index = 0}; /**< Of the text. Group 62.                                                 */
  DwgIO::Color Fill{.Index = 7};  /**< Of the background. Group 63.                                           */
  bool Filled{};
  int Type{512};        /**< Such as 512 for general. Group 90, from R2007. */
  int Unit{};           /**< Group 91, from R2007.                          */
  std::string Format{}; /**< Group 1, from R2007.                           */
  Border Top{};
  Border Middle{}; /**< The horizontal borders between rows. */
  Border Bottom{};
  Border Left{};
  Border Center{}; /**< The vertical borders between columns. */
  Border Right{};

  auto operator==(const RowStyle &) const -> bool = default;
};

/**
 *  A cell style of a table style from R2010, with the number and name cells, rows and columns know it by.
 */
struct DWGIO_API NamedCellStyle
{
  /**
   *  The number: 1 the title, 2 the header, 3 the data, 4 the table, 101 and up a style of the user's.
   *
   *  The file stores it before the style too, but for the table's own. Group 90.
   */
  int Id{};
  CellStyle Style{};
  int Class{2};       /**< 1 a data style, 2 a label style. Group 91. */
  std::string Name{}; /**< Such as "_TITLE". Group 300.               */

  auto operator==(const NamedCellStyle &) const -> bool = default;
};
} // namespace DwgIO::Objects::TableStyles

namespace DwgIO::Objects
{
/**
 *  A TABLESTYLE: how a table draws its title, header and data rows.
 *
 *  Up to R2007 it keeps three cells and the margins, from R2010 cell styles; a save across R2010 makes one form from
 *  the other.
 */
struct DWGIO_API TableStyle
{
  DwgIO::Base Base{};
  int Revision{};            /**< From R2010; 0 as AutoCAD writes it. Group 280, the first. */
  std::string Description{}; /**< AutoCAD writes the style's name here. Group 3.            */
  int Flow{};                /**< 0 down, 1 up; up to R2007. Group 70.                      */
  /**
   *  The flags, 0 as AutoCAD writes them.
   *
   *  Group 71 up to R2007; from R2010 a long after the description, where AutoCAD's save of an older style to R2018
   *  sets 8 and its save below R2018 clears it.
   */
  int Flags{};
  double Horizontal{1.5};  /**< The margin left and right of text in a cell; up to R2007. Group 40. */
  double Vertical{1.5};    /**< The margin above and below text in a cell; up to R2007. Group 41.   */
  bool TitleSuppressed{};  /**< The title row is left out; up to R2007. Group 280.                  */
  bool HeaderSuppressed{}; /**< The header row is left out; up to R2007. Group 281.                 */
  /**
   *  The data, title and header rows' cells, in that order; up to R2007.
   */
  std::vector<TableStyles::RowStyle> Cells{};
  int Next{101};     /**< From R2010: the number the user's next cell style takes, 101 as AutoCAD writes it. */
  Handle Unknown1{}; /**< Unnamed in the specification; a hard owner, null as AutoCAD writes it. From R2010. */
  TableStyles::NamedCellStyle Overall{}; /**< From R2010: "Table", numbered 4. */
  /**
   *  From R2010: the title, header and data rows' styles, in that order.
   */
  std::vector<TableStyles::NamedCellStyle> Styles{};

  auto operator==(const TableStyle &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_TABLESTYLES_STYLE_HPP */
