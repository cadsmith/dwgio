/***********************************************************************************************************************
 *  @file      source/entities/shapes/light.hpp
 *  @brief     How a LIGHT lies in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_LIGHT_HPP
#define DWGIO_ENTITIES_SHAPES_LIGHT_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/light.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  No chapter; the same in every release and stored under its class.
 */
template <>
struct Shape<Entities::Light>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "LIGHT";

  static auto Layout()
  {
    using Entities::Light;
    return Fields::Layout(
        Fields::Long(&Light::Version), Fields::Text(&Light::Name), Fields::Long(&Light::Kind), Fields::Bit(&Light::Lit),
        Fields::CmColor(&Light::Color), Fields::Bit(&Light::Glyph), Fields::Double(&Light::Intensity),
        Fields::Point3(&Light::Position), Fields::Point3(&Light::Target), Fields::Long(&Light::Fading),
        Fields::Bit(&Light::Limited), Fields::Double(&Light::AttenuationStart), Fields::Double(&Light::AttenuationEnd),
        Fields::Double(&Light::Hotspot), Fields::Double(&Light::Falloff), Fields::Bit(&Light::CastShadows),
        Fields::Long(&Light::Shading), Fields::Short(&Light::Size), Fields::Char(&Light::Softness));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_LIGHT_HPP */
