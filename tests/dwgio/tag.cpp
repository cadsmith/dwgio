/***********************************************************************************************************************
 *  @file      tests/dwgio/tag.cpp
 *  @brief     Comparing extended data groups.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tag.hpp"

#include "octets.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <string>

namespace DwgIO
{
TEST_CASE("tag: equality compares the code and the value")
{
  CHECK(Tag{.Code = 1000, .Value = std::string{"walls"}} == Tag{.Code = 1000, .Value = std::string{"walls"}});
  CHECK_FALSE(Tag{.Code = 1070, .Value = std::int64_t{1}} == Tag{.Code = 1071, .Value = std::int64_t{1}});
  CHECK_FALSE(Tag{.Code = 1005, .Value = Handle{1}} == Tag{.Code = 1005, .Value = std::int64_t{1}});
  CHECK(Tag{.Code = 1004, .Value = Chunk{BytesOf("\x01\x02")}} ==
        Tag{.Code = 1004, .Value = Chunk{BytesOf("\x01\x02")}});
}
} // namespace DwgIO
