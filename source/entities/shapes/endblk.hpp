/***********************************************************************************************************************
 *  @file      source/entities/shapes/endblk.hpp
 *  @brief     How an ENDBLK lies in the file, chapter 20.4.7.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_ENDBLK_HPP
#define DWGIO_ENTITIES_SHAPES_ENDBLK_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/endblk.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::EndBlk>
{
  static constexpr std::array<std::uint16_t, 1> Types{5};
  static constexpr std::string_view Name = "ENDBLK";

  static auto Layout() { return Fields::Layout(); }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_ENDBLK_HPP */
