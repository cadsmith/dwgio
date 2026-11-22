/***********************************************************************************************************************
 *  @file      dwgio/entities/helix.hpp
 *  @brief     The HELIX entity: a 3D spiral as a spline and as its axis and turns.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_HELIX_HPP
#define DWGIO_ENTITIES_HELIX_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/entities/spline.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>

namespace DwgIO::Entities
{
struct DWGIO_API HelixData
{
  std::int32_t Major{33};  /**< 33 as AutoCAD writes it. Group 90.           */
  std::int32_t Minor{377}; /**< 377 as AutoCAD writes it. Group 91.          */
  Vector BasePoint{};      /**< The axis's base point, in world coordinates. */
  Vector Start{};          /**< In world coordinates.                        */
  Vector Axis{Up};
  double Radius{1}; /**< The top radius; the base radius is the distance from Start to the axis. Group 40. */
  double Turns{1};
  double Rise{1}; /**< The height of one turn. Group 42. */
  bool RightHanded{true};
  /**
   *  What is kept when the helix is edited: 0 the height of a turn, 1 the turns, 2 the height. Group 280.
   */
  int Constraint{};

  auto operator==(const HelixData &) const -> bool = default;
};

/**
 *  A HELIX: a 3D spiral, kept both as a spline and as its axis and turns, stored under its class.
 */
struct DWGIO_API Helix
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Curve Curve{};
  Entities::HelixData Data{};

  auto operator==(const Helix &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_HELIX_HPP */
