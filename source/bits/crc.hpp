/***********************************************************************************************************************
 *  @file      source/bits/crc.hpp
 *  @brief     The 16-bit CRC a DWG stores after a block.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_CRC_HPP
#define DWGIO_BITS_CRC_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace DwgIO::Bits
{
/**
 *  The seed the file header, the sections and the objects start their CRC from.
 *
 *  The specification seeds zero and XORs a per-block magic into the result instead; both forms give the same number.
 */
constexpr std::uint16_t Seed = 0xC0C1;

/**
 *  The 16-bit CRC a DWG stores after a block: the reflected polynomial 0xA001, each byte folded in from the seed.
 */
[[nodiscard]] auto Crc(std::span<const std::byte> bytes, std::uint16_t seed) -> std::uint16_t;
} // namespace DwgIO::Bits

#endif /* DWGIO_BITS_CRC_HPP */
