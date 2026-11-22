/***********************************************************************************************************************
 *  @file      source/encoding/units.hpp
 *  @brief     UTF-16 code units to UTF-8 and back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENCODING_UNITS_HPP
#define DWGIO_ENCODING_UNITS_HPP

#include <string>
#include <string_view>

namespace DwgIO::Encoding
{
/**
 *  UTF-16 code units as UTF-8: a surrogate pair one point, a surrogate on its own U+FFFD.
 */
[[nodiscard]] auto FromUnits(std::u16string_view units) -> std::string;

/**
 *  UTF-8 as UTF-16 code units: a point past the first plane a pair, a damaged sequence U+FFFD.
 */
[[nodiscard]] auto ToUnits(std::string_view text) -> std::u16string;
} // namespace DwgIO::Encoding

#endif /* DWGIO_ENCODING_UNITS_HPP */
