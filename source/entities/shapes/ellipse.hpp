/***********************************************************************************************************************
 *  @file      source/entities/shapes/ellipse.hpp
 *  @brief     How an ELLIPSE lies in the file, chapter 20.4.39.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_ELLIPSE_HPP
#define DWGIO_ENTITIES_SHAPES_ELLIPSE_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/ellipse.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Ellipse>
{
  static constexpr std::array<std::uint16_t, 1> Types{35};
  static constexpr std::string_view Name = "ELLIPSE";

  static auto Layout()
  {
    using Entities::Ellipse;
    return Fields::Layout(Fields::Point3(&Ellipse::Center), Fields::Point3(&Ellipse::Axis),
                          Fields::Point3(&Ellipse::Extrusion), Fields::Double(&Ellipse::Ratio),
                          Fields::Double(&Ellipse::Start), Fields::Double(&Ellipse::End));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_ELLIPSE_HPP */
