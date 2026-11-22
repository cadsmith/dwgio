/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/writer.cpp
 *  @brief     The bit writer: bit order, shortest codes, handles and colors as AutoCAD writes them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/writer.hpp"

#include "bits/reader.hpp"
#include "bits/reference.hpp"

#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace DwgIO::Bits
{
namespace
{
template <typename... BytesT>
auto Exactly(BytesT... bytes) -> std::vector<std::byte>
{
  return {std::byte{static_cast<std::uint8_t>(bytes)}...};
}

auto First(const Writer &out, std::size_t count) -> std::vector<std::byte>
{
  const std::span<const std::byte> bytes = out.Bytes().first(count);
  return {bytes.begin(), bytes.end()};
}
} // namespace

TEST_CASE("bits/writer: bits go most significant first and pad to a byte")
{
  Writer out;
  out.B(true);
  out.BB(0b01);
  CHECK(out.Position() == 3);
  out.Pad();
  CHECK(out.Position() == 8);
  CHECK(out.Take() == Exactly(0b1010'0000));
  CHECK(out.Position() == 0);
  out.Bits(0xABC, 12);
  out.Pad();
  CHECK(out.Take() == Exactly(0xAB, 0xC0));
}

TEST_CASE("bits/writer: raw numbers are little-endian at any position")
{
  Writer out;
  out.B(false);
  out.RC(0x01);
  out.RS(0x0302);
  out.RL(0x07060504);
  out.RLL(0x0F0E0D0C0B0A0908);
  out.RD(1.0);
  Reader in{out.Bytes()};
  CHECK_FALSE(in.B());
  CHECK(in.RC() == 0x01);
  CHECK(in.RS() == 0x0302);
  CHECK(in.RL() == 0x07060504);
  CHECK(in.RLL() == 0x0F0E0D0C0B0A0908);
  CHECK(in.RD() == 1.0);
}

TEST_CASE("bits/writer: a bitshort takes the shortest code")
{
  Writer out;
  out.BS(0);
  out.BS(256);
  CHECK(out.Take() == Exactly(0b1011'0000));
  out.BS(200);
  out.Pad();
  CHECK(out.Take() == Exactly(0b0111'0010, 0b0000'0000));
  out.BS(-1);
  out.BS(257);
  Reader in{out.Bytes()};
  CHECK(in.BS() == -1);
  CHECK(in.BS() == 257);
  CHECK(in.Position() == 36);
}

TEST_CASE("bits/writer: a bitlong takes the shortest code")
{
  Writer out;
  out.BL(0);
  out.BL(255);
  out.BL(256);
  out.BL(-7);
  Reader in{out.Bytes()};
  CHECK(in.BL() == 0);
  CHECK(in.Position() == 2);
  CHECK(in.BL() == 255);
  CHECK(in.Position() == 12);
  CHECK(in.BL() == 256);
  CHECK(in.Position() == 46);
  CHECK(in.BL() == -7);
}

TEST_CASE("bits/writer: a bitlonglong takes the fewest bytes counted in three bits")
{
  Writer out;
  out.BLL(0);
  out.BLL(1);
  out.BLL(0x0300);
  out.BLL(0x010000000000);
  Reader in{out.Bytes()};
  CHECK(in.BLL() == 0);
  CHECK(in.Position() == 3);
  CHECK(in.BLL() == 1);
  CHECK(in.Position() == 14);
  CHECK(in.BLL() == 0x0300);
  CHECK(in.Position() == 33);
  CHECK(in.BLL() == 0x010000000000);
  CHECK(in.Position() == 33 + 3 + 48);
}

TEST_CASE("bits/writer: a bitdouble takes the shortest code and keeps negative zero")
{
  Writer out;
  out.BD(1.0);
  out.BD(0.0);
  CHECK(out.Take() == Exactly(0b0110'0000));
  out.BD(2.5);
  out.BD(-0.0);
  Reader in{out.Bytes()};
  CHECK(in.BD() == 2.5);
  CHECK(in.Position() == 66);
  CHECK(std::signbit(in.BD()));
}

TEST_CASE("bits/writer: modular numbers take the fewest bytes")
{
  Writer out;
  out.MC(-1);
  out.MC(143);
  out.MC(4610);
  out.MC(-1413);
  out.UMC(65);
  out.UMC(300);
  out.MS(4145);
  out.MS(4650033);
  out.MC(0);
  out.MS(0);
  CHECK(First(out, 13) == Exactly(0x41, 0x8F, 0x01, 0x82, 0x24, 0x85, 0x4B, 0x41, 0xAC, 0x02, 0x31, 0x10, 0x31));
  Reader in{out.Bytes()};
  CHECK(in.MC() == -1);
  CHECK(in.MC() == 143);
  CHECK(in.MC() == 4610);
  CHECK(in.MC() == -1413);
  CHECK(in.UMC() == 65);
  CHECK(in.UMC() == 300);
  CHECK(in.MS() == 4145);
  CHECK(in.MS() == 4650033);
  CHECK(in.MC() == 0);
  CHECK(in.MS() == 0);
  CHECK(in.Position() == in.Size());
}

TEST_CASE("bits/writer: a modular char keeps its last byte's sign bit free")
{
  Writer out;
  out.MC(8270);
  out.MC(64);
  out.MC(-64);
  out.UMC(64);
  out.UMC(127);
  CHECK(out.Take() == Exactly(0xCE, 0xC0, 0x00, 0xC0, 0x00, 0xC0, 0x40, 0x40, 0x7F));
}

TEST_CASE("bits/writer: a handle reference takes the fewest bytes")
{
  Writer out;
  out.H(Reference{.Code = 5, .Value = Handle{0x1F}});
  out.H(Reference{.Code = 6, .Value = Handle{}});
  out.H(Reference{.Code = 5, .Value = Handle{0x05E7}});
  CHECK(out.Take() == Exactly(0x51, 0x1F, 0x60, 0x52, 0x05, 0xE7));
  out.H(Reference{.Code = 4, .Value = Handle{0xFFFFFFFFFFFFFFFF}});
  Reader in{out.Bytes()};
  CHECK(in.H() == Reference{.Code = 4, .Value = Handle{0xFFFFFFFFFFFFFFFF}});
}

TEST_CASE("bits/writer: a double with default takes the shortest patch")
{
  static constexpr double other = 2.5;
  Writer out;
  out.DD(other, other);
  out.DD(2.5000000000000004, other);
  out.DD(2.531280517578125, other);
  out.DD(1.0, other);
  Reader in{out.Bytes()};
  CHECK(in.DD(other) == other);
  CHECK(in.Position() == 2);
  CHECK(in.DD(other) == 2.5000000000000004);
  CHECK(in.Position() == 36);
  CHECK(in.DD(other) == 2.531280517578125);
  CHECK(in.Position() == 86);
  CHECK(in.DD(other) == 1.0);
  CHECK(in.Position() == 152);
}

TEST_CASE("bits/writer: a thickness or an extrusion with the usual value takes one bit")
{
  Writer out;
  out.BT(0.0, Version::R2000);
  out.BE(Up, Version::R2000);
  out.BT(1.0, Version::R2000);
  out.BE(Vector{.X = 1, .Y = 0, .Z = 0}, Version::R2000);
  out.BT(0.0, Version::R14);
  out.BE(Up, Version::R14);
  Reader in{out.Bytes()};
  CHECK(in.BT(Version::R2000) == 0.0);
  CHECK(in.BE(Version::R2000) == Up);
  CHECK(in.Position() == 2);
  CHECK(in.BT(Version::R2000) == 1.0);
  CHECK(in.BE(Version::R2000) == Vector{.X = 1, .Y = 0, .Z = 0});
  CHECK(in.Position() == 12);
  CHECK(in.BT(Version::R14) == 0.0);
  CHECK(in.BE(Version::R14) == Up);
  CHECK(in.Position() == 20);
}

TEST_CASE("bits/writer: points are two or three bitdoubles")
{
  Writer out;
  out.Point2(Vector{.X = 1, .Y = 0, .Z = 7});
  out.Point3(Vector{.X = 1, .Y = 0, .Z = 1});
  CHECK(out.Take() == Exactly(0b0110'0110, 0b0100'0000));
}

TEST_CASE("bits/writer: a color is written in the form the release stores")
{
  Writer out;
  out.CMC(Color{.Index = 4}, Version::R2000);
  out.ENC(Color{.Index = 4}, Version::R2000);
  out.CMC(Color{.Index = 0, .Rgb = 0x00011F00, .Names = ColorNames{.Name = "A", .Book = "B"}}, Version::R2004);
  out.ENC(Color{.Index = 2, .Rgb = 0x00030201, .Transparency = 0x03000000}, Version::R2004);
  out.ENC(Color{.Index = 1}, Version::R2004);
  Reader in{out.Bytes()};
  CHECK(in.CMC(Version::R2000) == Color{.Index = 4});
  CHECK(in.ENC(Version::R2000) == Color{.Index = 4});
  CHECK(in.CMC(Version::R2004) == Color{.Index = 0, .Rgb = 0x00011F00, .Names = ColorNames{.Name = "A", .Book = "B"}});
  CHECK(in.ENC(Version::R2004) == Color{.Index = 2, .Rgb = 0x00030201, .Transparency = 0x03000000});
  CHECK(in.ENC(Version::R2004) == Color{.Index = 1});
  // A name without a book, and a book without a name, each alone in its box.
  Writer lone;
  lone.CMC(Color{.Index = 0, .Rgb = 0x00011F00, .Names = ColorNames{.Name = "A", .Book = {}}}, Version::R2004);
  lone.CMC(Color{.Index = 0, .Rgb = 0x00011F00, .Names = ColorNames{.Name = {}, .Book = "B"}}, Version::R2004);
  Reader alone{lone.Bytes()};
  CHECK(alone.CMC(Version::R2004) ==
        Color{.Index = 0, .Rgb = 0x00011F00, .Names = ColorNames{.Name = "A", .Book = {}}});
  CHECK(alone.CMC(Version::R2004) ==
        Color{.Index = 0, .Rgb = 0x00011F00, .Names = ColorNames{.Name = {}, .Book = "B"}});
  Writer referring;
  referring.ENC(Color{.Index = 3, .Reference = Handle{.Value = 5}}, Version::R2004);
  Reader referred{referring.Bytes()};
  CHECK(referred.ENC(Version::R2004).Reference.has_value());
  // A CMC by index, by layer or by block from R2004: the index 0 and the method in the long, as AutoCAD writes it.
  Writer method;
  method.CMC(Color{.Index = 7}, Version::R2004);
  method.CMC(Color{.Index = 256}, Version::R2004);
  method.CMC(Color{.Index = 0}, Version::R2004);
  Reader raw{method.Bytes()};
  CHECK(raw.BS() == 0);
  CHECK(raw.BL() == static_cast<std::int32_t>(0xC3000007));
  CHECK(raw.RC() == 0);
  CHECK(raw.BS() == 0);
  CHECK(raw.BL() == static_cast<std::int32_t>(0xC0000000));
  CHECK(raw.RC() == 0);
  CHECK(raw.BS() == 0);
  CHECK(raw.BL() == static_cast<std::int32_t>(0xC1000000));
  Reader back{method.Bytes()};
  CHECK(back.CMC(Version::R2004) == Color{.Index = 7});
  CHECK(back.CMC(Version::R2004) == Color{.Index = 256});
  CHECK(back.CMC(Version::R2004) == Color{.Index = 0});
  CHECK(in.Position() == in.Size() - 4);
}

TEST_CASE("bits/writer: an object type takes the shortest form its release has")
{
  Writer out;
  out.OT(20, Version::R2007);
  out.OT(20, Version::R2010);
  out.OT(0x1F0 + 0x41, Version::R2010);
  out.OT(0x8004, Version::R2010);
  out.OT(0x100, Version::R2010);
  CHECK(First(out, 1) == Exactly(0x45));
  Reader in{out.Bytes()};
  CHECK(in.OT(Version::R2007) == 20);
  CHECK(in.OT(Version::R2010) == 20);
  CHECK(in.Position() == 20);
  CHECK(in.OT(Version::R2010) == 0x1F0 + 0x41);
  CHECK(in.Position() == 30);
  CHECK(in.OT(Version::R2010) == 0x8004);
  CHECK(in.OT(Version::R2010) == 0x100);
  CHECK(in.Position() == 66);
}

TEST_CASE("bits/writer: raw bytes are written at any position")
{
  Writer out;
  out.Bits(0xA, 4);
  const auto bytes = Exactly(0xBC, 0xDE);
  out.Raw(bytes);
  out.Pad();
  CHECK(out.Take() == Exactly(0xAB, 0xCD, 0xE0));
}

TEST_CASE("bits/writer: a triplet writes 0 or 2 or 6 or 7 rounding up to the next code")
{
  Writer out;
  out.B3(-1);
  out.B3(9);
  out.B3(7);
  out.Pad();
  CHECK(out.Take() == Exactly(0b0111'1110));
  out.BLL(-1);
  Reader in{out.Bytes()};
  CHECK(in.BLL() == 0x00FFFFFFFFFFFFFF);
}

TEST_CASE("bits/writer: writes on a byte boundary need no padding and a negative type is a short")
{
  Writer out;
  out.Pad();
  CHECK(out.Position() == 0);
  out.CMC(Color{.Index = 0, .Rgb = 7}, Version::R2004);
  out.OT(-1, Version::R2010);
  Reader in{out.Bytes()};
  CHECK(in.CMC(Version::R2004) == Color{.Index = 0, .Rgb = 7});
  CHECK(in.OT(Version::R2010) == 0xFFFF);
}

TEST_CASE("bits/writer: appending a writer places its bits at the current position")
{
  Writer head;
  head.Bits(0b101, 3);
  Writer tail;
  tail.Bits(0b11, 2);
  tail.RC(0xA5);
  head.Append(tail);
  CHECK(head.Position() == 13);
  Writer whole;
  whole.RC(1);
  whole.RC(2);
  head.Append(whole);
  CHECK(head.Position() == 29);
  Writer empty;
  head.Append(empty);
  CHECK(head.Position() == 29);
  Reader in{head.Bytes()};
  CHECK(in.Bits(3) == 0b101);
  CHECK(in.Bits(2) == 0b11);
  CHECK(in.RC() == 0xA5);
  CHECK(in.RC() == 1);
  CHECK(in.RC() == 2);
}

TEST_CASE("bits/writer: an entity color with a reference writes the 0x4000 flag and reads it back")
{
  Color color;
  color.Index = 5;
  color.Rgb = static_cast<int>(0xC2112233);
  color.Reference = Handle{};
  Writer writer;
  writer.ENC(color, Version::R2004);
  Reader reader{writer.Bytes()};
  const Color back = reader.ENC(Version::R2004);
  CHECK(back.Reference.has_value());
  CHECK(back.Rgb == color.Rgb);
  CHECK(back.Index == 5);
  Color plain;
  plain.Index = 5;
  plain.Rgb = color.Rgb;
  Writer other;
  other.ENC(plain, Version::R2004);
  CHECK(other.Bytes().size() == writer.Bytes().size());
  CHECK_FALSE(std::ranges::equal(other.Bytes(), writer.Bytes()));
  CHECK_FALSE(Reader{other.Bytes()}.ENC(Version::R2004).Reference.has_value());
}
} // namespace DwgIO::Bits
