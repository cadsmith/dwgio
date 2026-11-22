/***********************************************************************************************************************
 *  @file      dwgio/tables/vport.hpp
 *  @brief     The VPORT record: a tiled viewport of model space.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_VPORT_HPP
#define DWGIO_TABLES_VPORT_HPP

#include "dwgio/api.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/axes.hpp"
#include "dwgio/tables/record.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Tables
{
/**
 *  A VPORT record: a tiled viewport of model space. The viewports on screen together share the name "*Active".
 */
struct DWGIO_API VPort
{
  Tables::Record Record{};
  Vector Lower{};                        /**< On screen, from 0 to 1 across and up; Z unused. Groups 10, 20.        */
  Vector Upper{.X = 1, .Y = 1, .Z = 0};  /**< On screen, from 0 to 1 across and up; Z unused. Groups 11, 21.        */
  Vector ViewCenter{};                   /**< The view's center, in display coordinates; Z unused. Groups 12, 22.   */
  Vector SnapBase{};                     /**< The snap base point, in display coordinates; Z unused. Groups 13, 23. */
  Vector Snap{.X = 10, .Y = 10, .Z = 0}; /**< Z unused. Groups 14, 24.                                              */
  Vector Grid{.X = 10, .Y = 10, .Z = 0}; /**< Z unused. Groups 15, 25.                                              */
  /**
   *  The direction the view looks from, toward the target, in world coordinates. Groups 16, 26, 36.
   */
  Vector Direction{.X = 0, .Y = 0, .Z = 1};
  Vector Target{};      /**< In world coordinates. Groups 17, 27, 37. */
  double ViewHeight{1}; /**< In display coordinates. Group 40.        */
  /**
   *  In display coordinates; the file stores it where group 41 has the width over the height.
   */
  double ViewWidth{1};
  double LensLength{50}; /**< The lens length of a perspective view, in millimeters. Group 42. */
  double Front{};        /**< An offset from the target. Group 43.                             */
  double Back{};         /**< An offset from the target. Group 44.                             */
  double SnapAngle{};    /**< The snap angle, in radians; group 50 gives degrees.              */
  double Twist{};        /**< About the view direction, in radians; group 51 gives degrees.    */
  /**
   *  Bits, as $VIEWMODE: 1 perspective, 2 front clipping, 4 back clipping, 8 UCS follow, 16 front clip not at the eye.
   *  Group 71.
   */
  int Mode{};
  int Zoom{1000}; /**< As $VIEWRES: how smooth arcs and circles are drawn. Group 72. */
  bool FastZoom{true};
  int UcsIcon{3}; /**< As $UCSICON: 1 shown, 2 at the origin. Group 74. */
  bool SnapOn{};
  bool GridOn{true};
  bool Isometric{}; /**< Isometric rather than square snap. Group 77. */
  int Isoplane{};   /**< 0 left, 1 top, 2 right. Group 78.            */
  /**
   *  0 2D optimized, 1 wireframe, 2 hidden line, 3 flat, 4 Gouraud, 5 and 6 those two with wireframe. Group 281, from
   *  R2000.
   */
  int Render{};
  bool Unknown1{}; /**< Unnamed in the specification. From R2000. */
  /**
   *  The viewport keeps a UCS of its own, made current when the viewport is. Group 65, from R2000.
   */
  bool UcsPerViewport{true};
  Tables::Axes Axes{};        /**< From R2000.                                        */
  bool DefaultLighting{true}; /**< Group 292, from R2007.                             */
  int DefaultLightingType{1}; /**< 1 one distant light, 2 two. Group 282, from R2007. */
  double Brightness{};        /**< Group 141, from R2007.                             */
  double Contrast{};          /**< Group 142, from R2007.                             */
  DwgIO::Color Ambient{.Index = 250, .Rgb = {}, .Transparency = {}}; /**< Group 63, from R2007. */
  Handle Background{};  /**< Group 332, from R2007.                                   */
  Handle VisualStyle{}; /**< Group 348, from R2007.                                   */
  Handle Sun{};         /**< A hard owner. Group 361, from R2007.                     */
  int GridDisplay{};    /**< Group 60, from R2007.                                    */
  int GridMajor{5};     /**< Grid lines between two major ones. Group 61, from R2007. */

  auto operator==(const VPort &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_VPORT_HPP */
