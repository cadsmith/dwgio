/***********************************************************************************************************************
 *  @file      tests/dwgio/handle.cpp
 *  @brief     Handles as hex text, their order and hash.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/handle.hpp"

#include <doctest/doctest.h>

#include <functional>

namespace DwgIO
{
TEST_CASE("handle: a handle is written in upper-case hex")
{
  CHECK(HexOf(Handle{0x2c20a}) == "2C20A");
  CHECK(HexOf(Handle{}) == "0");
}

TEST_CASE("handle: a handle is read from hex of either case with blanks ignored")
{
  CHECK(ParseHandle("1F") == Handle{0x1f});
  CHECK(ParseHandle("1f") == Handle{0x1f});
  CHECK(ParseHandle(" 2C20A") == Handle{0x2c20a});
  CHECK(ParseHandle("FFFFFFFFFFFFFFFF") == Handle{0xffffffffffffffff});
  CHECK(ParseHandle("0") == Handle{});
}

TEST_CASE("handle: text that is not hex of at most sixteen digits is no handle")
{
  CHECK_FALSE(ParseHandle(""));
  CHECK_FALSE(ParseHandle("ARROW"));
  CHECK_FALSE(ParseHandle("1G"));
  CHECK_FALSE(ParseHandle("10000000000000000"));
}

TEST_CASE("handle: handle 0 is null and handles order and hash by value")
{
  CHECK_FALSE(Handle{});
  CHECK(Handle{1});
  CHECK(Handle{1} < Handle{2});
  CHECK(std::hash<Handle>{}(Handle{7}) == std::hash<Handle>{}(Handle{7}));
}

TEST_CASE("handle: text that starts with no hex digit is no handle")
{
  CHECK_FALSE(ParseHandle("zz"));
}
} // namespace DwgIO
