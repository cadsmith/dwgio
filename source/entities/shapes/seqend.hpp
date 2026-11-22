/***********************************************************************************************************************
 *  @file      source/entities/shapes/seqend.hpp
 *  @brief     How a SEQEND lies in the file, chapter 20.4.8.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_SEQEND_HPP
#define DWGIO_ENTITIES_SHAPES_SEQEND_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/seqend.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::SeqEnd>
{
  static constexpr std::array<std::uint16_t, 1> Types{6};
  static constexpr std::string_view Name = "SEQEND";

  static auto Layout() { return Fields::Layout(); }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_SEQEND_HPP */
