/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/eed.cpp
 *  @brief     Extended data: every item kind, hostile codes and records running past the end.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/eed.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "fields/context.hpp"
#include "octets.hpp"
#include "support/fixtures.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
const Encoding::Scheme Latin = Encoding::MakeScheme(Encoding::Standard());

auto ReadBytes(const std::vector<std::uint8_t> &values) -> std::vector<std::byte>
{
  std::vector<std::byte> out;
  for (const std::uint8_t one : values)
  {
    out.push_back(std::byte{one});
  }
  return out;
}
} // namespace

TEST_CASE("objects/eed: each application's items run until a zero length")
{
  Bits::Writer out;
  out.BS(17);
  out.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  out.RC(0);
  out.RC(5);
  out.RC(0);
  out.RC(30);
  out.Raw(ReadBytes({'A', 'r', 'i', 'a', 'l'}));
  out.RC(71);
  out.RL(34);
  out.RC(70);
  out.RS(0xFFFF);
  out.BS(13);
  out.H(Bits::Reference{.Code = 5, .Value = Handle{0x9E}});
  out.RC(2);
  out.RC(0);
  out.RC(40);
  out.RD(2.5);
  out.RC(2);
  out.RC(1);
  out.BS(0);
  out.BS(7);
  Bits::Reader in{out.Bytes()};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const std::vector<Xdata> read = ReadEed(in, with);
  const std::vector<Xdata> wanted{
      Xdata{.Application = {},
            .AppId = Handle{0x12},
            .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string{"Arial"}},
                     DwgIO::Tag{.Code = 1071, .Value = std::int64_t{34}},
                     DwgIO::Tag{.Code = 1070, .Value = std::int64_t{-1}}}},
      Xdata{.Application = {},
            .AppId = Handle{0x9E},
            .Tags = {DwgIO::Tag{.Code = 1002, .Value = std::string{"{"}}, DwgIO::Tag{.Code = 1040, .Value = 2.5},
                     DwgIO::Tag{.Code = 1002, .Value = std::string{"}"}}}},
  };
  CHECK(read == wanted);
  CHECK(in.BS() == 7);
  CHECK(issues.empty());
}

TEST_CASE("objects/eed: handles and points and binary data and a string of another code page read")
{
  Bits::Writer out;
  out.BS(52);
  out.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  out.RC(3);
  out.Raw(ReadBytes({0, 0, 0, 0, 0, 0, 0, 0x10}));
  out.RC(5);
  out.Raw(ReadBytes({0, 0, 0, 0, 0, 0, 0x02, 0xCC}));
  out.RC(11);
  out.RD(1);
  out.RD(2);
  out.RD(3);
  out.RC(4);
  out.RC(2);
  out.RC(0xAB);
  out.RC(0xCD);
  out.RC(0);
  out.RC(1);
  out.RC(0);
  out.RC(35);
  out.RC(0xC7);
  out.BS(0);
  Bits::Reader in{out.Bytes()};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const std::vector<Xdata> read = ReadEed(in, with);
  REQUIRE(read.size() == 1);
  const std::vector<DwgIO::Tag> groups{
      DwgIO::Tag{.Code = 1003, .Value = Handle{0x10}},
      DwgIO::Tag{.Code = 1005, .Value = Handle{0x2CC}},
      DwgIO::Tag{.Code = 1011, .Value = 1.0},
      DwgIO::Tag{.Code = 1021, .Value = 2.0},
      DwgIO::Tag{.Code = 1031, .Value = 3.0},
      DwgIO::Tag{.Code = 1004, .Value = Chunk{BytesOf("\xAB\xCD")}},
      DwgIO::Tag{.Code = 1000, .Value = std::string{"\xd8\xa7"}},
  };
  CHECK(read.front().Tags == groups);
  CHECK(issues.empty());
}

TEST_CASE("objects/eed: an item code the specification lacks ends the record with an issue")
{
  Bits::Writer out;
  out.BS(3);
  out.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  out.RC(70);
  out.RS(3);
  out.BS(3);
  out.H(Bits::Reference{.Code = 5, .Value = Handle{0x13}});
  out.RC(99);
  out.RS(0);
  out.BS(0);
  out.BS(5);
  Bits::Reader in{out.Bytes()};
  std::vector<Issue> issues;
  const Fields::Context with{
      .Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Offset = 40, .Record = Handle{7}};
  const std::vector<Xdata> read = ReadEed(in, with);
  REQUIRE(read.size() == 2);
  CHECK(read.back().Tags.empty());
  CHECK(in.BS() == 5);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().Subject == Handle{7});
  CHECK(issues.front().What == "extended data item code 99 unknown");
}

TEST_CASE("objects/eed: a record running past the end leaves the reader failed")
{
  Bits::Writer out;
  out.BS(40);
  out.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  out.RC(70);
  out.RS(3);
  Bits::Reader in{out.Bytes()};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  CHECK(ReadEed(in, with).empty());
  CHECK(in.HasFailed());
}

TEST_CASE("objects/eed: Unicode strings read from R2007 and a truncated one fails")
{
  Bits::Writer out;
  out.BS(7);
  out.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  out.RC(0);
  out.RS(2);
  out.RS('H');
  out.RS('i');
  out.BS(0);
  Bits::Reader in{out.Bytes()};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2007, .Text = &Latin, .Issues = &issues};
  const std::vector<Xdata> read = ReadEed(in, with);
  REQUIRE(read.size() == 1);
  CHECK(read.front().Tags == std::vector{DwgIO::Tag{.Code = 1000, .Value = std::string{"Hi"}}});
  Bits::Writer cut;
  cut.BS(40);
  cut.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  cut.RC(0);
  cut.RS(200);
  Bits::Reader again{cut.Bytes()};
  CHECK(ReadEed(again, with).empty());
  CHECK(again.HasFailed());
}

TEST_CASE("objects/eed: a string in a page dwgio lacks reads through the file's scheme or as UTF-8")
{
  Bits::Writer out;
  out.BS(5);
  out.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  out.RC(0);
  out.RC(1);
  out.RC(0);
  out.RC(1);
  out.RC(0xE9);
  out.BS(0);
  std::vector<Issue> issues;
  Bits::Reader in{out.Bytes()};
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  CHECK(ReadEed(in, with).front().Tags == std::vector{DwgIO::Tag{.Code = 1000, .Value = std::string{"\xc3\xa9"}}});
  Bits::Reader again{out.Bytes()};
  const Fields::Context bare{.Release = Version::R2000, .Text = nullptr, .Issues = &issues};
  CHECK(ReadEed(again, bare).front().Tags ==
        std::vector{DwgIO::Tag{.Code = 1000, .Value = std::string{"\xef\xbf\xbd"}}});
}

TEST_CASE("objects/eed: codes between the known ones are unknown and an item past its record is noted")
{
  for (const std::uint8_t code : {std::uint8_t{6}, std::uint8_t{20}, std::uint8_t{50}})
  {
    Bits::Writer out;
    out.BS(2);
    out.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
    out.RC(code);
    out.RC(0);
    out.BS(0);
    std::vector<Issue> issues;
    Bits::Reader in{out.Bytes()};
    const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
    CHECK(ReadEed(in, with).front().Tags.empty());
    CHECK(issues.front().What == std::format("extended data item code {} unknown", code));
  }
  Bits::Writer out;
  out.BS(2);
  out.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  out.RC(70);
  out.RS(5);
  out.BS(0);
  std::vector<Issue> issues;
  Bits::Reader in{out.Bytes()};
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  CHECK(ReadEed(in, with).size() == 1);
  CHECK(issues.front().What == "extended data of 2 bytes misread");
  Bits::Writer cut;
  cut.BS(6);
  cut.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  cut.RC(0);
  cut.RC(3);
  cut.RC(0);
  cut.RC(30);
  Bits::Reader short_{cut.Bytes()};
  CHECK(ReadEed(short_, with).empty());
}

TEST_CASE("objects/eed: a string whose bytes or units end before their count fails the reader")
{
  Bits::Writer narrow;
  narrow.BS(4);
  narrow.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  narrow.RC(0);
  narrow.RC(5);
  narrow.RC(0);
  narrow.RC(30);
  std::vector<Issue> issues;
  Bits::Reader in{narrow.Bytes()};
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  CHECK(ReadEed(in, with).size() == 1);
  CHECK(in.HasFailed());
  Bits::Writer wide;
  wide.BS(4);
  wide.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  wide.RC(0);
  wide.RS(200);
  wide.RC(0);
  Bits::Reader again{wide.Bytes()};
  const Fields::Context later{.Release = Version::R2007, .Text = &Latin, .Issues = &issues};
  CHECK(ReadEed(again, later).size() == 1);
  CHECK(again.HasFailed());
}

TEST_CASE("objects/eed: every item kind is written as chapter 28 stores it")
{
  const std::vector<Xdata> records{
      Xdata{.Application = {},
            .AppId = Handle{0x12},
            .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string{"Arial"}},
                     DwgIO::Tag{.Code = 1071, .Value = std::int64_t{34}},
                     DwgIO::Tag{.Code = 1070, .Value = std::int64_t{-1}},
                     DwgIO::Tag{.Code = 1002, .Value = std::string{"{"}}, DwgIO::Tag{.Code = 1040, .Value = 2.5},
                     DwgIO::Tag{.Code = 1002, .Value = std::string{"}"}},
                     DwgIO::Tag{.Code = 1003, .Value = Handle{0x10}}, DwgIO::Tag{.Code = 1005, .Value = Handle{0x2CC}},
                     DwgIO::Tag{.Code = 1011, .Value = 1.0}, DwgIO::Tag{.Code = 1021, .Value = 2.0},
                     DwgIO::Tag{.Code = 1031, .Value = 3.0},
                     DwgIO::Tag{.Code = 1004, .Value = Chunk{BytesOf("\xAB\xCD")}}}},
  };
  Bits::Writer wanted;
  wanted.BS(77);
  wanted.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  wanted.RC(0);
  wanted.RC(5);
  wanted.RC(0);
  wanted.RC(30);
  wanted.Raw(ReadBytes({'A', 'r', 'i', 'a', 'l'}));
  wanted.RC(71);
  wanted.RL(34);
  wanted.RC(70);
  wanted.RS(0xFFFF);
  wanted.RC(2);
  wanted.RC(0);
  wanted.RC(40);
  wanted.RD(2.5);
  wanted.RC(2);
  wanted.RC(1);
  wanted.RC(3);
  wanted.Raw(ReadBytes({0, 0, 0, 0, 0, 0, 0, 0x10}));
  wanted.RC(5);
  wanted.Raw(ReadBytes({0, 0, 0, 0, 0, 0, 0x02, 0xCC}));
  wanted.RC(11);
  wanted.RD(1);
  wanted.RD(2);
  wanted.RD(3);
  wanted.RC(4);
  wanted.RC(2);
  wanted.RC(0xAB);
  wanted.RC(0xCD);
  wanted.BS(0);
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteEed(records, to);
  CHECK(std::vector<std::byte>(out.Bytes().begin(), out.Bytes().end()) ==
        std::vector<std::byte>(wanted.Bytes().begin(), wanted.Bytes().end()));
  CHECK(issues.empty());
  Bits::Reader back{out.Bytes()};
  CHECK(ReadEed(back, with) == records);
}

TEST_CASE("objects/eed: a Unicode string from R2007 writes its UTF-16 units")
{
  const std::vector<Xdata> records{Xdata{
      .Application = {}, .AppId = Handle{0x12}, .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string{"\xc3\xa9"}}}}};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2007, .Text = &Latin, .Issues = &issues};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteEed(records, to);
  Bits::Writer wanted;
  wanted.BS(5);
  wanted.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  wanted.RC(0);
  wanted.RS(1);
  wanted.RS(0xE9);
  wanted.BS(0);
  CHECK(std::vector<std::byte>(out.Bytes().begin(), out.Bytes().end()) ==
        std::vector<std::byte>(wanted.Bytes().begin(), wanted.Bytes().end()));
  Bits::Reader back{out.Bytes()};
  CHECK(ReadEed(back, with) == records);
}

TEST_CASE("objects/eed: a Unicode string from R2007 longer than 255 units is cut with a note")
{
  const std::vector<Xdata> records{Xdata{
      .Application = {}, .AppId = Handle{0x12}, .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string(70000, 'u')}}}};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2007, .Text = &Latin, .Issues = &issues};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteEed(records, to);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "extended data text of 70000 units cut to 255");
  Bits::Reader back{out.Bytes()};
  const std::vector<Xdata> read = ReadEed(back, with);
  REQUIRE(read.size() == 1);
  CHECK(read.front().Tags == std::vector{DwgIO::Tag{.Code = 1000, .Value = std::string(255, 'u')}});
}

TEST_CASE("objects/eed: a record too large for its size is left out with a note and the next one kept")
{
  Xdata large{.Application = {}, .AppId = Handle{0x12}, .Tags = {}};
  for (int chunk = 0; chunk < 130; ++chunk)
  {
    large.Tags.push_back(DwgIO::Tag{.Code = 1004, .Value = Chunk{BytesOf(std::string(255, 'b'))}});
  }
  const Xdata small{
      .Application = {}, .AppId = Handle{0x13}, .Tags = {DwgIO::Tag{.Code = 1070, .Value = std::int64_t{7}}}};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteEed({large, small}, to);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().Code == IssueCode::NotKept);
  CHECK(issues.front().What == "extended data of 12 of 33410 bytes left out");
  Bits::Reader back{out.Bytes()};
  CHECK(ReadEed(back, with) == std::vector{small});
}

TEST_CASE("objects/eed: a group the chapter cannot store is dropped and noted")
{
  const std::vector<Xdata> records{Xdata{
      .Application = {},
      .AppId = Handle{0x12},
      .Tags = {
          DwgIO::Tag{.Code = 1001, .Value = std::string{"ACAD"}}, DwgIO::Tag{.Code = 1040, .Value = std::string{"x"}},
          DwgIO::Tag{.Code = 1010, .Value = 1.0}, DwgIO::Tag{.Code = 1070, .Value = std::int64_t{7}},
          DwgIO::Tag{.Code = 1004, .Value = Chunk{BytesOf(std::string(300, 'b'))}},
          DwgIO::Tag{.Code = 1002, .Value = std::int64_t{0}}, DwgIO::Tag{.Code = 1003, .Value = std::string{"x"}},
          DwgIO::Tag{.Code = 1004, .Value = std::string{"x"}}, DwgIO::Tag{.Code = 1071, .Value = std::string{"x"}},
          DwgIO::Tag{.Code = 1000, .Value = std::string(300, 't')}, DwgIO::Tag{.Code = 1300, .Value = std::int64_t{1}},
          DwgIO::Tag{.Code = 999, .Value = std::int64_t{1}}, DwgIO::Tag{.Code = 1000, .Value = std::int64_t{1}},
          DwgIO::Tag{.Code = 1011, .Value = std::string{"x"}}, DwgIO::Tag{.Code = 1012, .Value = 2.0}}}};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Record = Handle{9}};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteEed(records, to);
  REQUIRE(issues.size() == 14);
  CHECK(issues[0].What == "extended data group 1001 not stored");
  CHECK(issues[1].What == "extended data group 1040 holds no real");
  CHECK(issues[2].What == "extended data point 1010 without its 1020 and 1030");
  CHECK(issues[3].What == "extended data chunk of 300 bytes cut to 255");
  CHECK(issues[4].What == "extended data group 1002 holds no text");
  CHECK(issues[5].What == "extended data group 1003 holds no handle");
  CHECK(issues[6].What == "extended data group 1004 holds no bytes");
  CHECK(issues[7].What == "extended data group 1071 holds no number");
  CHECK(issues[8].What == "extended data text of 300 bytes cut to 255");
  CHECK(issues[9].What == "extended data group 1300 not stored");
  CHECK(issues[10].What == "extended data group 999 not stored");
  CHECK(issues[11].What == "extended data group 1000 holds no text");
  CHECK(issues[12].What == "extended data group 1011 holds no real");
  CHECK(issues[13].What == "extended data point 1012 without its 1022 and 1032");
  CHECK(issues[0].Subject == Handle{9});
  Bits::Reader back{out.Bytes()};
  const std::vector<Xdata> read = ReadEed(back, with);
  REQUIRE(read.size() == 1);
  const std::vector<DwgIO::Tag> groups{DwgIO::Tag{.Code = 1010, .Value = 1.0},
                                       DwgIO::Tag{.Code = 1020, .Value = 0.0},
                                       DwgIO::Tag{.Code = 1030, .Value = 0.0},
                                       DwgIO::Tag{.Code = 1070, .Value = std::int64_t{7}},
                                       DwgIO::Tag{.Code = 1004, .Value = Chunk{BytesOf(std::string(255, 'b'))}},
                                       DwgIO::Tag{.Code = 1000, .Value = std::string(255, 't')},
                                       DwgIO::Tag{.Code = 1012, .Value = 2.0},
                                       DwgIO::Tag{.Code = 1022, .Value = 0.0},
                                       DwgIO::Tag{.Code = 1032, .Value = 0.0}};
  CHECK(read.front().Tags == groups);
}

TEST_CASE("objects/eed: every record in the fixtures writes back bit for bit")
{
  std::size_t records = 0;
  for (const std::string_view name : {"foundation-r2000.dwg", "tables-r2000.dwg", "foundation-r14.dwg",
                                      "tables-r14.dwg", "kinds-r2000.dwg", "kinds-r14.dwg"})
  {
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    const Encoding::Scheme scheme = Encoding::FindScheme(loaded->Drawing.Codepage).value();
    std::vector<Issue> issues;
    const Fields::Context with{.Release = loaded->Drawing.Release, .Text = &scheme, .Issues = &issues};
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *kept = GetIf<Unknown>(&object);
      if (kept == nullptr)
      {
        continue;
      }
      const Unknown &unknown = *kept;
      const std::span<const std::byte> whole{unknown.Bytes};
      Bits::Reader sizer{whole};
      sizer.MS();
      const std::size_t start = sizer.Position() / 8;
      Bits::Reader data{whole.subspan(start, whole.size() - start - 2)};
      data.OT(with.Release);
      if (with.Release >= Version::R2000)
      {
        data.RL();
      }
      data.H();
      const std::size_t from = data.Position();
      const std::vector<Xdata> read = ReadEed(data, with);
      if (read.empty())
      {
        continue;
      }
      records += read.size();
      Bits::Writer back;
      Fields::Output to{.To = &back, .With = with};
      WriteEed(read, to);
      Bits::Reader source{whole.subspan(start, whole.size() - start - 2)};
      source.Seek(from);
      Bits::Writer copy;
      while (source.Position() < data.Position())
      {
        copy.B(source.B());
      }
      CHECK_MESSAGE(std::vector<std::byte>(copy.Bytes().begin(), copy.Bytes().end()) ==
                        std::vector<std::byte>(back.Bytes().begin(), back.Bytes().end()),
                    name, " ", HexOf(unknown.Base.Self));
    }
    CHECK(issues.empty());
  }
  CHECK(records >= 8);
}

TEST_CASE("objects/eed: a string written with no scheme goes as UTF-8 under page 30")
{
  const std::vector<Xdata> records{Xdata{
      .Application = {}, .AppId = Handle{0x12}, .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string{"\xc3\xa9"}}}}};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = nullptr, .Issues = &issues};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  WriteEed(records, to);
  Bits::Writer wanted;
  wanted.BS(6);
  wanted.H(Bits::Reference{.Code = 5, .Value = Handle{0x12}});
  wanted.RC(0);
  wanted.RC(2);
  wanted.RC(0);
  wanted.RC(30);
  wanted.RC(0xC3);
  wanted.RC(0xA9);
  wanted.BS(0);
  CHECK(std::vector<std::byte>(out.Bytes().begin(), out.Bytes().end()) ==
        std::vector<std::byte>(wanted.Bytes().begin(), wanted.Bytes().end()));
  CHECK(issues.empty());
}
} // namespace DwgIO::Objects
