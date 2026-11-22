/***********************************************************************************************************************
 *  @file      source/tables/shapes/viewportheader.hpp
 *  @brief     How a VP_ENT_HDR record lies in the file, chapter 20.4.70.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_VIEWPORTHEADER_HPP
#define DWGIO_TABLES_SHAPES_VIEWPORTHEADER_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/viewportheader.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Tables::ViewportEntityHeader>
{
  static constexpr std::array<std::uint16_t, 1> Types{71};
  static constexpr std::string_view Name = "VP_ENT_HDR";
  static constexpr int SoftPointer = 4;

  static auto Layout()
  {
    using Tables::ViewportEntityHeader;
    return Fields::Layout(Fields::Part(&ViewportEntityHeader::Record, Tables::Shapes::RecordLayout()),
                          Fields::Bit(&ViewportEntityHeader::Flag),
                          Fields::Link(&ViewportEntityHeader::Viewport, SoftPointer),
                          Fields::Link(&ViewportEntityHeader::Previous, Tables::Shapes::HardPointer));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_VIEWPORTHEADER_HPP */
