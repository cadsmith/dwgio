/***********************************************************************************************************************
 *  @file      dwgio/opaque.hpp
 *  @brief     The bits of a typed object kept as the file stored them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OPAQUE_HPP
#define DWGIO_OPAQUE_HPP

#include "dwgio/api.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <vector>

namespace DwgIO
{
/**
 *  A run of bits as a stream held them: the bytes, the last one's low bits unused, and how many bits count.
 */
struct DWGIO_API BitString
{
  std::vector<std::byte> Bytes{}; /**< Most significant bit first. */
  std::size_t Size{};             /**< How many bits count.        */

  auto operator==(const BitString &) const -> bool = default;
};

/**
 *  What the library keeps of a typed object as the file stored it, after the fields it types: the rest of its data, of
 *  its handle stream and, from R2007, of its string stream.
 *
 *  A save to the same release writes them back; a save to another cannot, as their layout is the release's.
 */
struct DWGIO_API Opaque
{
  Version Release{}; /**< The release whose layout the bits are in. */
  BitString Data{};
  BitString Handles{}; /**< To the object's end, padding included. */
  BitString Strings{}; /**< From R2007.                            */

  auto operator==(const Opaque &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_OPAQUE_HPP */
