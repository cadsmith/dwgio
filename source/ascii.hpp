/***********************************************************************************************************************
 *  @file      source/ascii.hpp
 *  @brief     ASCII case folding for the names AutoCAD compares in any case.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ASCII_HPP
#define DWGIO_ASCII_HPP

#include <algorithm>
#include <string_view>

namespace DwgIO::Ascii
{
constexpr auto Upper(char letter) -> char
{
  return letter >= 'a' && letter <= 'z' ? static_cast<char>(letter - 'a' + 'A') : letter;
}

constexpr auto Lower(char letter) -> char
{
  return letter >= 'A' && letter <= 'Z' ? static_cast<char>(letter - 'A' + 'a') : letter;
}

constexpr auto Same(std::string_view left, std::string_view right) -> bool
{
  return std::ranges::equal(left, right, {}, Upper, Upper);
}
} // namespace DwgIO::Ascii

#endif /* DWGIO_ASCII_HPP */
