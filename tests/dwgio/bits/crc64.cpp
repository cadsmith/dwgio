/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/crc64.cpp
 *  @brief     The normal and mirrored 64-bit CRCs of R2007 files.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/crc64.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <span>
#include <string>

namespace DwgIO::Bits::Crc64
{
TEST_CASE("bits/crc64: the normal and mirrored CRCs of the check string match from zero and from the seeds")
{
  const std::string check = "123456789";
  const std::span<const std::byte> bytes = std::as_bytes(std::span{check});
  CHECK(NormalCrc(bytes, 0) == 0x07b8707ce305d57dU);
  CHECK(MirrorCrc(bytes, 0) == 0x8160a4d154a42879U);
  CHECK(SeedOne(0, bytes.size()) == 0xe7f8c934ffbbfd57U);
  CHECK(SeedTwo(0, bytes.size()) == 0xffbbfc79e7f8c92bU);
  CHECK(NormalCrc(bytes, SeedTwo(0, bytes.size())) == 0xb4268774201869a1U);
  CHECK(MirrorCrc(bytes, SeedOne(0, bytes.size())) == 0x39557f282c02b821U);
  CHECK(NormalCrc({}, 0) == ~std::uint64_t{0});
  CHECK(MirrorCrc({}, 7) == 7U);
}
} // namespace DwgIO::Bits::Crc64
