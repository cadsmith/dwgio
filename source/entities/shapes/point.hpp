/***********************************************************************************************************************
 *  @file      source/entities/shapes/point.hpp
 *  @brief     How a POINT lies in the file, chapter 20.4.31.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_POINT_HPP
#define DWGIO_ENTITIES_SHAPES_POINT_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/point.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Point>
{
  static constexpr std::array<std::uint16_t, 1> Types{27};
  static constexpr std::string_view Name = "POINT";

  static auto Layout()
  {
    using Entities::Point;
    return Fields::Layout(Fields::Point3(&Point::Position), Fields::Thickness(&Point::Thickness),
                          Fields::Extrusion(&Point::Extrusion), Fields::Double(&Point::Angle));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_POINT_HPP */
