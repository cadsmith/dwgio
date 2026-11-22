/***********************************************************************************************************************
 *  @file      source/objects/shapes/wipeout.hpp
 *  @brief     How the WIPEOUTVARIABLES lie in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_WIPEOUT_HPP
#define DWGIO_OBJECTS_SHAPES_WIPEOUT_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/wipeout.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  As the raster variables keep theirs; stored under its class.
 */
template <>
struct Shape<Objects::WipeoutVariables>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "WIPEOUTVARIABLES";

  static auto Layout() { return Fields::Layout(Fields::Short(&Objects::WipeoutVariables::Frame)); }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_WIPEOUT_HPP */
