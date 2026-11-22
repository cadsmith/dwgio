/***********************************************************************************************************************
 *  @file      dwgio/entities/spline.hpp
 *  @brief     The SPLINE entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SPLINE_HPP
#define DWGIO_ENTITIES_SPLINE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  A NURBS curve, by its knots and control points or by the points it fits: what a SPLINE holds, and a HELIX draws.
 */
struct DWGIO_API Curve
{
  /**
   *  1 closed, 2 periodic, 4 rational, as the file stores them for a spline by control points. Group 70.
   */
  int Flags{};
  /**
   *  From R2013, the file's own flags: 1 by fit points, 2 the control frame shown, 4 closed, 8 the knot parameter used.
   */
  std::int32_t Options{};
  std::int32_t Parameter{}; /**< From R2013: 0 chord, 1 square root, 2 uniform, 15 custom. */
  int Degree{3};            /**< 3 for a new one. Group 71.                                */
  double KnotTolerance{1e-9};
  double ControlTolerance{1e-10};
  /**
   *  The fit tolerance, when the curve is given by fit points; the file then keeps the tangents and the fit points and
   *  nothing of the knots and control points. Group 44.
   */
  std::optional<double> FitTolerance{};
  Vector StartTangent{}; /**< For a curve by fit points. Groups 12, 22, 32. */
  Vector EndTangent{};   /**< For a curve by fit points. Groups 13, 23, 33. */
  std::vector<double> Knots{};
  std::vector<Vector> ControlPoints{}; /**< In world coordinates. */
  /**
   *  The weight of each control point, written when Flags has 4: one short of the control points is written as 1, one
   *  beyond them is dropped. Group 41.
   */
  std::vector<double> Weights{};
  std::vector<Vector> FitPoints{}; /**< In world coordinates. */

  auto operator==(const Curve &) const -> bool = default;
};

struct DWGIO_API Spline
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Curve Curve{};

  auto operator==(const Spline &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_SPLINE_HPP */
