/***********************************************************************************************************************
 *  @file      source/bits/crc64.hpp
 *  @brief     The 64-bit CRCs of R2007's coded container.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_CRC64_HPP
#define DWGIO_BITS_CRC64_HPP

#include <cstddef>
#include <cstdint>
#include <span>

/**
 *  The 64-bit CRCs of chapter 5.12, which R2007's coded container carries: each eight bytes taken as its pairs 6, 4, 2
 *  and 0, a shorter tail in the order the text's table gives.
 */
namespace DwgIO::Bits::Crc64
{
/**
 *  The normal CRC: ECMA-182's polynomial 0x42F0E1EBA9EA3693 shifted left, the result inverted.
 */
[[nodiscard]] auto NormalCrc(std::span<const std::byte> bytes, std::uint64_t seed) -> std::uint64_t;

/**
 *  The mirrored CRC: the polynomial 0x95AC9329AC4BC9B5 shifted right, the result as it is.
 */
[[nodiscard]] auto MirrorCrc(std::span<const std::byte> bytes, std::uint64_t seed) -> std::uint64_t;

/**
 *  The seed a CRC of `length` bytes starts from, the first way of chapter 5.12.
 */
[[nodiscard]] auto SeedOne(std::uint64_t seed, std::uint64_t length) -> std::uint64_t;

/**
 *  The seed a CRC of `length` bytes starts from, the second way of chapter 5.12.
 */
[[nodiscard]] auto SeedTwo(std::uint64_t seed, std::uint64_t length) -> std::uint64_t;
} // namespace DwgIO::Bits::Crc64

#endif /* DWGIO_BITS_CRC64_HPP */
