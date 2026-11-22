/***********************************************************************************************************************
 *  @file      source/entities/shapes/text.hpp
 *  @brief     How a TEXT lies in the file, chapter 20.4.3.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_TEXT_HPP
#define DWGIO_ENTITIES_SHAPES_TEXT_HPP

#include "entities/shapes/lettering.hpp"
#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/text.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Text>
{
  static constexpr std::array<std::uint16_t, 1> Types{1};
  static constexpr std::string_view Name = "TEXT";

  static auto Layout()
  {
    return Fields::Layout(Fields::Part(&Entities::Text::Lettering, Entities::Shapes::LetteringLayout()));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_TEXT_HPP */
