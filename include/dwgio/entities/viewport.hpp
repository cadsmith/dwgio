/***********************************************************************************************************************
 *  @file      dwgio/entities/viewport.hpp
 *  @brief     The VIEWPORT entity: a view of model space on a paper space layout.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_VIEWPORT_HPP
#define DWGIO_ENTITIES_VIEWPORT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/axes.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/vector.hpp"

#include <string>
#include <vector>

namespace DwgIO::Entities
{
struct DWGIO_API Viewport
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Vector Center{}; /**< In paper space coordinates.                          */
  double Width{};  /**< In paper space units.                                */
  double Height{}; /**< In paper space units.                                */
  Vector Target{}; /**< In world coordinates. Groups 17, 27, 37, from R2000. */
  /**
   *  The direction the view looks from, toward its target, in world coordinates. Groups 16, 26, 36, from R2000.
   */
  Vector Direction{Up};
  double Twist{};      /**< In radians; group 51 gives degrees. From R2000.                */
  double ViewHeight{}; /**< The view's height, in model space units. Group 45, from R2000. */
  /**
   *  The lens length of a perspective view, in millimeters. Group 42, from R2000.
   */
  double LensLength{50};
  double Front{};      /**< An offset from the target. Group 43, from R2000.                                */
  double Back{};       /**< An offset from the target. Group 44, from R2000.                                */
  double SnapAngle{};  /**< The snap angle, in radians; group 50 gives degrees. From R2000.                 */
  Vector ViewCenter{}; /**< The view's center, in display coordinates; Z unused. Groups 12, 22, from R2000. */
  Vector SnapBase{};   /**< Z unused. Groups 13, 23, from R2000.                                            */
  Vector Snap{};       /**< Z unused. Groups 14, 24, from R2000.                                            */
  Vector Grid{};       /**< Z unused. Groups 15, 25, from R2000.                                            */
  int Zoom{};          /**< The circle zoom percent. Group 72, from R2000.                                  */
  int GridMajor{5};    /**< Grid lines between two major ones. Group 61, from R2007.                        */
  /**
   *  Layers frozen in this viewport alone; the file counts them before the flags. Group 341, from R2000.
   */
  std::vector<Handle> Frozen{};
  /**
   *  Bits: 1 perspective, 2 front clip, 4 back clip, 8 UCS follows, 16 front clip not at the eye, 32 UCS icon shown, 64
   *  UCS icon at the origin, 128 fast zoom, 256 snap, 512 grid, 1024 isometric snap, 2048 hidden lines plotted, 4096
   *  isopair top, 8192 isopair right, neither the left, 16384 zoom locked, 32768 always, 65536 clipped by an entity,
   *  131072 off, 262144 grid beyond the limits, 524288 adaptive grid, 1048576 the grid subdivided. Group 90, from
   *  R2000.
   */
  int Flags{32768};
  std::string Sheet{}; /**< The plot style sheet's name. Group 1, from R2000. */
  /**
   *  0 2D wireframe, 1 3D wireframe, 2 hidden line, 3 flat shaded, 4 Gouraud shaded, 5 and 6 the same with wireframe.
   *  Group 281, from R2000.
   */
  int Render{};
  bool UcsIconAtOrigin{}; /**< The UCS icon is drawn at the UCS origin. Group 74, from R2000. */
  /**
   *  Whether the viewport keeps its own UCS, made current when it is active. Group 71, from R2000.
   */
  bool UcsPerViewport{true};
  Tables::Axes Axes{};        /**< Groups 110, 111, 112, 146, 79, 345 and 346, from R2000.                          */
  Handle Clip{};              /**< The entity that clips a viewport that is not rectangular. Group 340, from R2000. */
  Handle Header{};            /**< Up to R2000.                                                                     */
  int ShadePlotMode{};        /**< 0 as displayed, 1 wireframe, 2 hidden, 3 rendered. Group 170, from R2004.        */
  bool DefaultLighting{true}; /**< Group 292, from R2007.                                                           */
  int DefaultLightingType{1}; /**< 1 one distant light, 2 two. Group 282, from R2007.                               */
  double Brightness{};        /**< Group 141, from R2007.                                                           */
  double Contrast{};          /**< Group 142, from R2007.                                                           */
  DwgIO::Color Ambient{.Index = 250, .Rgb = {}, .Transparency = {}}; /**< Group 63, from R2007. */
  Handle Background{};      /**< Group 332, from R2007.                                       */
  Handle VisualStyle{};     /**< Group 348, from R2007.                                       */
  Handle ShadePlotObject{}; /**< The object the shade plot comes from. Group 333, from R2007. */
  Handle Sun{};             /**< A hard owner. Group 361, from R2007.                         */

  auto operator==(const Viewport &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_VIEWPORT_HPP */
