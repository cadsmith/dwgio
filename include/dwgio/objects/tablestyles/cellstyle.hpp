/***********************************************************************************************************************
 *  @file      dwgio/objects/tablestyles/cellstyle.hpp
 *  @brief     How a cell, row, column or table is drawn, from R2010.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_TABLESTYLES_CELLSTYLE_HPP
#define DWGIO_OBJECTS_TABLESTYLES_CELLSTYLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/tablestyles/format.hpp"

#include <vector>

namespace DwgIO::Objects::TableStyles
{
/**
 *  The true color bytes of no color, a cell style's background when it has no fill.
 */
inline constexpr int Unfilled = static_cast<int>(0xC8000000U);

/**
 *  A cell style's border on one side, from R2010.
 */
struct DWGIO_API Edge
{
  /**
   *  The side: 1 top, 2 right, 4 bottom, 8 left, 16 inside vertical, 32 inside horizontal; 0 a border with nothing
   *  stored. Group 95.
   */
  int Side{};
  /**
   *  Which properties the border overrides: 1 the type, 2 the lineweight, 4 the linetype, 8 the color, 16 the
   *  visibility, 32 the double line spacing. Group 90.
   */
  int Overrides{};
  int Type{1}; /**< 1 a single line, 2 a double one. Group 91. */
  DwgIO::Color Color{.Index = 0};
  /**
   *  The lineweight in hundredths of a millimeter, or -1 ByLayer, -2 ByBlock, -3 the default. Group 92.
   */
  int Lineweight{-2};
  Handle Linetype{};
  bool Invisible{};
  double Spacing{}; /**< Of a double line. Group 40. */

  auto operator==(const Edge &) const -> bool = default;
};

/**
 *  A cell style's margins, from R2010. Group 40 each.
 */
struct DWGIO_API Margins
{
  double Top{};  /**< Above the content.   */
  double Left{}; /**< Left of the content. */
  double Bottom{};
  double Right{};
  double HorizontalSpacing{}; /**< Between contents side by side.        */
  double VerticalSpacing{};   /**< Between contents one above the other. */

  auto operator==(const Margins &) const -> bool = default;
};

/**
 *  How a cell, row, column or table is drawn, from R2010. A style with no data overrides nothing.
 */
struct DWGIO_API CellStyle
{
  /**
   *  What the style is of: 1 a cell, 2 a row, 3 a column, 4 the formatted table data, 5 a table. Group 90.
   */
  int Type{};
  int Data{}; /**< Whether data follows: 0 none, the style overriding nothing, else 1. Group 170. */
  /**
   *  Which properties the style overrides, as the content's Overrides with the cell style's own from 512. Group 91.
   */
  int Overrides{};
  /**
   *  The merge bits: 0x8000 every cell of a row or column merged, 0x10000 the table flowing bottom to top. Group 92.
   */
  int Merge{};
  DwgIO::Color Background{.Index = 0, .Rgb = Unfilled}; /**< Unfilled for none. Group 62. */
  /**
   *  How several contents are laid out: 1 flowing, 2 stacked across, 4 stacked down. Group 93.
   */
  int Layout{1};
  TableStyles::ContentFormat Content{};
  int Margined{};                 /**< The margin bits: 1 when the margins are stored. Group 171. */
  TableStyles::Margins Margins{}; /**< Stored when Margined has bit 1.                            */
  std::vector<Edge> Borders{};

  auto operator==(const CellStyle &) const -> bool = default;
};
} // namespace DwgIO::Objects::TableStyles

#endif /* DWGIO_OBJECTS_TABLESTYLES_CELLSTYLE_HPP */
