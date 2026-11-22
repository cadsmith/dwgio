/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/items.cpp
 *  @brief     Extended data groups: kinds by DXF range, written and read back, and cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/items.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "octets.hpp"
#include "support/reader.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/tag.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/items: a group code's kind follows the DXF ranges")
{
  CHECK(KindOf(0) == Kind::Text);
  CHECK(KindOf(9) == Kind::Text);
  CHECK(KindOf(10) == Kind::Point);
  CHECK(KindOf(18) == Kind::Point);
  CHECK(KindOf(19) == Kind::None);
  CHECK(KindOf(37) == Kind::None);
  CHECK(KindOf(38) == Kind::Real);
  CHECK(KindOf(39) == Kind::Real);
  CHECK(KindOf(40) == Kind::Real);
  CHECK(KindOf(59) == Kind::Real);
  CHECK(KindOf(60) == Kind::Short);
  CHECK(KindOf(79) == Kind::Short);
  CHECK(KindOf(80) == Kind::None);
  CHECK(KindOf(90) == Kind::Long);
  CHECK(KindOf(99) == Kind::Long);
  CHECK(KindOf(100) == Kind::Text);
  CHECK(KindOf(109) == Kind::Text);
  CHECK(KindOf(110) == Kind::Real);
  CHECK(KindOf(149) == Kind::Real);
  CHECK(KindOf(160) == Kind::LongLong);
  CHECK(KindOf(169) == Kind::LongLong);
  CHECK(KindOf(170) == Kind::Short);
  CHECK(KindOf(179) == Kind::Short);
  CHECK(KindOf(210) == Kind::Point);
  CHECK(KindOf(239) == Kind::Point);
  CHECK(KindOf(270) == Kind::Short);
  CHECK(KindOf(279) == Kind::Short);
  CHECK(KindOf(280) == Kind::Byte);
  CHECK(KindOf(289) == Kind::Byte);
  CHECK(KindOf(290) == Kind::Bool);
  CHECK(KindOf(299) == Kind::Bool);
  CHECK(KindOf(300) == Kind::Text);
  CHECK(KindOf(309) == Kind::Text);
  CHECK(KindOf(310) == Kind::Chunk);
  CHECK(KindOf(319) == Kind::Chunk);
  CHECK(KindOf(320) == Kind::Handle);
  CHECK(KindOf(369) == Kind::Handle);
  CHECK(KindOf(370) == Kind::Short);
  CHECK(KindOf(389) == Kind::Short);
  CHECK(KindOf(390) == Kind::Handle);
  CHECK(KindOf(399) == Kind::Handle);
  CHECK(KindOf(400) == Kind::Short);
  CHECK(KindOf(409) == Kind::Short);
  CHECK(KindOf(410) == Kind::Text);
  CHECK(KindOf(419) == Kind::Text);
  CHECK(KindOf(420) == Kind::Long);
  CHECK(KindOf(429) == Kind::Long);
  CHECK(KindOf(430) == Kind::Text);
  CHECK(KindOf(439) == Kind::Text);
  CHECK(KindOf(440) == Kind::Long);
  CHECK(KindOf(459) == Kind::Long);
  CHECK(KindOf(460) == Kind::Real);
  CHECK(KindOf(469) == Kind::Real);
  CHECK(KindOf(470) == Kind::Text);
  CHECK(KindOf(479) == Kind::Text);
  CHECK(KindOf(480) == Kind::Handle);
  CHECK(KindOf(481) == Kind::Handle);
  CHECK(KindOf(482) == Kind::None);
  CHECK(KindOf(999) == Kind::None);
  CHECK(KindOf(1000) == Kind::Text);
  CHECK(KindOf(1003) == Kind::Text);
  CHECK(KindOf(1004) == Kind::Chunk);
  CHECK(KindOf(1005) == Kind::Handle);
  CHECK(KindOf(1006) == Kind::None);
  CHECK(KindOf(1010) == Kind::Point);
  CHECK(KindOf(1013) == Kind::Point);
  CHECK(KindOf(1040) == Kind::Real);
  CHECK(KindOf(1042) == Kind::Real);
  CHECK(KindOf(1070) == Kind::Short);
  CHECK(KindOf(1071) == Kind::Long);
  CHECK(KindOf(1072) == Kind::None);
  CHECK(KindOf(-1) == Kind::None);
}

TEST_CASE("objects/items: groups of every kind are written as the file stores them and read back whole")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const std::vector<DwgIO::Tag> groups{DwgIO::Tag{.Code = 1, .Value = std::string{"text one"}},
                                       DwgIO::Tag{.Code = 10, .Value = 1.0},
                                       DwgIO::Tag{.Code = 20, .Value = 2.0},
                                       DwgIO::Tag{.Code = 30, .Value = 3.0},
                                       DwgIO::Tag{.Code = 40, .Value = 4.5},
                                       DwgIO::Tag{.Code = 70, .Value = std::int64_t{7}},
                                       DwgIO::Tag{.Code = 90, .Value = std::int64_t{90000}},
                                       DwgIO::Tag{.Code = 280, .Value = std::int64_t{2}},
                                       DwgIO::Tag{.Code = 290, .Value = true},
                                       DwgIO::Tag{.Code = 310, .Value = Chunk{.Bytes = BytesOf("\xDE\xAD\xBE\xEF")}},
                                       DwgIO::Tag{.Code = 340, .Value = Handle{0x1F}},
                                       DwgIO::Tag{.Code = 38, .Value = 1.5},
                                       DwgIO::Tag{.Code = 160, .Value = std::int64_t{-5000000000}},
                                       DwgIO::Tag{.Code = 210, .Value = 0.0},
                                       DwgIO::Tag{.Code = 220, .Value = 0.0},
                                       DwgIO::Tag{.Code = 230, .Value = 1.0},
                                       DwgIO::Tag{.Code = 370, .Value = std::int64_t{25}},
                                       DwgIO::Tag{.Code = 410, .Value = std::string{"Model"}},
                                       DwgIO::Tag{.Code = 420, .Value = std::int64_t{16711680}},
                                       DwgIO::Tag{.Code = 480, .Value = Handle{0x2}}};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteGroups(groups, {}, to);
  // The first group as the fixture holds it: the code, the length, the code page and the characters.
  const std::vector<std::byte> bytes{out.Bytes().begin(), out.Bytes().end()};
  REQUIRE(bytes.size() > 13);
  CHECK(bytes[0] == std::byte{0x01});
  CHECK(bytes[1] == std::byte{0x00});
  CHECK(bytes[2] == std::byte{0x08});
  CHECK(bytes[3] == std::byte{0x00});
  CHECK(bytes[4] == std::byte{0x1E});
  CHECK(bytes[5] == std::byte{'t'});
  Bits::Reader in{bytes};
  const Fields::Input reading{.In = &in, .With = with};
  const auto [read, rest] = ReadGroups(reading, bytes.size());
  CHECK(read == groups);
  CHECK(rest.empty());
  CHECK(issues.empty());
}

TEST_CASE("objects/items: data ending inside a group keeps the groups before it and the rest as bytes")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Record = Handle{0x2D3}};
  const std::vector<std::byte> bytes{std::byte{0x46}, std::byte{0x00}, std::byte{0x07}, std::byte{0x00},
                                     std::byte{0x01}, std::byte{0x00}, std::byte{0x08}, std::byte{0x00},
                                     std::byte{0x1E}, std::byte{'t'},  std::byte{'e'}};
  Bits::Reader in{bytes};
  const Fields::Input reading{.In = &in, .With = with};
  const auto [read, rest] = ReadGroups(reading, bytes.size());
  CHECK(read == std::vector{DwgIO::Tag{.Code = 70, .Value = std::int64_t{7}}});
  CHECK(rest == std::vector<std::byte>(bytes.begin() + 4, bytes.end()));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "XRECORD group 1 cut short");
  CHECK(issues.front().Subject == Handle{0x2D3});
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteGroups(read, rest, to);
  CHECK(std::ranges::equal(out.Bytes(), bytes));
}

TEST_CASE("objects/items: a group code of no kind ends the groups with the rest kept as bytes")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const std::vector<std::byte> bytes{std::byte{0x46}, std::byte{0x00}, std::byte{0x07}, std::byte{0x00},
                                     std::byte{0x50}, std::byte{0x00}, std::byte{0x01}, std::byte{0x02}};
  Bits::Reader in{bytes};
  const Fields::Input reading{.In = &in, .With = with};
  const auto [read, rest] = ReadGroups(reading, bytes.size());
  CHECK(read.size() == 1);
  CHECK(rest.size() == 4);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "XRECORD group 80 of no known kind");
}

TEST_CASE("objects/items: a group cut inside its count or bytes or value stops the groups with the rest kept")
{
  for (const std::vector<std::byte> &bytes :
       {std::vector<std::byte>{std::byte{0x01}, std::byte{0x00}, std::byte{0x08}},
        std::vector<std::byte>{std::byte{0x36}, std::byte{0x01}},
        std::vector<std::byte>{std::byte{0x36}, std::byte{0x01}, std::byte{0x04}, std::byte{0xDE}},
        std::vector<std::byte>{std::byte{0x28}, std::byte{0x00}, std::byte{0x01}, std::byte{0x02}},
        std::vector<std::byte>{std::byte{0x46}}})
  {
    std::vector<Issue> issues;
    const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
    Bits::Reader in{bytes};
    const Fields::Input reading{.In = &in, .With = with};
    const auto [read, rest] = ReadGroups(reading, bytes.size());
    CHECK(read.empty());
    CHECK(rest == bytes);
    REQUIRE(issues.size() == 1);
    CHECK(issues.front().What.ends_with("cut short"));
  }
}

TEST_CASE("objects/items: a value not of its code's kind or a point missing coordinates is noted on write")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteGroups(std::vector<DwgIO::Tag>{DwgIO::Tag{.Code = 40, .Value = std::string{"text"}},
                                      DwgIO::Tag{.Code = 10, .Value = std::string{"text"}},
                                      DwgIO::Tag{.Code = 10, .Value = 1.0}, DwgIO::Tag{.Code = 70, .Value = 2.5},
                                      DwgIO::Tag{.Code = 290, .Value = 1.0}, DwgIO::Tag{.Code = 310, .Value = 1.0},
                                      DwgIO::Tag{.Code = 340, .Value = 1.0}, DwgIO::Tag{.Code = 1, .Value = 1.0},
                                      DwgIO::Tag{.Code = 80, .Value = 1.0}},
              {}, to);
  std::vector<std::string> whats;
  for (const Issue &issue : issues)
  {
    whats.push_back(issue.What);
  }
  CHECK(whats == std::vector<std::string>{"XRECORD group 40 holds no real", "XRECORD group 10 holds no real",
                                          "XRECORD point 10 without its 20 and 30", "XRECORD group 70 holds no number",
                                          "XRECORD group 290 holds no bool", "XRECORD group 310 holds no bytes",
                                          "XRECORD group 340 holds no handle", "XRECORD group 1 holds no text",
                                          "XRECORD group 80 of no known kind not stored"});
  // The point alone was written: its code and three doubles, the two missing as zeros.
  CHECK(out.Bytes().size() == 2 + (3 * 8));
}

TEST_CASE("objects/items: a trailing point and text with no scheme and data past the reader's end are handled")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Issues = &issues};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteGroups(std::vector<DwgIO::Tag>{DwgIO::Tag{.Code = 1, .Value = std::string{"utf-8"}},
                                      DwgIO::Tag{.Code = 10, .Value = 1.0}},
              {}, to);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "XRECORD point 10 without its 20 and 30");
  const std::vector<std::byte> bytes{std::byte{0x46}, std::byte{0x00}, std::byte{0x07}, std::byte{0x00}};
  Bits::Reader in{bytes};
  const Fields::Input reading{.In = &in, .With = with};
  const auto [read, rest] = ReadGroups(reading, 100);
  CHECK(read.size() == 1);
  CHECK(rest.empty());
  CHECK(in.HasFailed());
  CHECK(issues.size() == 1);
}

TEST_CASE("objects/items: oversized chunks and text are cut and out-of-range numbers clamped and noted")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  const std::vector<DwgIO::Tag> groups{
      DwgIO::Tag{.Code = 310, .Value = Chunk{.Bytes = BytesOf(std::string(256, 'x'))}},
      DwgIO::Tag{.Code = 1, .Value = std::string(70000, 'y')}, DwgIO::Tag{.Code = 70, .Value = std::int64_t{70000}},
      DwgIO::Tag{.Code = 280, .Value = std::int64_t{300}}, DwgIO::Tag{.Code = 90, .Value = std::int64_t{5000000000}}};
  WriteGroups(groups, {}, to);
  std::vector<std::string> whats;
  for (const Issue &issue : issues)
  {
    whats.push_back(issue.What);
  }
  CHECK(whats == std::vector<std::string>{
                     "XRECORD chunk of 256 bytes cut to 255", "XRECORD text of 70000 bytes cut to 65535",
                     "XRECORD short 70000 out of range, 32767 written", "XRECORD byte 300 out of range, 255 written",
                     "XRECORD long 5000000000 out of range, 2147483647 written"});
  const std::vector<std::byte> bytes{out.Bytes().begin(), out.Bytes().end()};
  Bits::Reader in{bytes};
  const Fields::Input reading{.In = &in, .With = with};
  const auto [read, rest] = ReadGroups(reading, bytes.size());
  REQUIRE(read.size() == 5);
  CHECK(std::get<Chunk>(read[0].Value).Bytes.size() == 255);
  CHECK(std::get<std::string>(read[1].Value).size() == 65535);
  CHECK(read[2] == DwgIO::Tag{.Code = 70, .Value = std::int64_t{32767}});
  CHECK(read[3] == DwgIO::Tag{.Code = 280, .Value = std::int64_t{255}});
  CHECK(read[4] == DwgIO::Tag{.Code = 90, .Value = std::int64_t{2147483647}});
  CHECK(rest.empty());
}
} // namespace DwgIO::Objects
