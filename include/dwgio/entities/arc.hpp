/***********************************************************************************************************************
 *  @file      dwgio/entities/arc.hpp
 *  @brief     The ARC entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_ARC_HPP
#define DWGIO_ENTITIES_ARC_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
/**
 *  An ARC: drawn counterclockwise from its start angle to its end angle.
 */
struct DWGIO_API Arc
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  double Thickness{};
  Vector Center{}; /**< In the object coordinate system Extrusion defines; its Z is the elevation. */
  double Radius{};
  Vector Extrusion{Up};
  double Start{}; /**< In radians; group 50 gives degrees. */
  double End{};   /**< In radians; group 51 gives degrees. */

  auto operator==(const Arc &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_ARC_HPP */
