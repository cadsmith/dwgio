/***********************************************************************************************************************
 *  @file      tests/dwgio/encoding/encoding.cpp
 *  @brief     Text between a file's code page or UTF-8 and UTF-8, AutoCAD's \U+ escapes among it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/encoding.hpp"

#include "encoding/codepage.hpp"

#include <doctest/doctest.h>

#include <array>
#include <string>
#include <string_view>

namespace DwgIO::Encoding
{
TEST_CASE("encoding/encoding: a code page's bytes decode to UTF-8")
{
  const Scheme latin = MakeScheme(Standard());
  const Scheme arabic = MakeScheme(*Find("ANSI_1256"));
  CHECK(Decode(latin, "caf\xe9") == "caf\xc3\xa9");
  CHECK(Decode(latin, "\x80") == "\xe2\x82\xac");
  CHECK(Decode(arabic, "\xc7\xe1\xd3\xe1\xc7\xe3") == "\xd8\xa7\xd9\x84\xd8\xb3\xd9\x84\xd8\xa7\xd9\x85");
}

TEST_CASE("encoding/encoding: an escape of four hex digits decodes to its character before R2007")
{
  const Scheme latin = MakeScheme(Standard());
  CHECK(Decode(latin, "a\\U+0627z") == "a\xd8\xa7z");
  CHECK(Decode(latin, "\\U+30BF\\U+FFFF") == "\xe3\x82\xbf\xef\xbf\xbf");
  CHECK(Decode(latin, "\\\\U+0627") == "\\\xd8\xa7");
  // Any other form stays, to write back to the same bytes: AutoCAD's dimensions write \U+00B0 for a degree sign the
  // page holds, an escape of MTEXT's own. Lower case, a point the page holds, ASCII, a surrogate, a digit of no hex,
  // three digits, no plus, a short tail.
  CHECK(Decode(latin, "90\\U+00B0 \\u+30bf \\U+30bf \\U+0041 \\U+0000") ==
        "90\\U+00B0 \\u+30bf \\U+30bf \\U+0041 \\U+0000");
  CHECK(Decode(latin, "\\U+D83D \\U+DFFF \\U+30BG \\U+00:0 \\U+0-41") ==
        "\\U+D83D \\U+DFFF \\U+30BG \\U+00:0 \\U+0-41");
  CHECK(Decode(latin, "\\X+0627 \\U-0627 \\U+062") == "\\X+0627 \\U-0627 \\U+062");
  CHECK(Decode(latin, "\\U+D7FF\\U+E000") == "\xed\x9f\xbf\xee\x80\x80");
  // An East Asian page holds its characters as pairs: their escape stays.
  const Scheme japanese = MakeScheme(*Find("ANSI_932"));
  CHECK(Decode(japanese, "\\U+3042\\U+0627") == "\\U+3042\xd8\xa7");
  // A pair's second byte may be a backslash: the character comes first.
  CHECK(Decode(japanese, "\x83\x5cU+0627") == "\xe3\x82\xbdU+0627");
  // The UTF-8 of R2007 and later keeps them.
  CHECK(Decode(Utf8(), "\\U+0627") == "\\U+0627");
  // Every escape read writes back as it was.
  for (const std::string_view raw : {"90\\U+00B0", "\\u+30bf", "\\U+30BF", "\\U+3042", "\\U+0627z"})
  {
    CHECK(Encode(latin, Decode(latin, raw)) == raw);
  }
}

TEST_CASE("encoding/encoding: UTF-8 passes through and a damaged byte becomes U+FFFD")
{
  CHECK(Decode(Utf8(), "x\xd8\xa8") == "x\xd8\xa8");
  CHECK(Decode(Utf8(), "a\xff\xe7"
                       "b") == "a\xef\xbf\xbd\xef\xbf\xbd"
                               "b");
  CHECK(Decode(Utf8(), "\xd8\xa7\xe2\x82\xac\xff\xd8\xa7") == "\xd8\xa7\xe2\x82\xac\xef\xbf\xbd\xd8\xa7");
  CHECK(Decode(Utf8(), "\xef\xbf\xbd\xe2\x82") == "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd");
  CHECK(Encode(Utf8(), "\xd8\xa7\n") == "\xd8\xa7\n");
  CHECK(Encode(Utf8(), "a\xff") == "a\xef\xbf\xbd");
}

TEST_CASE("encoding/encoding: a code point the page lacks is written as an escape of four hex digits")
{
  const Scheme latin = MakeScheme(Standard());
  CHECK(Encode(latin, "caf\xc3\xa9") == "caf\xe9");
  CHECK(Encode(latin, "\xd8\xa7") == "\\U+0627");
  CHECK(Encode(latin, "\xe3\x82\xbf\xe3\x82\xa4 x") == "\\U+30BF\\U+30A4 x");
  CHECK(Encode(latin, "\xef\xbf\xbf") == "\\U+FFFF");
  // Beyond the basic plane, a question mark: AutoCAD keeps four digits of the point and loses the rest.
  CHECK(Encode(latin, "\xf0\x9f\x98\x80") == "?");
  CHECK(Encode(MakeScheme(*Find("ANSI_1256")), "\xd8\xa7\xd9\x84\xd8\xb3") == "\xc7\xe1\xd3");
}

TEST_CASE("encoding/encoding: text a page holds reads back as written")
{
  const std::array schemes{Utf8(), MakeScheme(Standard()), MakeScheme(*Find("ANSI_1256")),
                           MakeScheme(*Find("ANSI_1250"))};
  for (const Scheme &with : schemes)
  {
    const std::string_view text = "Plan n\xc2\xb0 3 \\U+0041 ^J";
    CHECK(Decode(with, Encode(with, text)) == text);
  }
}

TEST_CASE("encoding/encoding: an East Asian page takes two bytes a character")
{
  const Scheme japanese = MakeScheme(*Find("ANSI_932"));
  const std::string raw = "\x82\xa0\x81\x5c \xb1";
  const std::string text = Decode(japanese, raw);
  CHECK(text == "あ― ｱ");
  CHECK(Encode(japanese, text) == raw);
  CHECK(Decode(japanese, "\x82") == "\u0082");
  CHECK(Decode(japanese, "\x82\x20") == "\u0082 ");
  CHECK(Encode(japanese, "\u0082あ") == "\\U+0082\x82\xa0");
  CHECK(Encode(japanese, "\u0080") == "\x80");
  CHECK(Encode(japanese, "") == "\xff");
  CHECK(Encode(MakeScheme(*Find("ANSI_950")), "\u0081") == "\x81");
  CHECK(Encode(japanese, "ا") == "\\U+0627");
}

TEST_CASE("encoding/encoding: the file header's number selects the code page")
{
  REQUIRE(FindScheme(30));
  CHECK(FindScheme(30)->Page == &Standard());
  CHECK(FindScheme(29)->Page == Find("ANSI_1251"));
  CHECK(FindScheme(37)->Page == Find("ANSI_874"));
  CHECK(FindScheme(39)->Page == Find("ANSI_936"));
  CHECK(FindScheme(44)->Page == Find("ANSI_1258"));
  CHECK_FALSE(FindScheme(0));
  CHECK_FALSE(FindScheme(2));
  CHECK_FALSE(FindScheme(43));
  CHECK_FALSE(FindScheme(45));
  CHECK(Decode(*FindScheme(30), "\xe9") == "\xc3\xa9");
}

TEST_CASE("encoding/encoding: a scheme's number is its page's with ANSI_1252 for none")
{
  CHECK(NumberOf(FindScheme(28).value()) == 28);
  CHECK(NumberOf(FindScheme(44).value()) == 44);
  CHECK(NumberOf(Utf8()) == 30);
}
} // namespace DwgIO::Encoding
