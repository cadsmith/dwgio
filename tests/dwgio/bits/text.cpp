/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/text.cpp
 *  @brief     The T, TU and TV text codes and the NUL AutoCAD ends a text with.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"

#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace DwgIO::Bits
{
namespace
{
template <std::size_t CountT>
auto Over(const std::array<std::uint8_t, CountT> &bytes) -> Reader
{
  return Reader{std::as_bytes(std::span{bytes})};
}
} // namespace

TEST_CASE("bits/text: T is a bitshort length then that many code page bytes")
{
  static constexpr std::array<std::uint8_t, 3> bytes{0b0100'0000, 0b0111'1010, 0b0100'0000};
  Reader in = Over(bytes);
  CHECK(in.T(Encoding::MakeScheme(Encoding::Standard())) == "\xc3\xa9");
  CHECK(in.Position() == 18);
  static constexpr std::array<std::uint8_t, 4> pair{0x40, 0xB5, 0xB4, 0x00};
  Reader two = Over(pair);
  CHECK(two.T(Encoding::MakeScheme(*Encoding::Find("ANSI_936"))) == "中");
}

TEST_CASE("bits/text: T drops AutoCAD's trailing NUL and writes it back unless the text is empty")
{
  const Encoding::Scheme latin = Encoding::MakeScheme(Encoding::Standard());
  static constexpr std::array<std::uint8_t, 7> bytes{0x41, 0x50, 0x50, 0x50, 0x50, 0x40, 0x00};
  Reader in = Over(bytes);
  CHECK(in.T(latin) == "AAAA");
  CHECK(in.Position() == 50);
  Writer out;
  out.T("AAAA", latin);
  out.T("", latin);
  CHECK(out.Position() == 52);
  Reader back{out.Bytes()};
  CHECK(back.T(latin) == "AAAA");
  CHECK(back.T(latin).empty());
}

TEST_CASE("bits/text: TU is a bitshort count then that many UTF-16 code units")
{
  static constexpr std::array<std::uint8_t, 6> bytes{0x40, 0x92, 0x00, 0x1A, 0x40, 0x00};
  Reader in = Over(bytes);
  CHECK(in.TU() == "Hi");
  CHECK(in.Position() == 42);
}

TEST_CASE("bits/text: TV is TU from R2007 and T before")
{
  const Encoding::Scheme latin = Encoding::MakeScheme(Encoding::Standard());
  static constexpr std::array<std::uint8_t, 3> bytes{0b0100'0000, 0b0111'1010, 0b0100'0000};
  Reader old = Over(bytes);
  CHECK(old.TV(Version::R2004, latin) == "\xc3\xa9");
  static constexpr std::array<std::uint8_t, 6> units{0x40, 0x92, 0x00, 0x1A, 0x40, 0x00};
  Reader wide = Over(units);
  CHECK(wide.TV(Version::R2007, latin) == "Hi");
}

TEST_CASE("bits/text: a negative length fails the reader")
{
  static constexpr std::array<std::uint8_t, 3> bytes{0b0011'1111, 0b1111'1111, 0b1100'0000};
  Reader in = Over(bytes);
  CHECK(in.T(Encoding::Utf8()).empty());
  CHECK(in.HasFailed());
  Reader again = Over(bytes);
  CHECK(again.TU().empty());
  CHECK(again.HasFailed());
}

TEST_CASE("bits/text: written text reads back in each form")
{
  const Encoding::Scheme latin = Encoding::MakeScheme(Encoding::Standard());
  Writer out;
  out.T("caf\xc3\xa9", latin);
  out.TU("\xf0\x9f\x98\x80 Hi");
  out.TV("\xc3\xa9", Version::R2000, latin);
  out.TV("\xc3\xa9", Version::R2007, latin);
  out.T("", latin);
  out.TU("");
  Reader in{out.Bytes()};
  CHECK(in.T(latin) == "caf\xc3\xa9");
  CHECK(in.TU() == "\xf0\x9f\x98\x80 Hi");
  CHECK(in.TV(Version::R2000, latin) == "\xc3\xa9");
  CHECK(in.TV(Version::R2007, latin) == "\xc3\xa9");
  CHECK(in.T(latin).empty());
  CHECK(in.TU().empty());
  CHECK(in.Position() == in.Size() - 4);
}

TEST_CASE("bits/text: text longer than a bitshort can count is cut to fit")
{
  const Encoding::Scheme latin = Encoding::MakeScheme(Encoding::Standard());
  Writer out;
  CHECK_FALSE(out.T(std::string(32766, 'a'), latin));
  CHECK(out.T(std::string(32767, 'a'), latin) == 32766);
  CHECK_FALSE(out.TU(std::string(32767, 'b')));
  CHECK(out.TU(std::string(40000, 'b')) == 32767);
  CHECK(out.TV(std::string(40000, 'c'), Version::R2000, latin) == 32766);
  Reader in{out.Bytes()};
  CHECK(in.T(latin).size() == 32766);
  CHECK(in.T(latin).size() == 32766);
  CHECK(in.TU().size() == 32767);
  CHECK(in.TU().size() == 32767);
  CHECK(in.T(latin).size() == 32766);
}
} // namespace DwgIO::Bits
