/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/crc32.cpp
 *  @brief     The 32-bit CRC of the paged container's file header: its check value and chaining.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/crc32.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <span>
#include <string>

namespace DwgIO::Bits
{
TEST_CASE("bits/crc32: the check value and chaining match")
{
  const std::string check = "123456789";
  const std::span<const std::byte> bytes = std::as_bytes(std::span{check});
  CHECK(Crc32(bytes, 0) == 0xCBF43926U);
  CHECK(Crc32({}, 0) == 0U);
  const std::uint32_t half = Crc32(bytes.first(4), 0);
  CHECK(Crc32(bytes.subspan(4), half) == 0xCBF43926U);
}
} // namespace DwgIO::Bits
