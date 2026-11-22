/***********************************************************************************************************************
 *  @file      tests/dwgio/file/locator/sentinels.cpp
 *  @brief     Matching a sentinel's bytes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/sentinels.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

namespace DwgIO::File::Locator
{
TEST_CASE("file/locator/sentinels: a mark matches only where all its bytes are")
{
  std::vector<std::byte> bytes;
  Put(bytes, Sentinels::HeaderEnd);
  CHECK(Matches(bytes, Sentinels::HeaderEnd));
  CHECK_FALSE(Matches(std::span{bytes}.first(3), Sentinels::HeaderEnd));
  bytes.push_back(std::byte{0});
  CHECK(Matches(bytes, Sentinels::HeaderEnd));
}
} // namespace DwgIO::File::Locator
