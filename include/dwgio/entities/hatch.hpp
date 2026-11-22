/***********************************************************************************************************************
 *  @file      dwgio/entities/hatch.hpp
 *  @brief     The HATCH entity: its boundary paths, pattern lines and gradient fill.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_HATCH_HPP
#define DWGIO_ENTITIES_HATCH_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <string>
#include <variant>
#include <vector>

namespace DwgIO::Entities::Edges
{
/**
 *  A vertex of a polyline path, or a control point of a spline edge.
 */
struct DWGIO_API Node
{
  Vector Position{}; /**< In the hatch's own coordinates. */
  /**
   *  The bulge to the next vertex, kept when the path has bulges; or a control point's weight, kept when the spline is
   *  rational. Group 42.
   */
  double Weight{};

  auto operator==(const Node &) const -> bool = default;
};

struct DWGIO_API Line
{
  Vector Start{}; /**< In the hatch's own coordinates. */
  Vector End{};   /**< In the hatch's own coordinates. */

  auto operator==(const Line &) const -> bool = default;
};

struct DWGIO_API Arc
{
  Vector Center{}; /**< In the hatch's own coordinates. */
  double Radius{};
  double Start{};          /**< In radians; group 50 gives degrees. */
  double End{};            /**< In radians; group 51 gives degrees. */
  bool CounterClockwise{}; /**< Group 73.                           */

  auto operator==(const Arc &) const -> bool = default;
};

struct DWGIO_API Ellipse
{
  Vector Center{};         /**< In the hatch's own coordinates.                    */
  Vector Major{};          /**< The end of the major axis, relative to the center. */
  double Ratio{};          /**< The minor axis over the major. Group 40.           */
  double Start{};          /**< In radians; group 50 gives degrees.                */
  double End{};            /**< In radians; group 51 gives degrees.                */
  bool CounterClockwise{}; /**< Group 73.                                          */

  auto operator==(const Ellipse &) const -> bool = default;
};

struct DWGIO_API Spline
{
  int Degree{3};
  bool Rational{};
  bool Periodic{};
  std::vector<double> Knots{};
  std::vector<Node> ControlPoints{}; /**< With their weights when rational.                */
  std::vector<Vector> FitPoints{};   /**< From R2010.                                      */
  Vector StartTangent{};             /**< Kept with fit points, from R2010. Groups 12, 22. */
  Vector EndTangent{};               /**< Kept with fit points, from R2010. Groups 13, 23. */

  auto operator==(const Spline &) const -> bool = default;
};

/**
 *  An edge of a boundary, its kind given by group 72: 1 line, 2 circular arc, 3 elliptic arc, 4 spline.
 */
using Edge = std::variant<Line, Arc, Ellipse, Spline>;
} // namespace DwgIO::Entities::Edges

namespace DwgIO::Entities
{
/**
 *  A boundary path of a hatch: a polyline or a loop of edges, and the entities it was made from.
 */
struct DWGIO_API Path
{
  /**
   *  1 external, 2 polyline, 4 derived, 8 textbox, 16 outermost. Group 92. The 2 bit says which of the two lists below
   *  the file keeps.
   */
  int Flags{};
  bool Bulged{};                       /**< A polyline path keeps a bulge per vertex. Group 72.                      */
  bool Closed{};                       /**< A polyline path closes from its last vertex back to its first. Group 73. */
  std::vector<Edges::Node> Vertices{}; /**< For a polyline path.                                                     */
  std::vector<Edges::Edge> Edges{};    /**< For any other path.                                                      */
  std::vector<Handle> Sources{};       /**< The entities the boundary was made from. Group 330.                      */

  auto operator==(const Path &) const -> bool = default;
};

/**
 *  A line family of a hatch pattern: a line repeated across the hatch, dashed or not.
 */
struct DWGIO_API PatternLine
{
  double Angle{};               /**< In radians; group 53 gives degrees.                */
  double X{};                   /**< A point the first line passes through. Group 43.   */
  double Y{};                   /**< Group 44.                                          */
  double Run{};                 /**< The offset from one line to the next. Group 45.    */
  double Rise{};                /**< Group 46.                                          */
  std::vector<double> Dashes{}; /**< Positive for a dash, negative for a gap. Group 49. */

  auto operator==(const PatternLine &) const -> bool = default;
};

/**
 *  One color of a gradient fill, from R2004.
 */
struct DWGIO_API GradientColor
{
  double Place{}; /**< Along the gradient, 0 to 1. Group 463.                      */
  int Unknown1{}; /**< Unnamed in the specification.                               */
  int Rgb{};      /**< As stored. Group 421.                                       */
  int Unknown2{}; /**< Unnamed in the specification; it says the value is ignored. */

  auto operator==(const GradientColor &) const -> bool = default;
};

/**
 *  The gradient fill of a hatch, from R2004; a hatch without one keeps Shown 0 and the rest at the defaults, which are
 *  what AutoCAD writes then: a blue to yellow linear fill, tint 1, not in use.
 */
struct DWGIO_API Gradient
{
  int Shown{};    /**< Non-zero for a gradient fill. Group 450. */
  int Unknown1{}; /**< Unnamed in the specification. Group 451. */
  double Angle{}; /**< In radians. Group 460.                   */
  double Shift{};
  int Single{};   /**< Non-zero when the gradient fades one color. Group 452. */
  double Tint{1}; /**< Of a single color gradient. Group 462.                 */
  std::vector<GradientColor> Colors{
      GradientColor{.Place = 0, .Unknown1 = 0, .Rgb = static_cast<int>(0xC20000FFU), .Unknown2 = 0},
      GradientColor{.Place = 1, .Unknown1 = 0, .Rgb = static_cast<int>(0xC2FFFF00U), .Unknown2 = 0}};
  std::string Name{"LINEAR"}; /**< Such as LINEAR. Group 470. */

  auto operator==(const Gradient &) const -> bool = default;
};

struct DWGIO_API Hatch
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  double Elevation{}; /**< Along the normal: the Z of group 10. */
  Vector Normal{Up};
  std::string Name{}; /**< Such as ANSI31 or SOLID. Group 2. */
  bool Solid{};
  bool Associative{};
  std::vector<Path> Paths{};
  int Style{};     /**< 0 odd parity, 1 the outermost only, 2 all of it. Group 75. */
  int Kind{1};     /**< 0 user-defined, 1 predefined, 2 custom. Group 76.          */
  double Angle{};  /**< In radians; group 52 gives degrees.                        */
  double Scale{1}; /**< The scale or spacing of a pattern. Group 41.               */
  bool Doubled{};  /**< A user-defined pattern is crosshatched. Group 77.          */
  std::vector<PatternLine> PatternLines{};
  /**
   *  The pixel size AutoCAD computed a derived boundary with, kept when any path's Flags has 4. Group 47.
   */
  double Pixel{};
  std::vector<Vector> Seeds{};   /**< In the hatch's own coordinates. */
  Entities::Gradient Gradient{}; /**< From R2004.                     */

  auto operator==(const Hatch &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_HATCH_HPP */
