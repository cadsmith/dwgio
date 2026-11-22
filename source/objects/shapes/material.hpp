/***********************************************************************************************************************
 *  @file      source/objects/shapes/material.hpp
 *  @brief     How a MATERIAL lies in the file: its shading and maps.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_MATERIAL_HPP
#define DWGIO_OBJECTS_SHAPES_MATERIAL_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/material.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects::Shapes
{
inline auto IsOverriding(const Materials::MaterialColor &shade) -> bool
{
  return shade.Method == 1;
}

inline auto IsBitmap(const Materials::Map &map) -> bool
{
  return map.Source == 1;
}

/**
 *  Sixteen BD, row by row.
 */
inline auto ReadTransform(Materials::Map &map, const Fields::Input &in) -> bool
{
  for (double &value : map.Transform)
  {
    value = in.In->BD();
  }
  return !in.In->HasFailed();
}

inline auto WriteTransform(const Materials::Map &map, const Fields::Output &to) -> bool
{
  for (const double value : map.Transform)
  {
    to.To->BD(value);
  }
  return true;
}

/**
 *  The color is stored only when the method overrides the object's.
 */
inline auto ColorLayout()
{
  using Materials::MaterialColor;
  return Fields::Layout(Fields::Byte(&MaterialColor::Method), Fields::Double(&MaterialColor::Factor),
                        Fields::When(IsOverriding, Fields::Long(&MaterialColor::Color)));
}

/**
 *  The file is stored only when the source is an image.
 */
inline auto MapLayout()
{
  using Materials::Map;
  return Fields::Layout(Fields::Double(&Map::Blend), Fields::Byte(&Map::Projection), Fields::Byte(&Map::Tiling),
                        Fields::Byte(&Map::Automatic), Fields::Custom(ReadTransform, WriteTransform),
                        Fields::Byte(&Map::Source), Fields::When(IsBitmap, Fields::Text(&Map::File)));
}

inline auto MakeChannel(Materials::Map Materials::Maps::*map)
{
  return Fields::Part(&Material::Maps, Fields::Layout(Fields::Part(map, MapLayout())));
}
} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
/**
 *  No chapter; stored under its class.
 */
template <>
struct Shape<Objects::Material>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "MATERIAL";

  static auto Layout()
  {
    using Materials::Maps;
    using Objects::Material;
    using Shapes::MakeChannel;
    return Fields::Layout(Fields::Text(&Material::Name), Fields::Text(&Material::Description),
                          Fields::Part(&Material::Ambient, Shapes::ColorLayout()),
                          Fields::Part(&Material::Diffuse, Shapes::ColorLayout()), MakeChannel(&Maps::Diffuse),
                          Fields::Part(&Material::Specular, Shapes::ColorLayout()), MakeChannel(&Maps::Specular),
                          Fields::Double(&Material::Gloss), MakeChannel(&Maps::Reflection),
                          Fields::Double(&Material::Opacity), MakeChannel(&Maps::Opacity), MakeChannel(&Maps::Bump),
                          Fields::Double(&Material::Refraction), MakeChannel(&Maps::Refraction),
                          Fields::Since(Version::R2007, Fields::Double(&Material::Translucence),
                                        Fields::Double(&Material::Illumination),
                                        Fields::Double(&Material::Reflectivity), Fields::Long(&Material::Model),
                                        Fields::Long(&Material::Channels), Fields::Short(&Material::Mode)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_MATERIAL_HPP */
