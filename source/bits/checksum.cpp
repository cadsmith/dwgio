/***********************************************************************************************************************
 *  @file      source/bits/checksum.cpp
 *  @brief     The page checksum of the paged container.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/checksum.hpp"

#include <algorithm>

namespace DwgIO::Bits
{
namespace
{
constexpr std::uint32_t Modulus = 0xFFF1;

constexpr std::size_t Chunk = 0x15B0;
} // namespace

auto Checksum(std::uint32_t seed, std::span<const std::byte> bytes) -> std::uint32_t
{
  std::uint32_t first = seed & 0xFFFFU;
  std::uint32_t second = seed >> 16U;
  while (!bytes.empty())
  {
    const std::size_t chunk = std::min(Chunk, bytes.size());
    for (const std::byte one : bytes.first(chunk))
    {
      first += static_cast<std::uint32_t>(one);
      second += first;
    }
    first %= Modulus;
    second %= Modulus;
    bytes = bytes.subspan(chunk);
  }
  return (second << 16U) | (first & 0xFFFFU);
}
} // namespace DwgIO::Bits
