/***********************************************************************************************************************
 *  @file      dwgio/entities/point.hpp
 *  @brief     The POINT entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_POINT_HPP
#define DWGIO_ENTITIES_POINT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
/**
 *  A POINT: drawn in the style $PDMODE sets.
 */
struct DWGIO_API Point
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Vector Position{}; /**< In world coordinates. */
  double Thickness{};
  Vector Extrusion{Up};
  double Angle{}; /**< The X axis of the point's marker, in radians; group 50 gives degrees. */

  auto operator==(const Point &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_POINT_HPP */
