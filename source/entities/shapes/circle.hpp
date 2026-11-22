/***********************************************************************************************************************
 *  @file      source/entities/shapes/circle.hpp
 *  @brief     How a CIRCLE lies in the file, chapter 20.4.20.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_CIRCLE_HPP
#define DWGIO_ENTITIES_SHAPES_CIRCLE_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/circle.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Circle>
{
  static constexpr std::array<std::uint16_t, 1> Types{18};
  static constexpr std::string_view Name = "CIRCLE";

  static auto Layout()
  {
    using Entities::Circle;
    return Fields::Layout(Fields::Point3(&Circle::Center), Fields::Double(&Circle::Radius),
                          Fields::Thickness(&Circle::Thickness), Fields::Extrusion(&Circle::Extrusion));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_CIRCLE_HPP */
