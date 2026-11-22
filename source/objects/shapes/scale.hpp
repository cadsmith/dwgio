/***********************************************************************************************************************
 *  @file      source/objects/shapes/scale.hpp
 *  @brief     How a SCALE lies in the file, chapter 20.4.92.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_SCALE_HPP
#define DWGIO_OBJECTS_SHAPES_SCALE_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/scale.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.92; stored under its class.
 */
template <>
struct Shape<Objects::Scale>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "SCALE";

  static auto Layout()
  {
    using Objects::Scale;
    return Fields::Layout(Fields::Short(&Scale::Flags), Fields::Text(&Scale::Name), Fields::Double(&Scale::Paper),
                          Fields::Double(&Scale::Drawing), Fields::Bit(&Scale::Unit));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_SCALE_HPP */
