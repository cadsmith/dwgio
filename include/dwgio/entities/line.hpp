/***********************************************************************************************************************
 *  @file      dwgio/entities/line.hpp
 *  @brief     The LINE entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_LINE_HPP
#define DWGIO_ENTITIES_LINE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
struct DWGIO_API Line
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  double Thickness{};
  Vector Start{}; /**< In world coordinates. */
  Vector End{};   /**< In world coordinates. */
  Vector Extrusion{Up};

  auto operator==(const Line &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_LINE_HPP */
