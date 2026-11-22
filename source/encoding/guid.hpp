/***********************************************************************************************************************
 *  @file      source/encoding/guid.hpp
 *  @brief     A GUID's parts and the text AutoCAD writes it as.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENCODING_GUID_HPP
#define DWGIO_ENCODING_GUID_HPP

#include <array>
#include <cstdint>
#include <string>

namespace DwgIO::Encoding
{
/**
 *  A long, two shorts and eight bytes, as a DWG file stores a GUID.
 */
struct Guid
{
  std::uint32_t Data1{};
  std::uint16_t Data2{};
  std::uint16_t Data3{};
  std::array<std::uint8_t, 8> Data4{};
};

/**
 *  The GUID in braces, its hexadecimal digits in upper case or in lower case: AutoCAD writes the header's in upper case
 *  and an entity's in lower case.
 */
[[nodiscard]] auto FormatGuid(const Guid &guid, bool upper) -> std::string;
} // namespace DwgIO::Encoding

#endif /* DWGIO_ENCODING_GUID_HPP */
