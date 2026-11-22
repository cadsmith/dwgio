/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/walk.cpp
 *  @brief     Walking an objects section without its map.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/walk.hpp"

#include "support/fixtures.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/walk: the fixture's objects are found from the pad onwards and nothing where there are none")
{
  const std::vector<std::byte> file = ReadFixture("foundation-r2000.dwg");
  const std::vector<std::size_t> offsets = Walk(file, 21723, 137708);
  CHECK(offsets.size() == 219);
  CHECK(offsets.front() == 21727);
  CHECK(Walk(file, 10, 10).empty());
  static constexpr std::array<std::uint8_t, 3> cut{0x80, 0x80, 0x01};
  CHECK(Walk(std::as_bytes(std::span{cut}), 0, 3).empty());
  const std::vector<std::byte> noise(100, std::byte{0xFF});
  CHECK(Walk(noise, 0, 100).empty());
}

TEST_CASE("objects/walk: an object whose size runs past the end is no object")
{
  static constexpr std::array<std::uint8_t, 5> bytes{0x10, 0x00, 0x01, 0x02, 0x03};
  CHECK(Walk(std::as_bytes(std::span{bytes}), 0, 5).empty());
}

TEST_CASE("objects/walk: an object with no room for its CRC is no object")
{
  static constexpr std::array<std::uint8_t, 5> bytes{0x03, 0x00, 0x01, 0x02, 0x03};
  CHECK(Walk(std::as_bytes(std::span{bytes}), 0, 5).empty());
}
} // namespace DwgIO::Objects
