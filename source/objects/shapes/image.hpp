/***********************************************************************************************************************
 *  @file      source/objects/shapes/image.hpp
 *  @brief     How an IMAGEDEF, its reactor and the raster variables lie in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_IMAGE_HPP
#define DWGIO_OBJECTS_SHAPES_IMAGE_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/image.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.81; stored under its class.
 */
template <>
struct Shape<Objects::ImageDef>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "IMAGEDEF";

  static auto Layout()
  {
    using Objects::ImageDef;
    return Fields::Layout(Fields::Long(&ImageDef::Version), Fields::RawPoint2(&ImageDef::Size),
                          Fields::Text(&ImageDef::File), Fields::Bit(&ImageDef::Loaded), Fields::Byte(&ImageDef::Units),
                          Fields::RawPoint2(&ImageDef::Pixel));
  }
};

/**
 *  Chapter 20.4.82; stored under its class.
 */
template <>
struct Shape<Objects::ImageDefReactor>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "IMAGEDEF_REACTOR";

  static auto Layout() { return Fields::Layout(Fields::Long(&Objects::ImageDefReactor::Version)); }
};

/**
 *  Chapter 20.4.91; stored under its class.
 */
template <>
struct Shape<Objects::RasterVariables>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "RASTERVARIABLES";

  static auto Layout()
  {
    using Objects::RasterVariables;
    return Fields::Layout(Fields::Long(&RasterVariables::Version), Fields::Short(&RasterVariables::Frame),
                          Fields::Short(&RasterVariables::Quality), Fields::Short(&RasterVariables::Units));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_IMAGE_HPP */
