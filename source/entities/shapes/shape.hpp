/***********************************************************************************************************************
 *  @file      source/entities/shapes/shape.hpp
 *  @brief     How a SHAPE lies in the file, chapter 20.4.37.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_SHAPE_HPP
#define DWGIO_ENTITIES_SHAPES_SHAPE_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/shape.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Shape>
{
  static constexpr std::array<std::uint16_t, 1> Types{33};
  static constexpr std::string_view Name = "SHAPE";

  static auto Layout()
  {
    return Fields::Layout(Fields::Point3(&Entities::Shape::Position), Fields::Double(&Entities::Shape::Size),
                          Fields::Double(&Entities::Shape::Rotation), Fields::Double(&Entities::Shape::Width),
                          Fields::Double(&Entities::Shape::Oblique), Fields::Double(&Entities::Shape::Thickness),
                          Fields::Short(&Entities::Shape::Number), Fields::Point3(&Entities::Shape::Extrusion),
                          Fields::Link(&Entities::Shape::Style, 5));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_SHAPE_HPP */
