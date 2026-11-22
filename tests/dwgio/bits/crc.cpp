/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/crc.cpp
 *  @brief     The 16-bit CRC of the locator records and sections, against AutoCAD's own.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/crc.hpp"

#include "support/fixtures.hpp"

#include <doctest/doctest.h>

#include <span>

namespace DwgIO::Bits
{
TEST_CASE("bits/crc: the CRC of an AutoCAD R2000 file header with seed C0C1 matches")
{
  const auto file = ReadFixture("foundation-r2000.dwg");
  CHECK(Crc(std::span{file}.first(0x4F), Seed) == 0x302D);
}

TEST_CASE("bits/crc: the specification's seed-zero form agrees on six locator records")
{
  const auto file = ReadFixture("foundation-r2000.dwg");
  CHECK((Crc(std::span{file}.first(0x4F), 0) ^ 0x8461) == 0x302D);
}

TEST_CASE("bits/crc: empty input gives the seed")
{
  CHECK(Crc({}, Seed) == Seed);
}
} // namespace DwgIO::Bits
