/***********************************************************************************************************************
 *  @file      source/bits/crc64.cpp
 *  @brief     The 64-bit CRCs of R2007's coded container.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/crc64.hpp"

#include "bits/shuffle.hpp"

#include <array>

namespace DwgIO::Bits::Crc64
{
namespace
{
constexpr std::size_t Entries = 256;
constexpr std::uint64_t Forward = 0x42F0E1EBA9EA3693;
constexpr std::uint64_t Backward = 0x95AC9329AC4BC9B5;
constexpr std::uint64_t Top = std::uint64_t{1} << 63U;
constexpr unsigned Shift = 56;
constexpr std::uint64_t Multiplier = 0x343FD;
constexpr std::uint64_t Increment = 0x269EC3;
constexpr unsigned Half = 32;

auto Table(bool mirrored) -> std::array<std::uint64_t, Entries>
{
  std::array<std::uint64_t, Entries> table{};
  for (std::size_t at = 0; at < Entries; ++at)
  {
    std::uint64_t value = mirrored ? at : static_cast<std::uint64_t>(at) << Shift;
    for (int bit = 0; bit < 8; ++bit)
    {
      if (mirrored)
      {
        value = (value & 1U) != 0 ? (value >> 1U) ^ Backward : value >> 1U;
      }
      else
      {
        value = (value & Top) != 0 ? (value << 1U) ^ Forward : value << 1U;
      }
    }
    table.at(at) = value;
  }
  return table;
}

auto Normals() -> const std::array<std::uint64_t, Entries> &
{
  static const std::array<std::uint64_t, Entries> table = Table(false);
  return table;
}

auto Mirrors() -> const std::array<std::uint64_t, Entries> &
{
  static const std::array<std::uint64_t, Entries> table = Table(true);
  return table;
}

auto Mix(std::uint64_t seed, std::uint64_t length) -> std::uint64_t
{
  return ((seed + length) * Multiplier) + Increment;
}
} // namespace

auto NormalCrc(std::span<const std::byte> bytes, std::uint64_t seed) -> std::uint64_t
{
  std::uint64_t crc = seed;
  Shuffle(bytes, [&crc](std::byte one) -> void
          { crc = Normals().at(((crc >> Shift) ^ std::to_integer<std::uint64_t>(one)) & 0xFFU) ^ (crc << 8U); });
  return ~crc;
}

auto MirrorCrc(std::span<const std::byte> bytes, std::uint64_t seed) -> std::uint64_t
{
  std::uint64_t crc = seed;
  Shuffle(bytes, [&crc](std::byte one) -> void
          { crc = Mirrors().at((crc ^ std::to_integer<std::uint64_t>(one)) & 0xFFU) ^ (crc >> 8U); });
  return crc;
}

auto SeedOne(std::uint64_t seed, std::uint64_t length) -> std::uint64_t
{
  std::uint64_t mixed = Mix(seed, length);
  mixed |= (mixed * (Multiplier << Half)) + (Increment << Half);
  return ~mixed;
}

auto SeedTwo(std::uint64_t seed, std::uint64_t length) -> std::uint64_t
{
  const std::uint64_t mixed = Mix(seed, length);
  return ~((mixed * ((std::uint64_t{1} << Half) + Multiplier)) + length + Increment);
}
} // namespace DwgIO::Bits::Crc64
