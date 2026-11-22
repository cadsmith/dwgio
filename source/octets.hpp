/***********************************************************************************************************************
 *  @file      source/octets.hpp
 *  @brief     The bytes of a text and the text of bytes, for the blobs the API holds as bytes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OCTETS_HPP
#define DWGIO_OCTETS_HPP

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO
{
inline auto BytesOf(std::string_view text) -> std::vector<std::byte>
{
  const std::span<const std::byte> bytes = std::as_bytes(std::span{text});
  return {bytes.begin(), bytes.end()};
}

inline auto TextOf(std::span<const std::byte> bytes) -> std::string
{
  std::string text(bytes.size(), '\0');
  std::ranges::transform(bytes, text.begin(), [](std::byte one) -> char { return static_cast<char>(one); });
  return text;
}
} // namespace DwgIO

#endif /* DWGIO_OCTETS_HPP */
