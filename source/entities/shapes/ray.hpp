/***********************************************************************************************************************
 *  @file      source/entities/shapes/ray.hpp
 *  @brief     How a RAY lies in the file, chapter 20.4.42.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_RAY_HPP
#define DWGIO_ENTITIES_SHAPES_RAY_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/ray.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Ray>
{
  static constexpr std::array<std::uint16_t, 1> Types{40};
  static constexpr std::string_view Name = "RAY";

  static auto Layout()
  {
    using Entities::Ray;
    return Fields::Layout(Fields::Point3(&Ray::Start), Fields::Point3(&Ray::Direction));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_RAY_HPP */
