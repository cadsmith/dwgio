/***********************************************************************************************************************
 *  @file      dwgio/objects/plot.hpp
 *  @brief     How a layout plots: the page setup a LAYOUT or PLOTSETTINGS carries.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_PLOT_HPP
#define DWGIO_OBJECTS_PLOT_HPP

#include "dwgio/api.hpp"
#include "dwgio/vector.hpp"

#include <string>

namespace DwgIO::Objects
{
/**
 *  How a layout plots: the page setup a LAYOUT or a PLOTSETTINGS carries.
 */
struct DWGIO_API Plot
{
  std::string Name{};
  std::string Device{}; /**< The printer or plot configuration file. Group 2.          */
  std::string Size{};   /**< As the device names it. Group 4.                          */
  std::string View{};   /**< The VIEW plotted when Type is 3. Group 6.                 */
  double Left{};        /**< The unprintable margins, in millimeters. Groups 40 to 43. */
  double Bottom{};
  double Right{};
  double Top{};
  double Width{};         /**< In millimeters. Group 44.                                                 */
  double Height{};        /**< In millimeters. Group 45.                                                 */
  Vector Origin{};        /**< The plot's offset from the paper's origin, in millimeters. Groups 46, 47. */
  Vector Lower{};         /**< Of the window plotted when Type is 4. Groups 48, 49.                      */
  Vector Upper{};         /**< Of the window plotted when Type is 4. Groups 140, 141.                    */
  double PaperUnits{1};   /**< Of a custom scale. Group 142.                                             */
  double DrawingUnits{1}; /**< Of a custom scale. Group 143.                                             */
  /**
   *  Bits: 1 viewport borders, 2 plot styles shown, 4 centered, 8 hidden, 16 standard scale, 32 plot styles, 64
   *  lineweights scaled, 128 lineweights, 512 viewports first, 1024 model space. Group 70.
   */
  int Flags{688};
  int Units{}; /**< 0 inches, 1 millimeters, 2 pixels. Group 72. */
  /**
   *  0 none, 1 a quarter turn counterclockwise, 2 a half turn, 3 a quarter turn clockwise. Group 73.
   */
  int Rotation{};
  /**
   *  What is plotted: 0 the last screen, 1 the extents, 2 the limits, 3 View, 4 the window from Lower to Upper, 5 the
   *  layout. Group 74.
   */
  int Type{5};
  std::string Style{}; /**< Such as "acad.ctb". Group 7. */
  /**
   *  The standard scale: 0 scaled to fit, 16 1:1, others for the scales AutoCAD lists. Group 75.
   */
  int Standard{16};
  double Factor{1}; /**< The standard scale as a number. Group 147.      */
  Vector Image{};   /**< The origin of the paper image. Groups 148, 149. */
  /**
   *  How the plot shades: 0 as displayed, 1 wireframe, 2 hidden, 3 rendered, 4 and up the visual styles. Group 76, from
   *  R2004.
   */
  int Shading{};
  /**
   *  The resolution of a shaded plot: 0 draft, 1 preview, 2 normal, 3 presentation, 4 maximum, 5 custom. Group 77, from
   *  R2004.
   */
  int Resolution{2};
  int Dots{300}; /**< When Resolution is custom. Group 78, from R2004. */

  auto operator==(const Plot &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_PLOT_HPP */
