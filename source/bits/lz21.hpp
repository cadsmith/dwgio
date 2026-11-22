/***********************************************************************************************************************
 *  @file      source/bits/lz21.hpp
 *  @brief     The compression and page checksum of R2007's coded container.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_LZ21_HPP
#define DWGIO_BITS_LZ21_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

/**
 *  The compression and the page checksum of R2007's coded container, chapter 5.
 */
namespace DwgIO::Bits::Lz21
{
/**
 *  Decompresses a stream of chapter 5.10 into at most `most` bytes.
 *
 *  A literal run's opcode below 0x10 gives its length less eight, 0x0F asking for more bytes; its bytes are stored in
 *  32-byte chunks whose blocks the text's table orders, two- and three-byte blocks reversed and a sixteen-byte block's
 *  halves swapped, which the table does not say. A first opcode of 0x2X skips two bytes and takes the first run's
 *  length from the third. Nothing on a reference before the start, an output beyond `most` or an input that ends inside
 *  an instruction.
 */
[[nodiscard]] auto Decompress(std::span<const std::byte> compressed, std::size_t most)
    -> std::optional<std::vector<std::byte>>;

/**
 *  Compresses bytes into a stream Decompress reads back: literal runs and back references within 0xFFFF bytes found by
 *  a hash of three bytes, a reference taking the shortest of the forms its length and distance allow. Nothing is
 *  compressed from an empty input.
 */
[[nodiscard]] auto Compress(std::span<const std::byte> bytes) -> std::vector<std::byte>;

/**
 *  The page checksum of chapter 5.4.1: two running sums modulo 0xFFF1 over chunks of 0x15B0 bytes, each eight-byte
 *  group taken as its pairs 6, 4, 2, 0, the seed and the length starting the sums.
 */
[[nodiscard]] auto Checksum(std::uint64_t seed, std::span<const std::byte> bytes) -> std::uint32_t;
} // namespace DwgIO::Bits::Lz21

#endif /* DWGIO_BITS_LZ21_HPP */
