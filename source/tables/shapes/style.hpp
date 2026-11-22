/***********************************************************************************************************************
 *  @file      source/tables/shapes/style.hpp
 *  @brief     How a STYLE record lies in the file, chapter 20.4.56.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_STYLE_HPP
#define DWGIO_TABLES_SHAPES_STYLE_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/style.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.56; the shape file bit comes before the vertical bit, the reverse of the chapter's order.
 */
template <>
struct Shape<Tables::Style>
{
  static constexpr std::array<std::uint16_t, 1> Types{53};
  static constexpr std::string_view Name = "STYLE";

  static auto Layout()
  {
    using Tables::Style;
    return Fields::Layout(Fields::Part(&Style::Record, Tables::Shapes::RecordLayout()), Fields::Bit(&Style::Shape),
                          Fields::Bit(&Style::Vertical), Fields::Double(&Style::Height), Fields::Double(&Style::Width),
                          Fields::Double(&Style::Oblique), Fields::Byte(&Style::Generation),
                          Fields::Double(&Style::LastHeight), Fields::Text(&Style::Font),
                          Fields::Text(&Style::Bigfont));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_STYLE_HPP */
