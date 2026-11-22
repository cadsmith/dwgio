/***********************************************************************************************************************
 *  @file      source/entities/shapes/arc.hpp
 *  @brief     How an ARC lies in the file, chapter 20.4.18.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_ARC_HPP
#define DWGIO_ENTITIES_SHAPES_ARC_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/arc.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Arc>
{
  static constexpr std::array<std::uint16_t, 1> Types{17};
  static constexpr std::string_view Name = "ARC";

  static auto Layout()
  {
    using Entities::Arc;
    return Fields::Layout(Fields::Point3(&Arc::Center), Fields::Double(&Arc::Radius),
                          Fields::Thickness(&Arc::Thickness), Fields::Extrusion(&Arc::Extrusion),
                          Fields::Double(&Arc::Start), Fields::Double(&Arc::End));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_ARC_HPP */
