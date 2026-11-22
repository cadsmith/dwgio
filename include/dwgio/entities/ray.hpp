/***********************************************************************************************************************
 *  @file      dwgio/entities/ray.hpp
 *  @brief     The RAY entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_RAY_HPP
#define DWGIO_ENTITIES_RAY_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
struct DWGIO_API Ray
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Vector Start{};     /**< In world coordinates. */
  Vector Direction{}; /**< A unit vector.        */

  auto operator==(const Ray &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_RAY_HPP */
