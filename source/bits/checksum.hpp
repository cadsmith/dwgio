/***********************************************************************************************************************
 *  @file      source/bits/checksum.hpp
 *  @brief     The page checksum of the paged container.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_CHECKSUM_HPP
#define DWGIO_BITS_CHECKSUM_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace DwgIO::Bits
{
/**
 *  The checksum of chapter 4.2 that every page of the paged container carries: two running sums modulo 0xFFF1 over
 *  chunks of 0x15B0 bytes, the seed's halves starting them, the second sum in the high half of the result.
 */
[[nodiscard]] auto Checksum(std::uint32_t seed, std::span<const std::byte> bytes) -> std::uint32_t;
} // namespace DwgIO::Bits

#endif /* DWGIO_BITS_CHECKSUM_HPP */
