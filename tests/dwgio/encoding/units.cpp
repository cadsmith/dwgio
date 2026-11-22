/***********************************************************************************************************************
 *  @file      tests/dwgio/encoding/units.cpp
 *  @brief     UTF-16 code units to and from UTF-8, surrogates alone as U+FFFD.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/units.hpp"

#include <doctest/doctest.h>

#include <string>

namespace DwgIO::Encoding
{
TEST_CASE("encoding/units: UTF-16 code units decode to UTF-8 with a surrogate pair as one code point")
{
  CHECK(FromUnits(u"Hi") == "Hi");
  CHECK(FromUnits(u"éあ") == "\xc3\xa9\xe3\x81\x82");
  CHECK(FromUnits(u"\xd83d\xde00") == "\xf0\x9f\x98\x80");
  CHECK(FromUnits(u"") == "");
}

TEST_CASE("encoding/units: a lone surrogate decodes to U+FFFD")
{
  CHECK(FromUnits(u"a\xd83d") == "a\xef\xbf\xbd");
  CHECK(FromUnits(u"\xde00"
                  u"b") == "\xef\xbf\xbd"
                           "b");
  CHECK(FromUnits(u"\xd83d\x0041") == "\xef\xbf\xbd"
                                      "A");
}

TEST_CASE("encoding/units: UTF-8 encodes to UTF-16 with a code point past the BMP as a pair")
{
  CHECK(ToUnits("Hi") == u"Hi");
  CHECK(ToUnits("\xc3\xa9\xe3\x81\x82") == u"éあ");
  CHECK(ToUnits("\xf0\x9f\x98\x80") == u"\xd83d\xde00");
  CHECK(ToUnits("a\xff") == u"a�");
}

TEST_CASE("encoding/units: a unit past the surrogates is a code point of its own")
{
  CHECK(FromUnits(u"\ue000") == "\xee\x80\x80");
  CHECK(FromUnits(u"\xd83d\ue000") == "\xef\xbf\xbd\xee\x80\x80");
}
} // namespace DwgIO::Encoding
