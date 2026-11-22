/***********************************************************************************************************************
 *  @file      source/file/paged/magic.cpp
 *  @brief     The magic sequence of chapter 4.1.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/magic.hpp"

#include <cstdint>

namespace DwgIO::File::Paged
{
namespace
{
constexpr std::uint32_t Multiplier = 0x343FD;
constexpr std::uint32_t Increment = 0x269EC3;
} // namespace

auto MakeMagic(std::size_t count) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes;
  bytes.reserve(count);
  std::uint32_t seed = 1;
  for (std::size_t at = 0; at < count; ++at)
  {
    seed = (seed * Multiplier) + Increment;
    bytes.push_back(static_cast<std::byte>((seed >> 16U) & 0xFFU));
  }
  return bytes;
}
} // namespace DwgIO::File::Paged
