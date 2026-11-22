/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/reader.cpp
 *  @brief     The bit reader: bit order, raw and compressed numbers, handles and colors.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reader.hpp"

#include "bits/reference.hpp"

#include "dwgio/color.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
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

TEST_CASE("bits/reader: bits come most significant first")
{
  static constexpr std::array<std::uint8_t, 1> one{0b1011'0000};
  Reader in = Over(one);
  CHECK(in.B());
  CHECK(in.BB() == 0b01);
  CHECK(in.B());
  CHECK(in.Position() == 4);
  CHECK(in.Size() == 8);
}

TEST_CASE("bits/reader: a bit triplet stops at a zero or at its third bit")
{
  static constexpr std::array<std::uint8_t, 2> bits{0b0101'1011, 0b1000'0000};
  Reader in = Over(bits);
  CHECK(in.B3() == 0);
  CHECK(in.B3() == 2);
  CHECK(in.B3() == 6);
  CHECK(in.B3() == 7);
  CHECK(in.Position() == 9);
}

TEST_CASE("bits/reader: raw numbers are little-endian")
{
  static constexpr std::array<std::uint8_t, 15> bytes{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  Reader in = Over(bytes);
  CHECK(in.RC() == 0x01);
  CHECK(in.RS() == 0x0302);
  CHECK(in.RL() == 0x07060504);
  CHECK(in.RLL() == 0x0F0E0D0C0B0A0908);
  CHECK(in.Position() == in.Size());
  CHECK_FALSE(in.HasFailed());
}

TEST_CASE("bits/reader: a raw short reads across an unaligned position")
{
  static constexpr std::array<std::uint8_t, 3> bytes{0x1F, 0xE0, 0x00};
  Reader in = Over(bytes);
  in.Seek(3);
  CHECK(in.RS() == 0x00FF);
  CHECK(in.Position() == 19);
}

TEST_CASE("bits/reader: a raw double is eight little-endian bytes")
{
  static constexpr std::array<std::uint8_t, 8> bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F};
  Reader in = Over(bytes);
  CHECK(in.RD() == 1.0);
}

TEST_CASE("bits/reader: up to 64 bits read at once most significant first")
{
  static constexpr std::array<std::uint8_t, 10> bytes{0xAB, 0xCD, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  Reader in = Over(bytes);
  CHECK(in.Bits(12) == 0xABC);
  CHECK(in.Bits(4) == 0xD);
  CHECK(in.Bits(64) == 0x0102030405060708);
  CHECK(in.Bits(0) == 0);
}

TEST_CASE("bits/reader: padding moves to the next byte boundary")
{
  static constexpr std::array<std::uint8_t, 2> bytes{0xFF, 0x01};
  Reader in = Over(bytes);
  CHECK(in.B());
  in.Pad();
  CHECK(in.Position() == 8);
  in.Pad();
  CHECK(in.Position() == 8);
  CHECK(in.RC() == 0x01);
}

TEST_CASE("bits/reader: bytes read from any position")
{
  static constexpr std::array<std::uint8_t, 3> bytes{0x1A, 0xBC, 0xDE};
  Reader in = Over(bytes);
  in.Seek(4);
  CHECK(in.Bytes(2) == std::vector{std::byte{0xAB}, std::byte{0xCD}});
  CHECK(in.Position() == 20);
}

TEST_CASE("bits/reader: a byte count past what a size holds fails without allocating")
{
  static constexpr std::array<std::uint8_t, 3> bytes{0x1A, 0xBC, 0xDE};
  Reader in = Over(bytes);
  CHECK(in.Bytes(std::size_t{1} << 61U).empty()); // Its bits, eight to a byte, wrap to none.
  CHECK(in.HasFailed());
}

TEST_CASE("bits/reader: a read past the end fails and gives zero and stays at the end")
{
  static constexpr std::array<std::uint8_t, 1> bytes{0xFF};
  Reader in = Over(bytes);
  CHECK(in.RS() == 0);
  CHECK(in.HasFailed());
  CHECK(in.Position() == in.Size());
  CHECK_FALSE(in.B());
  CHECK(in.RC() == 0);
  CHECK(in.Bytes(1).empty());
}

TEST_CASE("bits/reader: a seek past the end fails")
{
  static constexpr std::array<std::uint8_t, 1> bytes{0xFF};
  Reader in = Over(bytes);
  in.Seek(8);
  CHECK_FALSE(in.HasFailed());
  in.Seek(9);
  CHECK(in.HasFailed());
  CHECK(in.Position() == 8);
}
} // namespace DwgIO::Bits

namespace DwgIO::Bits
{
TEST_CASE("bits/reader: a bitshort is a short or a byte or 0 or 256")
{
  static constexpr std::array<std::uint8_t, 5> bits{0x00, 0x40, 0x6D, 0x0F, 0x80};
  Reader in = Over(bits);
  CHECK(in.BS() == 257);
  CHECK(in.BS() == 0);
  CHECK(in.BS() == 256);
  CHECK(in.BS() == 15);
  CHECK(in.BS() == 0);
}

TEST_CASE("bits/reader: a negative bitshort is a short")
{
  static constexpr std::array<std::uint8_t, 3> bits{0b0011'1111, 0b1111'1111, 0b1100'0000};
  Reader in = Over(bits);
  CHECK(in.BS() == -1);
}

TEST_CASE("bits/reader: a bitlong is a long or a byte or 0")
{
  static constexpr std::array<std::uint8_t, 6> bits{0x00, 0x40, 0x40, 0x00, 0x24, 0x3E};
  Reader in = Over(bits);
  CHECK(in.BL() == 257);
  CHECK(in.BL() == 0);
  CHECK(in.BL() == 15);
  CHECK(in.BL() == 0);
}

TEST_CASE("bits/reader: a bitlonglong is three bits of length then that many bytes")
{
  // 000, then 010 and the bytes 00 03.
  static constexpr std::array<std::uint8_t, 3> bits{0b0000'1000, 0b0000'0000, 0b0000'1100};
  Reader in = Over(bits);
  CHECK(in.BLL() == 0);
  CHECK(in.Position() == 3);
  CHECK(in.BLL() == 0x0300);
  CHECK(in.Position() == 22);
}

TEST_CASE("bits/reader: a bitdouble is a double or 1.0 or 0.0")
{
  static constexpr std::array<std::uint8_t, 9> bits{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x0F, 0xD8};
  Reader in = Over(bits);
  CHECK(in.BD() == 1.0);
  CHECK(in.Position() == 66);
  CHECK(in.BD() == 1.0);
  CHECK(in.BD() == 0.0);
}

TEST_CASE("bits/reader: a modular char continues on its high bit and negates on 0x40")
{
  static constexpr std::array<std::uint8_t, 7> bytes{0x8F, 0x01, 0x41, 0x82, 0x24, 0x85, 0x4B};
  Reader in = Over(bytes);
  CHECK(in.MC() == 143);
  CHECK(in.MC() == -1);
  CHECK(in.MC() == 4610);
  CHECK(in.MC() == -1413);
  in.Seek(16);
  CHECK(in.UMC() == 65);
}

TEST_CASE("bits/reader: a modular short continues on 0x8000")
{
  static constexpr std::array<std::uint8_t, 6> bytes{0x31, 0x10, 0x31, 0xF4, 0x8D, 0x00};
  Reader in = Over(bytes);
  CHECK(in.MS() == 4145);
  CHECK(in.MS() == 4650033);
}

TEST_CASE("bits/reader: a handle reference is a code and a count and that many bytes most significant first")
{
  static constexpr std::array<std::uint8_t, 6> bytes{0x51, 0x1F, 0x60, 0x52, 0x05, 0xE7};
  Reader in = Over(bytes);
  CHECK(in.H() == Reference{.Code = 5, .Value = Handle{0x1F}});
  CHECK(in.H() == Reference{.Code = 6, .Value = Handle{}});
  CHECK(in.H() == Reference{.Code = 5, .Value = Handle{0x05E7}});
}

TEST_CASE("bits/reader: a double with default patches the default's bytes")
{
  static constexpr double other = 2.5;
  static constexpr std::array<std::uint8_t, 11> bits{0x10, 0x10, 0x00, 0x00, 0x08, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00};
  Reader in = Over(bits);
  CHECK(in.DD(other) == other);
  CHECK(in.DD(other) == 2.5000000000000004);
  CHECK(in.DD(other) == 2.531280517578125);
  static constexpr std::array<std::uint8_t, 9> full{0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x0F, 0xC0};
  Reader whole = Over(full);
  CHECK(whole.DD(other) == 1.0);
}

TEST_CASE("bits/reader: a thickness or an extrusion is one bit when it has the usual value")
{
  static constexpr std::array<std::uint8_t, 1> usual{0b1100'0000};
  Reader in = Over(usual);
  CHECK(in.BT(Version::R2000) == 0.0);
  CHECK(in.BE(Version::R2000) == Up);
  CHECK(in.Position() == 2);
  static constexpr std::array<std::uint8_t, 2> given{0x2A, 0x40};
  Reader value = Over(given);
  CHECK(value.BT(Version::R2000) == 1.0);
  CHECK(value.BE(Version::R2000) == Vector{.X = 0, .Y = 0, .Z = 1});
  static constexpr std::array<std::uint8_t, 1> old{0xA9};
  Reader before = Over(old);
  CHECK(before.BT(Version::R14) == 0.0);
  CHECK(before.BE(Version::R14) == Vector{.X = 0, .Y = 0, .Z = 1});
}

TEST_CASE("bits/reader: points are two or three bitdoubles")
{
  static constexpr std::array<std::uint8_t, 1> bits{0x64};
  Reader in = Over(bits);
  CHECK(in.Point2() == Vector{.X = 1, .Y = 0, .Z = 0});
  in.Seek(0);
  CHECK(in.Point3() == Vector{.X = 1, .Y = 0, .Z = 1});
}

TEST_CASE("bits/reader: a color before R2004 is a bitshort index")
{
  static constexpr std::array<std::uint8_t, 2> bits{0b0100'0001, 0b0000'0000};
  Reader in = Over(bits);
  CHECK(in.CMC(Version::R2000) == Color{.Index = 4});
  in.Seek(0);
  CHECK(in.ENC(Version::R2000) == Color{.Index = 4});
}

TEST_CASE("bits/reader: a color from R2004 adds RGB and a flag byte and the names it flags")
{
  static constexpr std::array<std::uint8_t, 10> bits{0x80, 0x01, 0xF0, 0x10, 0x00, 0x34, 0x05, 0x05, 0x01, 0x42};
  Reader in = Over(bits);
  CHECK(in.CMC(Version::R2004) == Color{.Index = 0, .Rgb = 0x00011F00, .Names = ColorNames{.Name = "A", .Book = "B"}});
}

TEST_CASE("bits/reader: a color from R2004 by index or layer or block carries its method in the long")
{
  // AutoCAD writes the index as 0 and the method in the long's top byte: 0xC3 an index in the low byte, 0xC0 by
  // layer, 0xC1 by block, 0xC2 a true color. The long's bytes come least significant first.
  static constexpr std::array<std::uint8_t, 6> index{0x80, 0x70, 0x00, 0x0C, 0x30, 0x00};
  Reader seven = Over(index);
  CHECK(seven.CMC(Version::R2004) == Color{.Index = 7});
  static constexpr std::array<std::uint8_t, 6> layer{0x80, 0x00, 0x00, 0x0C, 0x00, 0x00};
  Reader by = Over(layer);
  CHECK(by.CMC(Version::R2004) == Color{.Index = 256});
  static constexpr std::array<std::uint8_t, 6> block{0x80, 0x00, 0x00, 0x0C, 0x10, 0x00};
  Reader of = Over(block);
  CHECK(of.CMC(Version::R2004) == Color{.Index = 0});
}

TEST_CASE("bits/reader: an entity color from R2004 flags RGB and transparency")
{
  static constexpr std::array<std::uint8_t, 11> bits{0x00, 0xA8, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0C};
  Reader in = Over(bits);
  CHECK(in.ENC(Version::R2004) == Color{.Index = 2, .Rgb = 0x00030201, .Transparency = 0x03000000});
  static constexpr std::array<std::uint8_t, 2> plain{0x40, 0x40};
  Reader index = Over(plain);
  CHECK(index.ENC(Version::R2004) == Color{.Index = 1});
}

TEST_CASE("bits/reader: an object type is a bitshort before R2010 and a pair of bits and bytes from it")
{
  static constexpr std::array<std::uint8_t, 2> old{0x45, 0x00};
  Reader before = Over(old);
  CHECK(before.OT(Version::R2007) == 20);
  static constexpr std::array<std::uint8_t, 5> pairs{0x05, 0x14, 0x18, 0x12, 0x00};
  Reader in = Over(pairs);
  CHECK(in.OT(Version::R2010) == 20);
  CHECK(in.OT(Version::R2010) == 0x1F0 + 0x41);
  CHECK(in.OT(Version::R2010) == 0x8004);
}

TEST_CASE("bits/reader: a modular short running to the end fails and later reads give zero")
{
  static constexpr std::array<std::uint8_t, 8> bytes{0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};
  Reader in = Over(bytes);
  CHECK(in.MS() == 0);
  CHECK(in.HasFailed());
  CHECK(in.BS() == 0);
  CHECK(in.MC() == 0);
}

TEST_CASE("bits/reader: a handle with more bytes than remain fails")
{
  static constexpr std::array<std::uint8_t, 2> bytes{0x5F, 0x00};
  Reader in = Over(bytes);
  CHECK(in.H() == Reference{});
  CHECK(in.HasFailed());
}

TEST_CASE("bits/reader: a modular number that never ends fails")
{
  static constexpr std::array<std::uint8_t, 12> bytes{0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
                                                      0x80, 0x80, 0x80, 0x80, 0x80, 0x80};
  Reader in = Over(bytes);
  CHECK(in.MC() == 0);
  CHECK(in.HasFailed());
  Reader again = Over(bytes);
  CHECK(again.MS() == 0);
  CHECK(again.HasFailed());
}

TEST_CASE("bits/reader: asking for more bytes than remain fails at once")
{
  static constexpr std::array<std::uint8_t, 1> bytes{0xFF};
  Reader in = Over(bytes);
  CHECK(in.Bytes(2).empty());
  CHECK(in.HasFailed());
}

TEST_CASE("bits/reader: a color from R2004 with no names reads and cut-short Unicode text fails")
{
  static constexpr std::array<std::uint8_t, 6> bits{0x80, 0x00, 0x00, 0x00, 0x00, 0x00};
  Reader in = Over(bits);
  CHECK(in.CMC(Version::R2004) == Color{.Index = 0, .Rgb = 0});
  static constexpr std::array<std::uint8_t, 3> cut{0x41, 0x00, 0x00};
  Reader text = Over(cut);
  CHECK(text.TU().empty());
  CHECK(text.HasFailed());
}
} // namespace DwgIO::Bits
