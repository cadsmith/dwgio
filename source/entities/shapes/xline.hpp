/***********************************************************************************************************************
 *  @file      source/entities/shapes/xline.hpp
 *  @brief     How an XLINE lies in the file, chapter 20.4.43.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_XLINE_HPP
#define DWGIO_ENTITIES_SHAPES_XLINE_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/xline.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::XLine>
{
  static constexpr std::array<std::uint16_t, 1> Types{41};
  static constexpr std::string_view Name = "XLINE";

  static auto Layout()
  {
    using Entities::XLine;
    return Fields::Layout(Fields::Point3(&XLine::Start), Fields::Point3(&XLine::Direction));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_XLINE_HPP */
