/***********************************************************************************************************************
 *  @file      source/bits/lz.hpp
 *  @brief     The LZ77 variant of the paged container, chapter 4.7.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_LZ_HPP
#define DWGIO_BITS_LZ_HPP

#include <cstddef>
#include <span>
#include <vector>

namespace DwgIO::Bits::Lz
{
/**
 *  Decompresses a stream of chapter 4.7 onto the end of `out`, whose bytes are the window a back reference may reach
 *  into, at most `most` bytes.
 *
 *  False, with `out` as far as it got, on an opcode below 0x10, a reference before the window, an output beyond `most`,
 *  or an input that ends before the 0x11 that closes a stream. A literal length byte of 0x0F means 18, as 0x01 to 0x0E
 *  mean 4 to 17; the text leaves this out.
 */
[[nodiscard]] auto Decompress(std::span<const std::byte> compressed, std::size_t most, std::vector<std::byte> &out)
    -> bool;

/**
 *  Compresses bytes into a stream `Decompress` turns back into them, closed by 0x11 and two zero bytes, which AutoCAD's
 *  decoder wants.
 *
 *  Fewer than four bytes are padded with zeros to four; the last thirteen bytes are literals, no copy is shorter than
 *  four bytes, and a far copy of ten bytes or more takes the long form.
 */
[[nodiscard]] auto Compress(std::span<const std::byte> bytes) -> std::vector<std::byte>;
} // namespace DwgIO::Bits::Lz

#endif /* DWGIO_BITS_LZ_HPP */
