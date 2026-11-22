/***********************************************************************************************************************
 *  @file      dwgio/entities/block.hpp
 *  @brief     The BLOCK entity that opens a block's entities.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_BLOCK_HPP
#define DWGIO_ENTITIES_BLOCK_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"

#include <string>

namespace DwgIO::Entities
{
/**
 *  A BLOCK: opens a block's entities; the block record holds the rest.
 */
struct DWGIO_API Block
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  std::string Name{};

  auto operator==(const Block &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_BLOCK_HPP */
