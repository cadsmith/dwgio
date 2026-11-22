/***********************************************************************************************************************
 *  @file      dwgio/entities/xline.hpp
 *  @brief     The XLINE entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_XLINE_HPP
#define DWGIO_ENTITIES_XLINE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
/**
 *  An XLINE: a line endless both ways.
 */
struct DWGIO_API XLine
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Vector Start{};     /**< In world coordinates. */
  Vector Direction{}; /**< A unit vector.        */

  auto operator==(const XLine &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_XLINE_HPP */
