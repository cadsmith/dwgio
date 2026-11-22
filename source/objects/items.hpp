/***********************************************************************************************************************
 *  @file      source/objects/items.hpp
 *  @brief     DXF groups read and written as an xrecord stores them, typed by their codes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_ITEMS_HPP
#define DWGIO_OBJECTS_ITEMS_HPP

#include "encoding/encoding.hpp"
#include "fields/context.hpp"

#include "dwgio/tag.hpp"

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace DwgIO::Objects
{
enum class Kind : std::uint8_t
{
  None,
  Text,
  Point,
  Real,
  Short,
  Long,
  Byte,
  Bool,
  Chunk,
  Handle,
  LongLong,
};

/**
 *  As DXF types its groups: text for 0 to 9, 100 to 109, 300 to 309, 410 to 419, 430 to 439, 470 to 479 and 1000 to
 *  1003; a point of three doubles under one group for 10 to 18, 210 to 239 and 1010 to 1013; a double for 38 to 59, 110
 *  to 149, 460 to 469 and 1040 to 1042; a short for 60 to 79, 170 to 179, 270 to 279, 370 to 389, 400 to 409 and 1070;
 *  a long for 90 to 99, 420 to 429, 440 to 459 and 1071; a long long for 160 to 169; a byte for 280 to 289; a bool for
 *  290 to 299; a chunk for 310 to 319 and 1004; a handle for 320 to 369, 390 to 399, 480, 481 and 1005; none for the
 *  rest, the Y and Z codes of a point among them.
 */
[[nodiscard]] auto KindOf(int code) -> Kind;

/**
 *  The code page's scheme, else the context's, else UTF-8.
 */
[[nodiscard]] auto SchemeOf(int page, const Fields::Context &with) -> Encoding::Scheme;

/**
 *  As an xrecord stores them: each a code as an RS then its value, a text as an RS count, an RC code page and the
 *  characters, a point as three RDs under its own code with the two codes above it given to the second and third, a
 *  chunk as an RC count and the bytes, a handle as eight bytes.
 *
 *  The bytes left from the first group that is cut short or of no known kind come back as the second part, with an
 *  issue.
 */
[[nodiscard]] auto ReadGroups(const Fields::Input &in, std::size_t bytes)
    -> std::pair<std::vector<DwgIO::Tag>, std::vector<std::byte>>;

/**
 *  The bytes left go as they were.
 *
 *  A group whose value is not its code's kind, or a point missing its second or third group, is left out and noted; a
 *  chunk over 255 bytes or a text over 65535 is cut and noted, a short, byte or long out of range written as the
 *  nearest and noted.
 */
void WriteGroups(std::span<const DwgIO::Tag> groups, std::span<const std::byte> rest, const Fields::Output &to);
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_ITEMS_HPP */
