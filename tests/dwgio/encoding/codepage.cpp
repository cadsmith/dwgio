/***********************************************************************************************************************
 *  @file      tests/dwgio/encoding/codepage.cpp
 *  @brief     Code pages found by name, and their bytes and points.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/codepage.hpp"

#include <doctest/doctest.h>

namespace DwgIO::Encoding
{
TEST_CASE("encoding/codepage: a page is found by its header name in any case")
{
  REQUIRE(Find("ANSI_1256") != nullptr);
  CHECK(Find("ansi_1256") == Find("ANSI_1256"));
  CHECK(Find("1251") == Find("ANSI_1251"));
  CHECK(Find("ANSI_874") != nullptr);
  CHECK(Pages().size() == 15);
  CHECK(Standard().Name == "ANSI_1252");
}

TEST_CASE("encoding/codepage: a page dwgio lacks gives none")
{
  CHECK(Find("ANSI_1200") == nullptr);
  CHECK(Find("DOS850") == nullptr);
  CHECK(Find("") == nullptr);
  CHECK(Find("ANSI_") == nullptr);
  CHECK(Find("ANSI") == nullptr);
  CHECK(Find("ANSI_ANSI_932") == nullptr);
  CHECK(Find("ANSI_93") == nullptr);
}

TEST_CASE("encoding/codepage: the prefix and the number match without case")
{
  REQUIRE(Find("ANSI_932") != nullptr);
  CHECK(Find("Ansi_932") == Find("ANSI_932"));
  CHECK(Find("932") == Find("ANSI_932"));
  CHECK(Find("ANSI_1361")->Name == "ANSI_1361");
}

TEST_CASE("encoding/codepage: a byte maps to its code point and back")
{
  const Codepage &latin = Standard();
  const Codepage &arabic = *Find("ANSI_1256");
  CHECK(DecodeByte(latin, 'a') == U'a');
  CHECK(DecodeByte(latin, 0xe9) == U'é');
  CHECK(DecodeByte(latin, 0x80) == U'€');
  CHECK(DecodeByte(latin, 0x81) == U'\u0081');
  CHECK(DecodeByte(arabic, 0xc7) == U'ا');
  CHECK(EncodeByte(latin, U'é') == '\xe9');
  CHECK(EncodeByte(arabic, U'ا') == '\xc7');
  CHECK_FALSE(EncodeByte(latin, U'ا'));
}

TEST_CASE("encoding/codepage: the East Asian pages take two bytes a character")
{
  const Codepage &japanese = *Find("ANSI_932");
  CHECK(DecodePair(japanese, 0x82, 0xa0) == U'\u3042');
  CHECK(DecodePair(japanese, 0x81, 0x5c) == U'\u2015');
  CHECK(DecodeByte(japanese, 0xb1) == U'\uff71');
  CHECK(DecodePair(*Find("ANSI_936"), 0xd6, 0xd0) == U'\u4e2d');
  CHECK(DecodePair(*Find("ANSI_949"), 0xb0, 0xa1) == U'\uac00');
  CHECK(DecodePair(*Find("ANSI_950"), 0xa4, 0xa4) == U'\u4e2d');
  CHECK(DecodePair(*Find("ANSI_1361"), 0x88, 0x61) == U'\uac00');
  CHECK(EncodePair(japanese, U'\u3042') == Twin{'\x82', '\xa0'});
  CHECK(EncodePair(*Find("ANSI_936"), U'\u4e2d') == Twin{'\xd6', '\xd0'});
}

TEST_CASE("encoding/codepage: a pair outside the table or in a single-byte page maps to nothing")
{
  const Codepage &japanese = *Find("ANSI_932");
  CHECK(DecodePair(Standard(), 0x82, 0xa0) == 0);
  CHECK(DecodePair(japanese, 0x41, 0xa0) == 0);
  CHECK(DecodePair(japanese, 0x82, 0x20) == 0);
  CHECK(DecodePair(japanese, 0xff, 0xa0) == 0);
  CHECK(DecodePair(japanese, 0x82, 0xff) == 0);
  CHECK(DecodePair(japanese, 0x82, 0x30) == 0);
  CHECK_FALSE(EncodePair(japanese, U'\u0627'));
  CHECK_FALSE(EncodePair(japanese, U'\U0001F600'));
  CHECK_FALSE(EncodePair(Standard(), U'\u3042'));
}

TEST_CASE("encoding/codepage: a copy of a page maps as the page does")
{
  const Codepage japanese = *Find("ANSI_932");
  const Codepage chinese = *Find("ANSI_936");
  CHECK(EncodePair(japanese, U'あ') == Twin{'\x82', '\xa0'});
  CHECK(EncodePair(chinese, U'中') == Twin{'\xd6', '\xd0'});
  CHECK_FALSE(EncodePair(Standard(), U'中'));
}

TEST_CASE("encoding/codepage: a code point below 128 is its own byte in every page")
{
  CHECK(EncodeByte(Standard(), U'A') == 'A');
  CHECK(EncodeByte(*Find("ANSI_1256"), U'\x7f') == '\x7f');
}
} // namespace DwgIO::Encoding
