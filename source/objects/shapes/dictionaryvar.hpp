/***********************************************************************************************************************
 *  @file      source/objects/shapes/dictionaryvar.hpp
 *  @brief     How a DICTIONARYVAR lies in the file, chapter 20.4.74.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_DICTIONARYVAR_HPP
#define DWGIO_OBJECTS_SHAPES_DICTIONARYVAR_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/dictionaryvar.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.74; stored under its class.
 */
template <>
struct Shape<Objects::DictionaryVar>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "DICTIONARYVAR";

  static auto Layout()
  {
    using Objects::DictionaryVar;
    return Fields::Layout(Fields::Byte(&DictionaryVar::Schema), Fields::Text(&DictionaryVar::Value));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_DICTIONARYVAR_HPP */
