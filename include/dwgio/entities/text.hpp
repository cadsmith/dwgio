/***********************************************************************************************************************
 *  @file      dwgio/entities/text.hpp
 *  @brief     The TEXT entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_TEXT_HPP
#define DWGIO_ENTITIES_TEXT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/entities/lettering.hpp"

namespace DwgIO::Entities
{
struct DWGIO_API Text
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Lettering Lettering{};

  auto operator==(const Text &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_TEXT_HPP */
