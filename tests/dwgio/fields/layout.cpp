/***********************************************************************************************************************
 *  @file      tests/dwgio/fields/layout.cpp
 *  @brief     Field layouts: every field kind, the releases and conditions that skip one, and lists.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "fields/layout.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/custom.hpp"
#include "fields/list.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"

#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace DwgIO::Fields
{
namespace
{
struct Sample
{
  bool A{};
  int B{};
  double C{};
  std::string D{};
  Handle E{};
  Vector F{};
  std::vector<int> G{};
  int Later{};
  std::int32_t Long{};
  std::int64_t LongLong{};
  std::uint8_t Char{};
  std::uint16_t RawShort{};
  std::uint32_t RawLong{};
  double RawDouble{};
  Vector Flat{};
  double Thick{};
  Vector Normal{};
  Color Paint{};
  int Custom{};
  Handle Link{};
  std::vector<Handle> Links{};

  auto operator==(const Sample &) const -> bool = default;
};

auto IsFlagged(const Sample &sample) -> bool
{
  return sample.A;
}

auto ReadTwice(Sample &sample, const Input &in) -> bool
{
  sample.Custom = 2 * in.In->BS();
  return !in.In->HasFailed();
}

auto WriteHalf(const Sample &sample, const Output &to) -> bool
{
  to.To->BS(sample.Custom / 2);
  return true;
}

auto ReadInt(int &value, const Input &in) -> bool
{
  value = in.In->BS();
  return !in.In->HasFailed();
}

auto WriteNever(const int & /*unused*/, const Output & /*unused*/) -> bool
{
  return false;
}

auto WriteInt(const int &value, const Output &to) -> bool
{
  to.To->BS(value);
  return true;
}

template <std::size_t CountT>
auto Over(const std::array<std::uint8_t, CountT> &bytes, Context with = {}) -> std::pair<Bits::Reader, Input>
{
  std::pair<Bits::Reader, Input> made{Bits::Reader{std::as_bytes(std::span{bytes})}, Input{}};
  made.second = Input{.In = &made.first, .With = with};
  return made;
}

auto ReadBytes(const Bits::Writer &out) -> std::vector<std::byte>
{
  return {out.Bytes().begin(), out.Bytes().end()};
}

const Encoding::Scheme Latin = Encoding::MakeScheme(Encoding::Standard());

auto ReadWhole()
{
  return Layout(Bit(&Sample::A), Short(&Sample::B), Double(&Sample::C), Text(&Sample::D), StreamHandle(&Sample::E, 5),
                Point3(&Sample::F), List(&Sample::G, Counted::Long, Short()), Long(&Sample::Long),
                LongLong(&Sample::LongLong), Char(&Sample::Char), RawShort(&Sample::RawShort),
                RawLong(&Sample::RawLong), RawDouble(&Sample::RawDouble), Point2(&Sample::Flat),
                Thickness(&Sample::Thick), Extrusion(&Sample::Normal), CmColor(&Sample::Paint),
                Custom(ReadTwice, WriteHalf));
}

auto MakeSample() -> Sample
{
  return Sample{.A = true,
                .B = 300,
                .C = 2.5,
                .D = "caf\xc3\xa9",
                .E = Handle{0x1F},
                .F = Vector{.X = 1, .Y = 2, .Z = 0},
                .G = {1, 2, 300},
                .Later = 0,
                .Long = -7,
                .LongLong = 0x0300,
                .Char = 0xAB,
                .RawShort = 0x1234,
                .RawLong = 0xDEADBEEF,
                .RawDouble = 0.5,
                .Flat = Vector{.X = 0, .Y = 1, .Z = 0},
                .Thick = 0.0,
                .Normal = Up,
                .Paint = Color{.Index = 4},
                .Custom = 8,
                .Link = {},
                .Links = {}};
}
} // namespace

TEST_CASE("fields/layout: every field writes and reads back")
{
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const Sample sample = MakeSample();
  Bits::Writer out;
  Output to{.To = &out, .With = with};
  REQUIRE(Write(ReadWhole(), sample, to));
  Bits::Reader reader{out.Bytes()};
  Input in{.In = &reader, .With = with};
  Sample read;
  REQUIRE(Read(ReadWhole(), read, in));
  CHECK(read == sample);
  CHECK(reader.Position() == out.Position());
  CHECK(issues.empty());
}

TEST_CASE("fields/layout: a field since a release is skipped before it")
{
  const auto layout = Layout(Since(Version::R2000, Short(&Sample::Later)), Short(&Sample::B));
  Sample sample{.B = 3, .Later = 7};
  Bits::Writer out;
  Output to{.To = &out, .With = Context{.Release = Version::R14}};
  REQUIRE(Write(layout, sample, to));
  CHECK(out.Position() == 10);
  Sample old;
  Bits::Reader reader{out.Bytes()};
  Input in{.In = &reader, .With = Context{.Release = Version::R14}};
  REQUIRE(Read(layout, old, in));
  CHECK(old.Later == 0);
  CHECK(old.B == 3);
  Bits::Writer wide;
  Output later{.To = &wide, .With = Context{.Release = Version::R2000}};
  REQUIRE(Write(layout, sample, later));
  CHECK(wide.Position() == 20);
  Sample newer;
  Bits::Reader again{wide.Bytes()};
  Input back{.In = &again, .With = Context{.Release = Version::R2000}};
  REQUIRE(Read(layout, newer, back));
  CHECK(newer.Later == 7);
}

TEST_CASE("fields/layout: a field until a release is skipped after it")
{
  const auto layout = Layout(Until(Version::R14, Short(&Sample::Later)));
  const Sample sample{.Later = 7};
  Bits::Writer out;
  Output to{.To = &out, .With = Context{.Release = Version::R14}};
  REQUIRE(Write(layout, sample, to));
  CHECK(out.Position() == 10);
  Bits::Writer later;
  Output newer{.To = &later, .With = Context{.Release = Version::R2000}};
  REQUIRE(Write(layout, sample, newer));
  CHECK(later.Position() == 0);
}

TEST_CASE("fields/layout: fields under a condition follow the kind")
{
  const auto layout = Layout(Bit(&Sample::A), When(IsFlagged, Short(&Sample::B)));
  Bits::Writer out;
  Output to{.To = &out};
  REQUIRE(Write(layout, Sample{.A = false, .B = 9}, to));
  CHECK(out.Position() == 1);
  REQUIRE(Write(layout, Sample{.A = true, .B = 9}, to));
  CHECK(out.Position() == 12);
  Bits::Reader reader{out.Bytes()};
  Input in{.In = &reader};
  Sample first;
  REQUIRE(Read(layout, first, in));
  CHECK(first.B == 0);
  Sample second;
  REQUIRE(Read(layout, second, in));
  CHECK(second.B == 9);
}

TEST_CASE("fields/layout: a list writes its count then each element")
{
  const auto layout = Layout(List(&Sample::G, Counted::Long, Short()));
  Bits::Writer out;
  Output to{.To = &out};
  REQUIRE(Write(layout, Sample{.G = {1, 2}}, to));
  CHECK(out.Position() == 30);
  CHECK(ReadBytes(out) == std::vector{std::byte{0x40}, std::byte{0x90}, std::byte{0x14}, std::byte{0x08}});
  const auto counted = Layout(List(&Sample::G, Counted::Short, Short()));
  Bits::Writer shorter;
  Output less{.To = &shorter};
  REQUIRE(Write(counted, Sample{.G = {}}, less));
  CHECK(shorter.Position() == 2);
}

TEST_CASE("fields/layout: a count beyond what is left fails the read")
{
  static constexpr std::array<std::uint8_t, 6> huge{0x3F, 0xFF, 0xFF, 0xDF, 0xC0, 0x00};
  auto [reader, in] = Over(huge);
  Sample sample{.G = {1}};
  CHECK_FALSE(Read(Layout(List(&Sample::G, Counted::Long, Short())), sample, in));
  CHECK(sample.G.empty());
  CHECK(reader.HasFailed());
  static constexpr std::array<std::uint8_t, 6> negative{0x3F, 0xFF, 0xFF, 0xFF, 0xC0, 0x00};
  auto [other, again] = Over(negative);
  CHECK_FALSE(Read(Layout(List(&Sample::G, Counted::Long, Short())), sample, again));
  CHECK(other.HasFailed());
}

TEST_CASE("fields/layout: a failing reader stops the read at the field that failed")
{
  static constexpr std::array<std::uint8_t, 1> bytes{0b0101'0000};
  auto [reader, in] = Over(bytes);
  Sample sample{.B = 1, .Later = 1};
  CHECK_FALSE(Read(Layout(Short(&Sample::B), Short(&Sample::Later)), sample, in));
  CHECK(sample.B == 0);
  CHECK(sample.Later == 1);
}

TEST_CASE("fields/layout: a handle whose code is not the layout's is kept and noted")
{
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Offset = 100, .Record = Handle{9}};
  static constexpr std::array<std::uint8_t, 2> bytes{0x41, 0x1F};
  auto [reader, in] = Over(bytes, with);
  Sample sample;
  REQUIRE(Read(Layout(StreamHandle(&Sample::E, 5)), sample, in));
  CHECK(sample.E == Handle{0x1F});
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() == Issue{.Code = IssueCode::Damaged,
                                .Offset = 102,
                                .Subject = Handle{9},
                                .What = "handle 1F with code 4, 5 expected"});
  Bits::Writer out;
  Output to{.To = &out, .With = with};
  REQUIRE(Write(Layout(StreamHandle(&Sample::E, 5)), sample, to));
  CHECK(ReadBytes(out) == std::vector{std::byte{0x51}, std::byte{0x1F}});
}

TEST_CASE("fields/layout: a custom pair is called in both directions")
{
  const auto layout = Layout(Custom(ReadTwice, WriteHalf));
  Bits::Writer out;
  Output to{.To = &out};
  REQUIRE(Write(layout, Sample{.Custom = 8}, to));
  Bits::Reader reader{out.Bytes()};
  CHECK(reader.BS() == 4);
  reader.Seek(0);
  Input in{.In = &reader};
  Sample sample;
  REQUIRE(Read(layout, sample, in));
  CHECK(sample.Custom == 8);
}

TEST_CASE("fields/layout: every field after a failed one leaves its member alone")
{
  static constexpr std::array<std::uint8_t, 1> none{0x00};
  const auto layout = Layout(
      Short(&Sample::Long), Bit(&Sample::A), Short(&Sample::B), Double(&Sample::C), Text(&Sample::D),
      StreamHandle(&Sample::E, 5), Point3(&Sample::F), List(&Sample::G, Counted::Long, Short()), Long(&Sample::Long),
      LongLong(&Sample::LongLong), Char(&Sample::Char), RawShort(&Sample::RawShort), RawLong(&Sample::RawLong),
      RawDouble(&Sample::RawDouble), Point2(&Sample::Flat), Thickness(&Sample::Thick), Extrusion(&Sample::Normal),
      CmColor(&Sample::Paint), Custom(ReadTwice, WriteHalf), Since(Version::R14, Short(&Sample::Later)),
      When(IsFlagged, Short(&Sample::Later)), List(&Sample::G, Counted::Short, Custom(ReadInt, WriteInt)));
  Bits::Reader reader{std::as_bytes(std::span{none})};
  reader.Seek(7);
  Input in{.In = &reader, .With = Context{.Release = Version::R2000, .Text = &Latin}};
  Sample sample = MakeSample();
  sample.A = true;
  Sample before = sample;
  before.Long = 0;
  CHECK_FALSE(Read(layout, sample, in));
  CHECK(sample == before);
}

TEST_CASE("fields/layout: a list whose count or element cannot be read fails")
{
  static constexpr std::array<std::uint8_t, 1> none{0x00};
  auto [reader, in] = Over(none);
  reader.Seek(7);
  Sample sample;
  CHECK_FALSE(Read(Layout(List(&Sample::G, Counted::Long, Short())), sample, in));
  static constexpr std::array<std::uint8_t, 2> cut{0b0100'0000, 0b1001'0000};
  auto [other, again] = Over(cut);
  CHECK_FALSE(Read(Layout(List(&Sample::G, Counted::Long, Short())), sample, again));
  CHECK(sample.G.empty());
}

TEST_CASE("fields/layout: a short-counted list reads back and an element that will not write stops the list")
{
  const auto counted = Layout(List(&Sample::G, Counted::Short, Short()));
  Bits::Writer out;
  Output to{.To = &out};
  REQUIRE(Write(counted, Sample{.G = {4, 5}}, to));
  Bits::Reader reader{out.Bytes()};
  Input in{.In = &reader};
  Sample sample;
  REQUIRE(Read(counted, sample, in));
  CHECK(sample.G == std::vector{4, 5});
  const auto stuck = Layout(List(&Sample::G, Counted::Short, Custom(ReadInt, WriteNever)));
  Bits::Writer none;
  Output nowhere{.To = &none};
  CHECK_FALSE(Write(stuck, Sample{.G = {1}}, nowhere));
  CHECK(Write(stuck, Sample{.G = {}}, nowhere));
}

TEST_CASE("fields/layout: a differing handle code is noted only where issues are kept")
{
  static constexpr std::array<std::uint8_t, 2> bytes{0x41, 0x1F};
  auto [reader, in] = Over(bytes, Context{.Release = Version::R2000});
  Sample sample;
  REQUIRE(Read(Layout(StreamHandle(&Sample::E, 5)), sample, in));
  CHECK(sample.E == Handle{0x1F});
}

TEST_CASE("fields/layout: a handle or custom read that runs out of bits fails")
{
  static constexpr std::array<std::uint8_t, 1> bytes{0x51};
  auto [reader, in] = Over(bytes);
  reader.Seek(4);
  Sample sample;
  CHECK_FALSE(Read(Layout(StreamHandle(&Sample::E, 5)), sample, in));
  auto [other, again] = Over(bytes);
  other.Seek(7);
  CHECK_FALSE(Read(Layout(Custom(ReadTwice, WriteHalf)), sample, again));
}

TEST_CASE("fields/layout: lists of custom elements count either way and stop at a failing element")
{
  const auto layout = Layout(List(&Sample::G, Counted::Long, Custom(ReadInt, WriteNever)));
  Bits::Writer out;
  Output to{.To = &out};
  CHECK(Write(layout, Sample{.G = {}}, to));
  CHECK_FALSE(Write(layout, Sample{.G = {1}}, to));
  static constexpr std::array<std::uint8_t, 2> cut{0b0100'0000, 0b1001'0000};
  auto [reader, in] = Over(cut);
  Sample sample;
  CHECK_FALSE(Read(layout, sample, in));
}

TEST_CASE("fields/layout: a list of custom elements round-trips under either count")
{
  for (const Counted count : {Counted::Long, Counted::Short, Counted::Char})
  {
    const auto layout = Layout(List(&Sample::G, count, Custom(ReadInt, WriteInt)));
    Bits::Writer out;
    Output to{.To = &out};
    REQUIRE(Write(layout, Sample{.G = {7, 8, 9}}, to));
    Bits::Reader reader{out.Bytes()};
    Input in{.In = &reader};
    Sample sample;
    REQUIRE(Read(layout, sample, in));
    CHECK(sample.G == std::vector{7, 8, 9});
    // The room a list may take counts an object's handle stream too, when the reading has one.
    Bits::Reader again{out.Bytes()};
    Bits::Reader handles{std::span<const std::byte>{}};
    Input inside{.In = &again, .Handles = &handles};
    Sample other;
    REQUIRE(Read(layout, other, inside));
    CHECK(other.G == std::vector{7, 8, 9});
  }
  const auto layout = Layout(List(&Sample::G, Counted::Long, Custom(ReadInt, WriteInt)));
  static constexpr std::array<std::uint8_t, 1> one{0x00};
  auto [reader, in] = Over(one);
  reader.Seek(7);
  Sample sample;
  CHECK_FALSE(Read(layout, sample, in));
  static constexpr std::array<std::uint8_t, 6> huge{0x3F, 0xFF, 0xFF, 0xDF, 0xC0, 0x00};
  auto [other, again] = Over(huge);
  CHECK_FALSE(Read(layout, sample, again));
  static constexpr std::array<std::uint8_t, 6> negative{0x3F, 0xFF, 0xFF, 0xFF, 0xC0, 0x00};
  auto [third, more] = Over(negative);
  CHECK_FALSE(Read(layout, sample, more));
}

TEST_CASE("fields/layout: a custom field over a value is skipped once the reader failed")
{
  static constexpr std::array<std::uint8_t, 1> one{0x00};
  auto [reader, in] = Over(one);
  reader.Seek(7);
  int value = 5;
  CHECK_FALSE(Read(Layout(Short(), Custom(ReadInt, WriteInt)), value, in));
  CHECK(value == 0);
}

TEST_CASE("fields/layout: text with no scheme in the context is UTF-8")
{
  Bits::Writer out;
  Output to{.To = &out, .With = Context{.Release = Version::R2000}};
  REQUIRE(Write(Layout(Text(&Sample::D)), Sample{.D = "caf\xc3\xa9"}, to));
  Bits::Reader reader{out.Bytes()};
  Input in{.In = &reader, .With = Context{.Release = Version::R2000}};
  Sample sample;
  REQUIRE(Read(Layout(Text(&Sample::D)), sample, in));
  CHECK(sample.D == "caf\xc3\xa9");
}

TEST_CASE("fields/layout: from R2007 the text and the color names go to the string stream when there is one")
{
  const Color named{.Index = 0, .Rgb = 0x00010203, .Names = ColorNames{.Name = "Red", .Book = "Book"}};

  struct Painted
  {
    std::string Label{};
    Color Paint{};
    int After{};
  };

  const auto layout = Layout(Text(&Painted::Label), CmColor(&Painted::Paint), Short(&Painted::After));
  const Painted painted{.Label = "caf\xc3\xa9", .Paint = named, .After = 7};
  Bits::Writer data;
  Bits::Writer strings;
  Output to{.To = &data, .Strings = &strings, .With = Context{.Release = Version::R2010}};
  REQUIRE(Write(layout, painted, to));
  Bits::Reader texts{strings.Bytes()};
  CHECK(texts.TU() == "caf\xc3\xa9");
  CHECK(texts.TU() == "Red");
  CHECK(texts.TU() == "Book");
  Bits::Reader dataIn{data.Bytes()};
  Bits::Reader stringsIn{strings.Bytes()};
  Input in{.In = &dataIn, .Strings = &stringsIn, .With = Context{.Release = Version::R2010}};
  Painted read;
  REQUIRE(Read(layout, read, in));
  CHECK(read.Label == painted.Label);
  CHECK(read.Paint == named);
  CHECK(read.After == 7);
  CHECK(dataIn.Position() == data.Position());
  // Before R2007 a string stream is ignored and the text stays with the bits; with none it does too from R2007.
  for (const Version release : {Version::R2004, Version::R2010})
  {
    Bits::Writer inline_;
    Bits::Writer unused;
    Output old{
        .To = &inline_, .Strings = release == Version::R2004 ? &unused : nullptr, .With = Context{.Release = release}};
    REQUIRE(Write(layout, painted, old));
    CHECK(unused.Position() == 0);
    Bits::Reader back{inline_.Bytes()};
    Input again{.In = &back, .With = Context{.Release = release}};
    Painted twice;
    REQUIRE(Read(layout, twice, again));
    CHECK(twice.Label == painted.Label);
    CHECK(twice.Paint == named);
    CHECK(twice.After == 7);
  }
}

TEST_CASE("fields/layout: an out-of-range short or an overlong text is cut to fit and noted")
{
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Offset = 10, .Record = Handle{3}};
  Sample sample = MakeSample();
  sample.B = -40000;
  sample.D = std::string(40000, 'x');
  Bits::Writer out;
  Output to{.To = &out, .With = with};
  REQUIRE(Write(Layout(Short(&Sample::B), Text(&Sample::D)), sample, to));
  REQUIRE(issues.size() == 2);
  CHECK(issues.front() == Issue{.Code = IssueCode::OutOfRange,
                                .Subject = Handle{3},
                                .What = "short -40000 out of range, -32768 written"});
  CHECK(issues.back().What == "text cut to 32766 code units");
  Bits::Reader reader{out.Bytes()};
  Input in{.In = &reader, .With = with};
  Sample read;
  REQUIRE(Read(Layout(Short(&Sample::B), Text(&Sample::D)), read, in));
  CHECK(read.B == -32768);
  CHECK(read.D.size() == 32766);
  Bits::Writer quiet;
  Output silent{.To = &quiet, .With = Context{.Release = Version::R2000}};
  REQUIRE(Write(Layout(Short(&Sample::B)), sample, silent));
  Bits::Reader back{quiet.Bytes()};
  CHECK(back.BS() == -32768);
}

TEST_CASE("fields/layout: an out-of-range byte is written as the nearest byte and noted")
{
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Issues = &issues, .Offset = 10, .Record = Handle{3}};
  Bits::Writer out;
  Output to{.To = &out, .With = with};
  for (const int value : {300, -1, 255})
  {
    Sample sample;
    sample.B = value;
    REQUIRE(Write(Layout(Byte(&Sample::B)), sample, to));
  }
  REQUIRE(issues.size() == 2);
  CHECK(issues.front() ==
        Issue{.Code = IssueCode::OutOfRange, .Subject = Handle{3}, .What = "byte 300 out of range, 255 written"});
  CHECK(issues.back().What == "byte -1 out of range, 0 written");
  Bits::Reader back{out.Bytes()};
  CHECK(back.RC() == 255);
  CHECK(back.RC() == 0);
  CHECK(back.RC() == 255);
}

TEST_CASE("fields/layout: a link reads and writes a handle in the handle stream and notes another code")
{
  static constexpr std::array<std::uint8_t, 2> bytes{0x41, 0x1F};
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  Bits::Reader data{std::span<const std::byte>{}};
  Bits::Reader handles{std::as_bytes(std::span{bytes})};
  Input in{.In = &data, .Handles = &handles, .With = with};
  Sample sample;
  REQUIRE(Read(Layout(Link(&Sample::Link, 4)), sample, in));
  CHECK(sample.Link == Handle{0x1F});
  CHECK(data.Position() == 0);
  CHECK(issues.empty());
  Bits::Writer out;
  Bits::Writer links;
  Output to{.To = &out, .Handles = &links, .With = with};
  REQUIRE(Write(Layout(Link(&Sample::Link, 4)), sample, to));
  CHECK(out.Position() == 0);
  CHECK(ReadBytes(links) == std::vector<std::byte>{std::byte{0x41}, std::byte{0x1F}});
  static constexpr std::array<std::uint8_t, 2> other{0x51, 0x1F};
  Bits::Reader more{std::as_bytes(std::span{other})};
  Input again{.In = &data, .Handles = &more, .With = with};
  REQUIRE(Read(Layout(Link(&Sample::Link, 4)), sample, again));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "handle 1F with code 5, 4 expected");
  Input none{.In = &data, .With = with};
  CHECK_FALSE(Read(Layout(Link(&Sample::Link, 4)), sample, none));
  Output nowhere{.To = &out, .With = with};
  CHECK_FALSE(Write(Layout(Link(&Sample::Link, 4)), sample, nowhere));
  static constexpr std::array<std::uint8_t, 1> cut{0x42};
  Bits::Reader shorter{std::as_bytes(std::span{cut})};
  Input brief{.In = &data, .Handles = &shorter, .With = with};
  CHECK_FALSE(Read(Layout(Link(&Sample::Link, 4)), sample, brief));
  Bits::Reader failed{std::span<const std::byte>{}};
  failed.RC();
  Input after{.In = &failed, .Handles = &handles, .With = with};
  CHECK_FALSE(Read(Layout(Link(&Sample::Link, 4)), sample, after));
}

TEST_CASE("fields/layout: a counted list of links takes its count from the data and its handles from the stream")
{
  Bits::Writer count;
  count.BL(2);
  static constexpr std::array<std::uint8_t, 4> bytes{0x21, 0x10, 0x21, 0x11};
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  Bits::Reader data{count.Bytes()};
  Bits::Reader handles{std::as_bytes(std::span{bytes})};
  Input in{.In = &data, .Handles = &handles, .With = with};
  Sample sample;
  REQUIRE(Read(Layout(List(&Sample::Links, Counted::Long, Link(2))), sample, in));
  CHECK(sample.Links == std::vector<Handle>{Handle{0x10}, Handle{0x11}});
  Bits::Writer out;
  Bits::Writer links;
  Output to{.To = &out, .Handles = &links, .With = with};
  REQUIRE(Write(Layout(List(&Sample::Links, Counted::Long, Link(2))), sample, to));
  CHECK(ReadBytes(out) == ReadBytes(count));
  CHECK(ReadBytes(links) ==
        std::vector<std::byte>(std::as_bytes(std::span{bytes}).begin(), std::as_bytes(std::span{bytes}).end()));
  CHECK(issues.empty());
}

TEST_CASE("fields/layout: a list counted by a char takes one byte for its count")
{
  Bits::Writer count;
  count.RC(2);
  static constexpr std::array<std::uint8_t, 2> bytes{0x20, 0x20};
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  Bits::Reader data{count.Bytes()};
  Bits::Reader handles{std::as_bytes(std::span{bytes})};
  Input in{.In = &data, .Handles = &handles, .With = with};
  Sample sample;
  REQUIRE(Read(Layout(List(&Sample::Links, Counted::Char, Link(2))), sample, in));
  CHECK(sample.Links == std::vector<Handle>(2));
  Bits::Writer out;
  Bits::Writer links;
  Output to{.To = &out, .Handles = &links, .With = with};
  REQUIRE(Write(Layout(List(&Sample::Links, Counted::Char, Link(2))), sample, to));
  CHECK(ReadBytes(out) == ReadBytes(count));
  CHECK(links.Position() == 16);
}

TEST_CASE("fields/layout: a short-counted list writes a bitshort count and a negative count fails the read")
{
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  Sample sample;
  sample.G = {7, 8, 9};
  Bits::Writer out;
  Output to{.To = &out, .With = with};
  REQUIRE(Write(Layout(List(&Sample::G, Counted::Short, Short())), sample, to));
  Bits::Reader back{out.Bytes()};
  CHECK(back.BS() == 3);
  CHECK(back.BS() == 7);
  Bits::Writer negative;
  negative.BL(-1);
  Bits::Reader in{negative.Bytes()};
  Input reading{.In = &in, .With = with};
  Sample read;
  CHECK_FALSE(Read(Layout(List(&Sample::G, Counted::Long, Short())), read, reading));
  CHECK(in.HasFailed());
}

TEST_CASE("fields/layout: a link with an offset code resolves against the record without a note")
{
  static constexpr std::array<std::uint8_t, 7> bytes{0x60, 0x80, 0xA1, 0x03, 0xC1, 0x02, 0x40};
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Record = Handle{0x10}};
  Bits::Reader data{std::span<const std::byte>{}};
  Bits::Reader handles{std::as_bytes(std::span{bytes})};
  Input in{.In = &data, .Handles = &handles, .With = with};
  Sample sample;
  const auto layout = Layout(List(&Sample::Links, Counted::Long, Link(4)));
  Bits::Writer count;
  count.BL(4);
  Bits::Reader counted{count.Bytes()};
  Input reading{.In = &counted, .Handles = &handles, .With = with};
  REQUIRE(Read(layout, sample, reading));
  CHECK(sample.Links == std::vector<Handle>{Handle{0x11}, Handle{0x0F}, Handle{0x13}, Handle{0x0E}});
  CHECK(issues.empty());
  REQUIRE(Read(Layout(Link(&Sample::Link, 3)), sample, in));
  CHECK(issues.size() == 1);
}

TEST_CASE("fields/layout: a negative handle count or one from a spent reader is refused and one that fits kept")
{
  std::vector<Issue> issues;
  const Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  std::size_t count = 0;
  Bits::Writer negative;
  negative.BL(-1);
  Bits::Reader in{negative.Bytes()};
  const Input reading{.In = &in, .With = with};
  CHECK_FALSE(ReadHandleCount(reading, count));
  Bits::Reader spent{std::span<const std::byte>{}};
  const Input empty{.In = &spent, .With = with};
  CHECK_FALSE(ReadHandleCount(empty, count));
  Bits::Writer two;
  two.BL(2);
  two.RL(0);
  Bits::Reader fine{two.Bytes()};
  const Input sane{.In = &fine, .With = with};
  CHECK(ReadHandleCount(sane, count));
  CHECK(count == 2);
}
} // namespace DwgIO::Fields
