/***********************************************************************************************************************
 *  @file      source/entities/shapes/trace.hpp
 *  @brief     How a TRACE lies in the file, chapter 20.4.36.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_TRACE_HPP
#define DWGIO_ENTITIES_SHAPES_TRACE_HPP

#include "entities/shapes/solid.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/trace.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Trace>
{
  static constexpr std::array<std::uint16_t, 1> Types{32};
  static constexpr std::string_view Name = "TRACE";

  static auto Layout()
  {
    using Entities::Trace;
    return Fields::Layout(Fields::Thickness(&Trace::Thickness),
                          Fields::Custom(Entities::Shapes::ReadCorners<Trace>, Entities::Shapes::WriteCorners<Trace>),
                          Fields::Extrusion(&Trace::Extrusion));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_TRACE_HPP */
