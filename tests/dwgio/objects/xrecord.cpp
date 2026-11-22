/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/xrecord.cpp
 *  @brief     The XRECORD against its DXF twin, written back, and cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/xrecord.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/read.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/xrecord.hpp"
#include "objects/write.hpp"
#include "octets.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
/**
 *  Two hex digits as a byte character.
 */
auto ParseHex(std::string_view digits) -> std::string
{
  std::string bytes;
  for (std::size_t at = 0; at + 1 < digits.size(); at += 2)
  {
    bytes.push_back(static_cast<char>(std::stoi(std::string{digits.substr(at, 2)}, nullptr, 16)));
  }
  return bytes;
}

/**
 *  The twin's groups of an XRECORD after its cloning flag, as the kind each code holds.
 */
auto ReadTwinGroups(const std::vector<Pair> &twin, std::size_t opened) -> std::vector<DwgIO::Tag>
{
  std::vector<DwgIO::Tag> groups;
  bool inside = false;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    const auto [code, value] = twin[at];
    if (!inside)
    {
      inside = code == 280;
      continue;
    }
    if ((code >= 10 && code <= 59) || (code >= 110 && code <= 149) || (code >= 210 && code <= 239) ||
        (code >= 460 && code <= 469))
    {
      groups.push_back(DwgIO::Tag{.Code = code, .Value = std::stod(value)});
    }
    else if ((code >= 60 && code <= 79) || (code >= 90 && code <= 99) || (code >= 160 && code <= 179) ||
             (code >= 270 && code <= 289) || (code >= 370 && code <= 389) || (code >= 400 && code <= 409) ||
             (code >= 420 && code <= 429) || (code >= 440 && code <= 459))
    {
      groups.push_back(DwgIO::Tag{.Code = code, .Value = static_cast<std::int64_t>(std::stoll(value))});
    }
    else if (code >= 290 && code <= 299)
    {
      groups.push_back(DwgIO::Tag{.Code = code, .Value = value == "1"});
    }
    else if (code >= 310 && code <= 319)
    {
      groups.push_back(DwgIO::Tag{.Code = code, .Value = Chunk{.Bytes = BytesOf(ParseHex(value))}});
    }
    else if ((code >= 320 && code <= 369) || (code >= 390 && code <= 399))
    {
      groups.push_back(DwgIO::Tag{.Code = code, .Value = ParseHandle(value).value_or(Handle{})});
    }
    else
    {
      groups.push_back(DwgIO::Tag{.Code = code, .Value = value});
    }
  }
  return groups;
}
} // namespace

TEST_CASE("objects/xrecord: every xrecord of the fixture matches the DXF twin")
{
  const std::vector<Pair> twin = ReadTwin("objects-r2000.dxf");
  for (const std::string_view name : {"objects-r2000.dwg", "objects-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    std::size_t count = 0;
    std::size_t regenerated = 0;
    for (const Object &object : drawing.Objects)
    {
      const auto *xrecord = GetIf<XRecord>(&object);
      if (xrecord == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(xrecord->Base.Self));
      CHECK(TypeOf(object) == "XRECORD");
      CHECK(xrecord->Rest.empty());
      const std::optional<std::size_t> opened = FindHandled(twin, xrecord->Base.Self);
      if (!opened)
      {
        ++regenerated;
        continue;
      }
      CHECK(xrecord->Cloning == ReadWhole(twin, *opened, 280));
      const std::vector<DwgIO::Tag> expected = ReadTwinGroups(twin, *opened);
      if (drawing.Release >= Version::R2000)
      {
        CHECK(xrecord->Data == expected);
      }
      else
      {
        CHECK(xrecord->Data.size() == expected.size());
      }
    }
    CHECK(count >= 62);
    CHECK(regenerated == (drawing.Release >= Version::R2000 ? 46 : 110));
    const auto *root = GetIf<Dictionary>(FindObject(drawing, drawing.Header.DictionaryNamedObjects));
    REQUIRE(root);
    const Handle *own = EntryNamed(*root, "DWGIO_DICT", drawing);
    REQUIRE(own);
    const auto *dictionary = GetIf<Dictionary>(FindObject(drawing, *own));
    REQUIRE(dictionary);
    const Handle *full = EntryNamed(*dictionary, "FULL", drawing);
    REQUIRE(full);
    const auto *record = GetIf<XRecord>(FindObject(drawing, *full));
    REQUIRE(record);
    REQUIRE(record->Data.size() == 23);
    CHECK(record->Data.front() == DwgIO::Tag{.Code = 1, .Value = std::string{"text one"}});
    CHECK(record->Data[2] == DwgIO::Tag{.Code = 10, .Value = 1.0});
    CHECK(record->Data[4] == DwgIO::Tag{.Code = 30, .Value = 3.0});
    CHECK(record->Data[8] == DwgIO::Tag{.Code = 90, .Value = std::int64_t{90000}});
    CHECK(record->Data[10] == DwgIO::Tag{.Code = 290, .Value = true});
    CHECK(record->Data[12] == DwgIO::Tag{.Code = 310, .Value = Chunk{.Bytes = BytesOf("\xDE\xAD\xBE\xEF")}});
    CHECK(record->Data[13] == DwgIO::Tag{.Code = 340, .Value = drawing.Header.BlockRecordModelSpace});
    CHECK(record->Data[15] == DwgIO::Tag{.Code = 38, .Value = 1.5});
    CHECK(record->Data[16] == DwgIO::Tag{.Code = 39, .Value = 2.5});
    CHECK(record->Data[17] == DwgIO::Tag{.Code = 370, .Value = std::int64_t{25}});
    CHECK(record->Data[18] == DwgIO::Tag{.Code = 410, .Value = std::string{"Model"}});
    CHECK(record->Data[19] == DwgIO::Tag{.Code = 420, .Value = std::int64_t{16711680}});
    CHECK(record->Data[20] == DwgIO::Tag{.Code = 440, .Value = std::int64_t{7}});
    CHECK(record->Data[21] == DwgIO::Tag{.Code = 460, .Value = 0.5});
    CHECK(record->Data[22] == DwgIO::Tag{.Code = 470, .Value = std::string{"tag"}});
    CHECK(record->Objects.empty());
    const Handle *empty = EntryNamed(*dictionary, "EMPTY", drawing);
    REQUIRE(empty);
    const auto *bare = GetIf<XRecord>(FindObject(drawing, *empty));
    REQUIRE(bare);
    CHECK(bare->Data.empty());
    CHECK(bare->Cloning == 1);
    CHECK(bare->Base.Owner == dictionary->Base.Self);
  }
}

TEST_CASE("objects/xrecord: a group of every kind and object handles round-trip as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const Objects::FileContext reading{.Release = Version::R2000, .Text = &Latin, .Classes = {}, .Issues = &issues};
  XRecord xrecord;
  xrecord.Base.Self = Handle{0x100};
  xrecord.Base.Owner = Handle{0xC};
  xrecord.Cloning = 2;
  xrecord.Data = {DwgIO::Tag{.Code = 1, .Value = std::string{"one"}},
                  DwgIO::Tag{.Code = 10, .Value = 1.0},
                  DwgIO::Tag{.Code = 20, .Value = 2.0},
                  DwgIO::Tag{.Code = 30, .Value = 3.0},
                  DwgIO::Tag{.Code = 40, .Value = 4.5},
                  DwgIO::Tag{.Code = 70, .Value = std::int64_t{7}},
                  DwgIO::Tag{.Code = 90, .Value = std::int64_t{-1}},
                  DwgIO::Tag{.Code = 280, .Value = std::int64_t{2}},
                  DwgIO::Tag{.Code = 290, .Value = false},
                  DwgIO::Tag{.Code = 310, .Value = Chunk{.Bytes = BytesOf("ab")}},
                  DwgIO::Tag{.Code = 340, .Value = Handle{0x1F}}};
  xrecord.Objects = {Handle{0x101}, Handle{0x50}};
  std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{xrecord}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{xrecord});
  xrecord.Data = {DwgIO::Tag{.Code = 70, .Value = std::int64_t{7}}};
  xrecord.Rest = {std::byte{0x01}, std::byte{0x00}, std::byte{0x08}, std::byte{0x00}, std::byte{0x1E}, std::byte{'t'}};
  xrecord.Objects.clear();
  back = Objects::ReadObject(Objects::WriteObject(Object{xrecord}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{xrecord});
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "XRECORD group 1 cut short");
  CHECK(issues.front().Subject == Handle{0x100});
}

TEST_CASE("objects/xrecord: a data size out of range fails and a missing handle stream is noted")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const auto read = [&](const std::vector<std::byte> &data, const std::vector<std::byte> *handles) -> bool
  {
    const std::vector<std::byte> none;
    Bits::Reader in{data};
    Bits::Reader stream{handles == nullptr ? none : *handles};
    Fields::Input reading{.In = &in, .Handles = handles == nullptr ? nullptr : &stream, .With = with};
    XRecord xrecord;
    return Fields::Read(Objects::Shape<XRecord>::Layout(), xrecord, reading);
  };
  Bits::Writer big;
  big.BL(1000000);
  CHECK_FALSE(read({big.Bytes().begin(), big.Bytes().end()}, nullptr));
  Bits::Writer negative;
  negative.BL(-1);
  CHECK_FALSE(read({negative.Bytes().begin(), negative.Bytes().end()}, nullptr));
  CHECK_FALSE(read({}, nullptr));
  REQUIRE(issues.size() == 2);
  CHECK(issues[0].What == "XRECORD data of 1000000 bytes out of range");
  CHECK(issues[1].What == "XRECORD data of -1 bytes out of range");
  Bits::Writer flagless;
  flagless.BL(0);
  const std::vector<std::byte> bare;
  CHECK_FALSE(read({flagless.Bytes().begin(), flagless.Bytes().end()}, &bare));
  Bits::Writer empty;
  empty.BL(0);
  empty.BS(1);
  CHECK(read({empty.Bytes().begin(), empty.Bytes().end()}, nullptr));
  // A handle whose counter says four bytes where none are left ends the object handles without a note.
  const std::vector<std::byte> cut{std::byte{0x04}};
  CHECK(read({empty.Bytes().begin(), empty.Bytes().end()}, &cut));
  CHECK(issues.size() == 2);
}
} // namespace DwgIO::Objects
