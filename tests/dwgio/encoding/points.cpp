/***********************************************************************************************************************
 *  @file      tests/dwgio/encoding/points.cpp
 *  @brief     Code points read from and appended as UTF-8, damage as U+FFFD.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/points.hpp"

#include <doctest/doctest.h>

#include <string>
#include <string_view>

namespace DwgIO::Encoding
{
TEST_CASE("encoding/points: a code point decodes from UTF-8 with its length")
{
  CHECK(First("a").Point == U'a');
  CHECK(First("\xc3\xa9").Point == U'é');
  CHECK(First("\xc3\xa9").Length == 2);
  CHECK(First("\xe2\x82\xac").Point == U'€');
  CHECK(First("\xf0\x9f\x98\x80").Point == U'\U0001F600');
  CHECK(First("\xf0\x9f\x98\x80").Length == 4);
}

TEST_CASE("encoding/points: damaged UTF-8 decodes to U+FFFD one byte long")
{
  CHECK(First("\xc3").Point == U'�');
  CHECK(First("\xc3(").Point == U'�');
  CHECK(First("\xc0\xaf").Point == U'�');
  CHECK(First("\xed\xa0\x80").Point == U'�');
  CHECK(First("\xf4\x90\x80\x80").Point == U'�');
  CHECK(First("\xff").Length == 1);
}

TEST_CASE("encoding/points: a code point is appended as UTF-8")
{
  std::string out;
  for (const char32_t point : {U'a', U'é', U'€', U'\U0001F600'})
  {
    Put(out, point);
  }
  CHECK(out == "a\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80");
}

TEST_CASE("encoding/points: a code point past Unicode is written as U+FFFD")
{
  std::string out;
  Put(out, char32_t{0x110000});
  CHECK(out == "\xef\xbf\xbd");
}
} // namespace DwgIO::Encoding
