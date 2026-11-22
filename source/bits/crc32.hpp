/***********************************************************************************************************************
 *  @file      source/bits/crc32.hpp
 *  @brief     The 32-bit CRC of the paged container's file header.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_CRC32_HPP
#define DWGIO_BITS_CRC32_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace DwgIO::Bits
{
/**
 *  The 32-bit CRC of chapter 2.14.2, which the paged container's file header carries: the reflected polynomial
 *  0xEDB88320, the seed inverted on the way in and the result on the way out, so a CRC chains as a seed.
 */
[[nodiscard]] auto Crc32(std::span<const std::byte> bytes, std::uint32_t seed) -> std::uint32_t;
} // namespace DwgIO::Bits

#endif /* DWGIO_BITS_CRC32_HPP */
