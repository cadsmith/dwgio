/***********************************************************************************************************************
 *  @file      source/bits/crc32.cpp
 *  @brief     The 32-bit CRC of the paged container's file header.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/crc32.hpp"

#include <array>

namespace DwgIO::Bits
{
namespace
{
constexpr std::uint32_t Polynomial = 0xEDB88320;

constexpr std::size_t Entries = 256;

auto Table() -> std::array<std::uint32_t, Entries>
{
  std::array<std::uint32_t, Entries> table{};
  for (std::size_t at = 0; at < Entries; ++at)
  {
    auto value = static_cast<std::uint32_t>(at);
    for (int bit = 0; bit < 8; ++bit)
    {
      value = (value & 1U) != 0 ? (value >> 1U) ^ Polynomial : value >> 1U;
    }
    table.at(at) = value;
  }
  return table;
}

auto Crcs() -> const std::array<std::uint32_t, Entries> &
{
  static const std::array<std::uint32_t, Entries> table = Table();
  return table;
}
} // namespace

auto Crc32(std::span<const std::byte> bytes, std::uint32_t seed) -> std::uint32_t
{
  std::uint32_t inverted = ~seed;
  for (const std::byte one : bytes)
  {
    inverted = (inverted >> 8U) ^ Crcs().at((inverted ^ static_cast<std::uint32_t>(one)) & 0xFFU);
  }
  return ~inverted;
}
} // namespace DwgIO::Bits
