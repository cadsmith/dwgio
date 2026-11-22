/***********************************************************************************************************************
 *  @file      dwgio/entities/ellipse.hpp
 *  @brief     The ELLIPSE entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_ELLIPSE_HPP
#define DWGIO_ENTITIES_ELLIPSE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
struct DWGIO_API Ellipse
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Vector Center{}; /**< In world coordinates.                                      */
  Vector Axis{};   /**< The semi-major axis from the center, in world coordinates. */
  Vector Extrusion{Up};
  double Ratio{1};  /**< The minor axis as a fraction of the major one.               */
  double Start{};   /**< Parameter angle in radians; 0 for a whole ellipse.           */
  double End{Turn}; /**< Parameter angle in radians; a full turn for a whole ellipse. */

  auto operator==(const Ellipse &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_ELLIPSE_HPP */
