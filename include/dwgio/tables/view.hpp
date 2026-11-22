/***********************************************************************************************************************
 *  @file      dwgio/tables/view.hpp
 *  @brief     The VIEW record: a named view.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_VIEW_HPP
#define DWGIO_TABLES_VIEW_HPP

#include "dwgio/api.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/axes.hpp"
#include "dwgio/tables/record.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Tables
{
struct DWGIO_API View
{
  Tables::Record Record{};
  double ViewHeight{1}; /**< In display coordinates. Group 40. */
  double ViewWidth{1};  /**< In display coordinates. Group 41. */
  Vector ViewCenter{};  /**< In display coordinates; Z unused. */
  Vector Target{};      /**< In world coordinates.             */
  /**
   *  The direction the view looks from, toward the target, in world coordinates.
   */
  Vector Direction{.X = 0, .Y = 0, .Z = 1};
  double Twist{};        /**< About the view direction, in radians; group 50 gives degrees.    */
  double LensLength{50}; /**< The lens length of a perspective view, in millimeters. Group 42. */
  double Front{};        /**< An offset from the target. Group 43.                             */
  double Back{};         /**< An offset from the target. Group 44.                             */
  /**
   *  Bits, as $VIEWMODE: 1 perspective, 2 front clipping, 4 back clipping, 16 front clip not at the eye. Group 71.
   */
  int Mode{};
  /**
   *  0 2D optimized, 1 wireframe, 2 hidden line, 3 flat, 4 Gouraud, 5 and 6 those two with wireframe. Group 281, from
   *  R2000.
   */
  int Render{};
  bool Paper{};               /**< The 1 bit of group 70.                                                           */
  bool Associated{};          /**< The view keeps a UCS of its own in Axes. Group 72, from R2000.                   */
  Tables::Axes Axes{};        /**< From R2000: the axes stored only when Associated is set, the two handles always. */
  bool DefaultLighting{true}; /**< Group 292, from R2007.                                                           */
  int DefaultLightingType{1}; /**< 1 one distant light, 2 two. Group 282, from R2007.                               */
  double Brightness{};        /**< Group 141, from R2007.                                                           */
  double Contrast{};          /**< Group 142, from R2007.                                                           */
  DwgIO::Color Ambient{.Index = 250, .Rgb = {}, .Transparency = {}}; /**< Group 63, from R2007. */
  Handle Background{};                                               /**< Group 332, from R2007.               */
  Handle VisualStyle{};                                              /**< Group 348, from R2007.               */
  Handle Sun{};                                                      /**< A hard owner. Group 361, from R2007. */
  bool Plottable{};                                                  /**< Group 73, from R2007.                */
  Handle Section{};                                                  /**< Group 334, from R2007.               */

  auto operator==(const View &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_VIEW_HPP */
