/***********************************************************************************************************************
 *  @file      dwgio/entities/endblk.hpp
 *  @brief     The ENDBLK entity that closes a block's entities.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_ENDBLK_HPP
#define DWGIO_ENTITIES_ENDBLK_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"

namespace DwgIO::Entities
{
/**
 *  An ENDBLK: closes a block's entities.
 */
struct DWGIO_API EndBlk
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};

  auto operator==(const EndBlk &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_ENDBLK_HPP */
