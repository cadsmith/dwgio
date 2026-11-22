/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/checksum.cpp
 *  @brief     The page checksum of the paged container: one byte, a chunk boundary, a seed.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/checksum.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace DwgIO::Bits
{
TEST_CASE("bits/checksum: the checksum of nothing and of one byte and across a chunk boundary and from a seed")
{
  CHECK(Checksum(0, {}) == 0U);
  const std::array one{std::byte{1}};
  CHECK(Checksum(0, one) == 0x00010001U);
  const std::vector<std::byte> many(0x15B0 + 1, std::byte{1});
  const std::uint32_t count = 0x15B1;
  CHECK(Checksum(0, many) == ((((count * (count + 1) / 2) % 0xFFF1) << 16U) | (count % 0xFFF1)));
  CHECK(Checksum(0x00020003, one) == 0x00060004U);
}
} // namespace DwgIO::Bits
