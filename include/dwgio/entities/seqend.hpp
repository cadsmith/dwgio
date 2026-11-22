/***********************************************************************************************************************
 *  @file      dwgio/entities/seqend.hpp
 *  @brief     The SEQEND entity that closes attributes or vertices.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SEQEND_HPP
#define DWGIO_ENTITIES_SEQEND_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"

namespace DwgIO::Entities
{
/**
 *  A SEQEND: closes an insert's attributes or a polyline's vertices.
 */
struct DWGIO_API SeqEnd
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};

  auto operator==(const SeqEnd &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_SEQEND_HPP */
