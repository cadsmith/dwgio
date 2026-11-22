/***********************************************************************************************************************
 *  @file      dwgio/unknown.hpp
 *  @brief     An object of a kind dwgio does not type, kept as bytes and bits.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_UNKNOWN_HPP
#define DWGIO_UNKNOWN_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/symbol.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace DwgIO
{
/**
 *  An object of a kind the library does not type: its common header read, its bytes kept, and its own bits apart from
 *  that header.
 *
 *  A save to the same release writes the bytes back, or, when they are cleared, the header again from Base and Common
 *  followed by the own bits: clear them after changing either. A save to another release writes the header in that
 *  release's form and the own bits after it where AutoCAD writes the class the same way in both, and otherwise leaves
 *  the object out with an issue.
 */
struct DWGIO_API Unknown
{
  std::uint16_t Type{}; /**< A fixed type code, or 500 and up for a class. */
  /**
   *  The class's DXF name for a type of 500 and up; empty for a fixed type or a class the file lacks.
   */
  Symbol Class{};
  bool Entity{}; /**< By its fixed type or its class. */
  DwgIO::Base Base{};
  DwgIO::Common Common{};         /**< Meaningful when Entity is set.            */
  std::vector<std::byte> Bytes{}; /**< As stored, from its size through its CRC. */
  /**
   *  What follows the common header, in the release read: the object's own data, the rest of its handle stream without
   *  the padding that fills the object to the byte, and from R2007 its string stream. Empty when the header could not
   *  be read whole.
   */
  std::optional<Opaque> Body{};

  auto operator==(const Unknown &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_UNKNOWN_HPP */
