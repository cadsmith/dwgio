/***********************************************************************************************************************
 *  @file      dwgio/entities/trace.hpp
 *  @brief     The TRACE entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_TRACE_HPP
#define DWGIO_ENTITIES_TRACE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/corners.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
/**
 *  A TRACE: a filled band of four corners, drawn by the old TRACE command.
 */
struct DWGIO_API Trace
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  DwgIO::Corners Corners{}; /**< In object coordinates, all at the Z of the first. */
  double Thickness{};
  Vector Extrusion{Up};

  auto operator==(const Trace &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_TRACE_HPP */
