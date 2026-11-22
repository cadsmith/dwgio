/***********************************************************************************************************************
 *  @file      source/objects/shapes/placeholder.hpp
 *  @brief     How an ACDBPLACEHOLDER lies in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_PLACEHOLDER_HPP
#define DWGIO_OBJECTS_SHAPES_PLACEHOLDER_HPP

#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/placeholder.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  Nothing beyond the common data.
 */
template <>
struct Shape<Objects::Placeholder>
{
  static constexpr std::array<std::uint16_t, 1> Types{80};
  /**
   *  AutoCAD's files store it under its class before R2004, under the type from it.
   */
  static constexpr Version Fixed = Version::R2004;
  static constexpr std::string_view Name = "ACDBPLACEHOLDER";

  static auto Layout() { return Fields::Layout(); }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_PLACEHOLDER_HPP */
