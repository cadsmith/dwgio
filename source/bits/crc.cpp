/***********************************************************************************************************************
 *  @file      source/bits/crc.cpp
 *  @brief     The 16-bit CRC a DWG stores after a block.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/crc.hpp"

#include <array>

namespace DwgIO::Bits
{
namespace
{
constexpr unsigned Polynomial = 0xA001;

constexpr std::size_t Entries = 256;

constexpr unsigned Width = 8;

consteval auto Entry(std::size_t index) -> std::uint16_t
{
  auto crc = static_cast<unsigned>(index);
  for (unsigned bit = 0; bit < Width; ++bit)
  {
    const bool low = (crc & 1U) != 0;
    crc >>= 1U;
    if (low)
    {
      crc ^= Polynomial;
    }
  }
  return static_cast<std::uint16_t>(crc);
}

consteval auto MakeTable() -> std::array<std::uint16_t, Entries>
{
  std::array<std::uint16_t, Entries> table{};
  for (std::size_t index = 0; index < Entries; ++index)
  {
    table.at(index) = Entry(index);
  }
  return table;
}

constexpr std::array Table = MakeTable();
} // namespace

auto Crc(std::span<const std::byte> bytes, std::uint16_t seed) -> std::uint16_t
{
  unsigned crc = seed;
  for (const std::byte one : bytes)
  {
    const std::size_t index = (crc ^ static_cast<unsigned>(one)) & 0xFFU;
    crc = (crc >> Width) ^ Table.at(index);
  }
  return static_cast<std::uint16_t>(crc);
}
} // namespace DwgIO::Bits
