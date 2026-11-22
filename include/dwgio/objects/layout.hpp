/***********************************************************************************************************************
 *  @file      dwgio/objects/layout.hpp
 *  @brief     The LAYOUT object: a tab of the drawing and how it plots.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_LAYOUT_HPP
#define DWGIO_OBJECTS_LAYOUT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/plot.hpp"
#include "dwgio/tables/axes.hpp"
#include "dwgio/vector.hpp"

#include <string>
#include <vector>

namespace DwgIO::Objects
{
/**
 *  A LAYOUT: a tab of the drawing, the model or a paper space sheet, with how it plots.
 */
struct DWGIO_API Layout
{
  DwgIO::Base Base{};
  Objects::Plot Plot{};
  std::string Name{}; /**< Such as "Model" or "Layout1". */
  /**
   *  Bits: 1 to scale linetypes by the viewport, as PSLTSCALE; 2 to keep drawing within the limits, as LIMCHECK. Group
   *  70.
   */
  int Flags{1};
  int Order{}; /**< The tab's place among the layouts; 0 for model space. Group 71. */
  Vector Limmin{};
  Vector Limmax{.X = 12, .Y = 9, .Z = 0};
  Vector Insbase{};
  Vector Extmin{};
  Vector Extmax{};
  Tables::Axes Axes{};             /**< The layout's UCS. Groups 13, 16, 17, 146, 76, 345 and 346.        */
  Handle Block{};                  /**< The BLOCK_RECORD of the layout's space.                           */
  Handle Viewport{};               /**< The viewport last active in the layout.                           */
  Handle PlotView{};               /**< The VIEW plotted when the plot type says so. Group 6, from R2004. */
  Handle VisualStyle{};            /**< From R2007.                                                       */
  std::vector<Handle> Viewports{}; /**< From R2004. Group 330 in a 102 {BLKREFS brace of the record.      */

  auto operator==(const Layout &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_LAYOUT_HPP */
