/***********************************************************************************************************************
 *  @file      source/encoding/points.hpp
 *  @brief     UTF-8 sequences to code points and back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENCODING_POINTS_HPP
#define DWGIO_ENCODING_POINTS_HPP

#include <cstddef>
#include <string>
#include <string_view>

namespace DwgIO::Encoding
{
struct Step
{
  char32_t Point{};
  std::size_t Length{};
};

[[nodiscard]] auto First(std::string_view text) -> Step;

void Put(std::string &out, char32_t point);
} // namespace DwgIO::Encoding

#endif /* DWGIO_ENCODING_POINTS_HPP */
