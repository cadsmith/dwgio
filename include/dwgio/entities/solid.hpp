/***********************************************************************************************************************
 *  @file      dwgio/entities/solid.hpp
 *  @brief     The SOLID entity: a filled face of four corners.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SOLID_HPP
#define DWGIO_ENTITIES_SOLID_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/corners.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
/**
 *  A SOLID: a filled face of four corners in its plane, not a 3D solid.
 */
struct DWGIO_API Solid
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  DwgIO::Corners Corners{}; /**< In object coordinates, all at the Z of the first. */
  double Thickness{};
  Vector Extrusion{Up};

  auto operator==(const Solid &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_SOLID_HPP */
