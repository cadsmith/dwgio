/***********************************************************************************************************************
 *  @file      source/encoding/guid.cpp
 *  @brief     A GUID's parts and the text AutoCAD writes it as.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/guid.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <string>

namespace DwgIO::Encoding
{
auto FormatGuid(const Guid &guid, bool upper) -> std::string
{
  const std::array<std::uint8_t, 8> &last = guid.Data4;
  std::string text =
      std::format("{{{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}}}", guid.Data1, guid.Data2,
                  guid.Data3, last[0], last[1], last[2], last[3], last[4], last[5], last[6], last[7]);
  if (upper)
  {
    std::ranges::transform(text, text.begin(), [](char one) -> char
                           { return static_cast<char>(std::toupper(static_cast<unsigned char>(one))); });
  }
  return text;
}
} // namespace DwgIO::Encoding
