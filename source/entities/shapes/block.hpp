/***********************************************************************************************************************
 *  @file      source/entities/shapes/block.hpp
 *  @brief     How a BLOCK entity lies in the file, chapter 20.4.6.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_BLOCK_HPP
#define DWGIO_ENTITIES_SHAPES_BLOCK_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/block.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Block>
{
  static constexpr std::array<std::uint16_t, 1> Types{4};
  static constexpr std::string_view Name = "BLOCK";

  static auto Layout() { return Fields::Layout(Fields::Text(&Entities::Block::Name)); }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_BLOCK_HPP */
