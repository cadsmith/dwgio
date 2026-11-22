/***********************************************************************************************************************
 *  @file      dwgio/entities/circle.hpp
 *  @brief     The CIRCLE entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_CIRCLE_HPP
#define DWGIO_ENTITIES_CIRCLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
struct DWGIO_API Circle
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  double Thickness{};
  Vector Center{}; /**< In the object coordinate system Extrusion defines; its Z is the elevation. */
  double Radius{};
  Vector Extrusion{Up};

  auto operator==(const Circle &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_CIRCLE_HPP */
