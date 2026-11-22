/***********************************************************************************************************************
 *  @file      source/bits/strings.hpp
 *  @brief     The string stream of objects, the header and the classes from R2007.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_STRINGS_HPP
#define DWGIO_BITS_STRINGS_HPP

#include "bits/reader.hpp"
#include "bits/writer.hpp"

#include <cstddef>
#include <expected>
#include <optional>
#include <span>
#include <string>

namespace DwgIO::Bits
{
/**
 *  Where a string stream lies in the bits of an object, the header or the classes, from its first bit.
 */
struct Strung
{
  std::size_t Start{};
  std::size_t End{}; /**< The bit after its last, where its size begins. */

  auto operator==(const Strung &) const -> bool = default;
};

/**
 *  From R2007, the string stream of bits whose data ends where the handles start: the bit before them says whether
 *  there is one, the short before that its size in bits, or with 0x8000 the low fifteen bits under another short's high
 *  ones before it, and the stream ends where those shorts begin.
 *
 *  Nothing when the bit is clear or the handles start at 0; `string stream size before the data` or `string stream
 *  before the data` when the sizes reach back past the first bit.
 */
[[nodiscard]] auto FindStrings(std::span<const std::byte> bits, std::size_t handles)
    -> std::expected<std::optional<Strung>, std::string>;

/**
 *  None when there is no stream.
 */
[[nodiscard]] auto MakeStringReader(std::span<const std::byte> bits, const std::optional<Strung> &strung)
    -> std::optional<Reader>;

/**
 *  The bits that follow the data from R2007: the strings written, their size as one short or two, and the bit that says
 *  there are strings; the bit alone, clear, when none were written.
 */
[[nodiscard]] auto WriteStringTail(const Writer &strings) -> Writer;
} // namespace DwgIO::Bits

#endif /* DWGIO_BITS_STRINGS_HPP */
