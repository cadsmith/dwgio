/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/read.cpp
 *  @brief     Reading every object of a fixture under its map handle, and untyped objects' own bits.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/read.hpp"

#include "bits/crc.hpp"
#include "bits/reference.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "objects/context.hpp"
#include "objects/kinds.hpp"
#include "objects/map.hpp"
#include "objects/write.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/base.hpp"
#include "dwgio/class.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
auto All(Fixture &fixture) -> std::vector<Object>
{
  std::vector<Object> objects;
  for (const Mapped &entry : fixture.Entries)
  {
    const std::optional<Object> one = ReadObject(fixture.File, entry.Offset, fixture.Reading());
    REQUIRE_MESSAGE(one, HexOf(entry.Self));
    CHECK(BaseOf(*one).Self == entry.Self);
    objects.push_back(*one);
  }
  return objects;
}

auto Count(const std::vector<Object> &objects, std::string_view name) -> std::size_t
{
  return static_cast<std::size_t>(
      std::ranges::count_if(objects, [name](const Object &one) -> bool { return TypeOf(one) == name; }));
}

auto First(const std::vector<Object> &objects, std::string_view name) -> const Object &
{
  const auto found = std::ranges::find_if(objects, [name](const Object &one) -> bool { return TypeOf(one) == name; });
  REQUIRE(found != objects.end());
  return *found;
}

auto FindHandled(const std::vector<Object> &objects, Handle self) -> std::vector<Object>::const_iterator
{
  return std::ranges::find(objects, self, [](const Object &one) { return BaseOf(one).Self; });
}

auto IsEntity(const Object &object) -> bool
{
  return CommonOf(object) != nullptr;
}
} // namespace

TEST_CASE("objects/read: every object of the R2000 fixture reads under its map handle")
{
  Fixture fixture = Open("foundation-r2000.dwg");
  const std::vector<Object> objects = All(fixture);
  CHECK(objects.size() == 219);
  CHECK(fixture.Issues.empty());
  CHECK(Count(objects, "DICTIONARY") == 55);
  CHECK(Count(objects, "XRECORD") == 49);
  CHECK(Count(objects, "VISUALSTYLE") == 24);
  CHECK(Count(objects, "DICTIONARYVAR") == 18);
  CHECK(Count(objects, "SCALE") == 17);
  CHECK(Count(objects, "APPID") == 8);
  CHECK(Count(objects, "LINE") == 1);
  CHECK(Count(objects, "CIRCLE") == 1);
  CHECK(Count(objects, "ARC") == 1);
  CHECK(Count(objects, "LWPOLYLINE") == 1);
  CHECK(std::ranges::count_if(objects, IsEntity) == 10);
  std::size_t extended = 0;
  for (const Object &one : objects)
  {
    extended += BaseOf(one).Extended.size();
  }
  CHECK(extended == 36);
}

TEST_CASE("objects/read: every object of the R14 fixture reads under its map handle")
{
  Fixture fixture = Open("foundation-r14.dwg");
  const std::vector<Object> objects = All(fixture);
  CHECK(objects.size() == 305);
  CHECK(fixture.Issues.empty());
  CHECK(Count(objects, "XRECORD") == 110);
  CHECK(Count(objects, "LINE") == 1);
  const Object &line = First(objects, "LINE");
  const Common &common = *CommonOf(line);
  CHECK(common.Space == Space::Model);
  const auto layer = FindHandled(objects, common.Layer);
  REQUIRE(layer != objects.end());
  CHECK(TypeOf(*layer) == "LAYER");
  const auto control = FindHandled(objects, BaseOf(*layer).Owner);
  REQUIRE(control != objects.end());
  CHECK(TypeOf(*control) == "LAYER_CONTROL");
  CHECK(common.Previous == Handle{});
  CHECK(common.Next == Handle{BaseOf(line).Self.Value + 1});
  CHECK(CommonOf(First(objects, "CIRCLE"))->Next == Handle{0x2CE});
}

TEST_CASE("objects/read: the line's common data matches the DXF twin")
{
  Fixture fixture = Open("foundation-r2000.dwg");
  const std::vector<Object> objects = All(fixture);
  const std::vector<Pair> twin = ReadTwin("foundation-r2000.dxf");
  const Object &line = First(objects, "LINE");
  const Common &common = *CommonOf(line);
  const std::optional<std::size_t> opened = FindOpening(twin, "LINE");
  REQUIRE(opened);
  CHECK(BaseOf(line).Self == Self(twin, *opened));
  const std::optional<std::size_t> layer = FindOpening(twin, "LAYER", GroupOf(twin, *opened, 8));
  REQUIRE(layer);
  CHECK(common.Layer == Self(twin, *layer));
  CHECK(common.Space == Space::Model);
  CHECK_FALSE(BaseOf(line).Owner);
  CHECK(common.Color.Index == 256);
  CHECK(common.LinetypeScale == 1.0);
  CHECK(common.Lineweight == Lineweight::ByLayer);
  CHECK_FALSE(common.Invisible);
  CHECK(common.LinetypeOrigin == Origin::ByLayer);
  CHECK(common.Linetype == Handle{});
  CHECK(common.PlotstyleOrigin == Origin::ByLayer);
  CHECK(common.Plotstyle == Handle{});
  CHECK(common.Previous == Handle{});
  CHECK(common.Next == Handle{BaseOf(line).Self.Value + 1});
  const auto &polyline = Get<Entities::LwPolyline>(First(objects, "LWPOLYLINE"));
  CHECK(polyline.Common.Previous == Handle{0x2CE});
  CHECK(polyline.Common.Next == Handle{});
}

TEST_CASE("objects/read: table records are owned by their controls and dictionaries by their parents")
{
  Fixture fixture = Open("foundation-r2000.dwg");
  const std::vector<Object> objects = All(fixture);
  const std::vector<Pair> twin = ReadTwin("foundation-r2000.dxf");
  std::map<std::string, Handle> controls;
  for (const Object &one : objects)
  {
    const std::string name{TypeOf(one)};
    if (name.ends_with("_CONTROL"))
    {
      controls[name.substr(0, name.size() - 8)] = BaseOf(one).Self;
    }
  }
  CHECK(controls.size() == 10);
  for (const Object &one : objects)
  {
    const std::string name{TypeOf(one)};
    const std::string table = name == "BLOCK_HEADER" ? "BLOCK" : name;
    if (!IsEntity(one) && controls.contains(table))
    {
      CAPTURE(name);
      CHECK(BaseOf(one).Owner == controls.at(table));
    }
  }
  const std::optional<std::size_t> layer = FindOpening(twin, "LAYER", "0");
  REQUIRE(layer);
  const auto zero = FindHandled(objects, Self(twin, *layer));
  REQUIRE(zero != objects.end());
  CHECK(BaseOf(*zero).Owner == ParseHandle(GroupOf(twin, *layer, 330)));
  CHECK(BaseOf(*zero).Dictionary == Handle{0x13C});
  const auto child = FindHandled(objects, Handle{0xD});
  REQUIRE(child != objects.end());
  CHECK(BaseOf(*child).Owner == Handle{0xC});
  CHECK(BaseOf(*child).Reactors == std::vector{Handle{0xC}});
}

TEST_CASE("objects/read: the first xdata record is a style's font")
{
  Fixture fixture = Open("foundation-r2000.dwg");
  const std::vector<Object> objects = All(fixture);
  const auto style = FindHandled(objects, Handle{0x11});
  REQUIRE(style != objects.end());
  CHECK(TypeOf(*style) == "STYLE");
  REQUIRE(BaseOf(*style).Extended.size() == 1);
  CHECK(BaseOf(*style).Extended.front().AppId == Handle{0x12});
  CHECK(BaseOf(*style).Extended.front().Tags == std::vector{DwgIO::Tag{.Code = 1000, .Value = std::string{"Arial"}},
                                                            DwgIO::Tag{.Code = 1071, .Value = std::int64_t{34}}});
}

TEST_CASE("objects/read: a wrong CRC is noted with the handle and the object kept")
{
  Fixture fixture = Open("foundation-r2000.dwg");
  // The first object the library does not type, whose bytes are kept whole.
  const auto entry = std::ranges::find_if(fixture.Entries,
                                          [&fixture](const Mapped &one) -> bool
                                          {
                                            const std::optional<Object> read =
                                                ReadObject(fixture.File, one.Offset, fixture.Reading());
                                            return read && Holds<Unknown>(*read);
                                          });
  REQUIRE(entry != fixture.Entries.end());
  const std::optional<Object> sound = ReadObject(fixture.File, entry->Offset, fixture.Reading());
  REQUIRE(sound);
  fixture.File[entry->Offset + Get<Unknown>(*sound).Bytes.size() - 1] ^= std::byte{0x01};
  const std::optional<Object> read = ReadObject(fixture.File, entry->Offset, fixture.Reading());
  REQUIRE(read);
  CHECK(BaseOf(*read).Self == entry->Self);
  REQUIRE(fixture.Issues.size() == 1);
  CHECK(fixture.Issues.front() == Issue{.Code = IssueCode::Checksum,
                                        .Offset = entry->Offset,
                                        .Subject = entry->Self,
                                        .What = std::format("object {} CRC mismatch", HexOf(entry->Self))});
}

TEST_CASE("objects/read: an offset or size past the end of the file gives nothing and an issue")
{
  Fixture fixture = Open("foundation-r2000.dwg");
  const std::size_t beyond = fixture.File.size() + 10;
  CHECK_FALSE(ReadObject(fixture.File, beyond, fixture.Reading()));
  REQUIRE(fixture.Issues.size() == 1);
  CHECK(fixture.Issues.front() ==
        Issue{.Code = IssueCode::Damaged, .Offset = beyond, .What = "object at 138579 beyond the file"});
  fixture.Issues.clear();
  const Mapped last = *std::ranges::max_element(fixture.Entries, {}, &Mapped::Offset);
  fixture.File[last.Offset] = std::byte{0xFF};
  fixture.File[last.Offset + 1] = std::byte{0x7F};
  CHECK_FALSE(ReadObject(fixture.File, last.Offset, fixture.Reading()));
  REQUIRE(fixture.Issues.size() == 1);
  CHECK(fixture.Issues.front().What == std::format("object at {} runs past the end", last.Offset));
}

TEST_CASE("objects/read: a type with no class is kept unnamed with an issue")
{
  Fixture fixture = Open("foundation-r2000.dwg");
  Objects::FileContext with = fixture.Reading();
  with.Classes = {};
  std::uint16_t type = 0;
  const auto classed = std::ranges::find_if(fixture.Entries,
                                            [&](const Mapped &one) -> bool
                                            {
                                              const std::optional<Object> read =
                                                  ReadObject(fixture.File, one.Offset, fixture.Reading());
                                              const auto *unknown = read ? GetIf<Unknown>(&*read) : nullptr;
                                              type = unknown != nullptr ? unknown->Type : std::uint16_t{0};
                                              return type >= FirstClass;
                                            });
  REQUIRE(classed != fixture.Entries.end());
  fixture.Issues.clear();
  const std::optional<Object> read = ReadObject(fixture.File, classed->Offset, with);
  REQUIRE(read);
  const auto &unknown = Get<Unknown>(*read);
  CHECK(unknown.Type == type);
  CHECK(unknown.Class.IsEmpty());
  CHECK(TypeOf(*read).empty());
  REQUIRE(fixture.Issues.size() == 1);
  CHECK(fixture.Issues.front().What == std::format("type {} with no class", type));
  CHECK(fixture.Issues.front().Subject == unknown.Base.Self);
}

TEST_CASE("objects/read: an entity with its own graphics and owner and linetype and plot style reads")
{
  const auto build = [](std::uint32_t handles, std::uint32_t *start, bool junk) -> std::vector<std::byte>
  {
    Bits::Writer data;
    data.BS(0x2B); // An OLE frame of R13, a kind not typed: the common data is all it needs.
    data.RL(handles);
    data.H(Bits::Reference{.Code = 0, .Value = Handle{0x2CC}});
    data.BS(0);
    data.B(true);
    data.RL(2);
    data.RC(0xAA);
    data.RC(0xBB);
    data.BB(0);
    data.BL(1);
    data.B(true);
    data.ENC(Color{.Index = 1}, Version::R2000);
    data.BD(1.0);
    data.BB(3);
    data.BB(3);
    data.BS(1);
    data.RC(29);
    data.Fill();
    *start = static_cast<std::uint32_t>(data.Position());
    data.H(Bits::Reference{.Code = 4, .Value = Handle{0x1F}});
    data.H(Bits::Reference{.Code = 4, .Value = Handle{0xC}});
    data.H(Bits::Reference{.Code = 3, .Value = Handle{}});
    data.H(Bits::Reference{.Code = 5, .Value = Handle{0x10}});
    data.H(Bits::Reference{.Code = 5, .Value = Handle{0x15}});
    data.H(Bits::Reference{.Code = 5, .Value = Handle{0x1D}});
    if (junk)
    {
      data.RC(0x48); // A handle of eight bytes, one of them there.
      data.RC(0x01);
    }
    data.Fill();
    return FrameObject(data);
  };
  std::uint32_t start = 0;
  static_cast<void>(build(0, &start, false));
  const std::vector<std::byte> bytes = build(start, &start, false);
  std::vector<Issue> issues;
  const Objects::FileContext with{.Release = Version::R2000, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> read = ReadObject(bytes, 0, with);
  REQUIRE(read);
  CHECK(issues.empty());
  const auto &entity = Get<Unknown>(*read);
  CHECK(entity.Base.Owner == Handle{0x1F});
  CHECK(entity.Base.Reactors == std::vector{Handle{0xC}});
  CHECK(entity.Common.Layer == Handle{0x10});
  CHECK(entity.Common.LinetypeOrigin == Origin::Record);
  CHECK(entity.Common.Linetype == Handle{0x15});
  CHECK(entity.Common.PlotstyleOrigin == Origin::Record);
  CHECK(entity.Common.Plotstyle == Handle{0x1D});
  CHECK(entity.Common.Invisible);
  CHECK(entity.Common.Previous == Handle{0x2CB});
  CHECK(entity.Common.Next == Handle{0x2CD});
  // Its own bits: the data's padding, no handles but the common ones, no strings; written from them and its header,
  // it gives its bytes back.
  REQUIRE(entity.Body);
  CHECK(entity.Body->Release == Version::R2000);
  CHECK(entity.Body->Handles.Size == 0);
  Unknown bare = entity;
  bare.Bytes.clear();
  CHECK(WriteObject(bare, Fields::Context{.Release = Version::R2000, .Text = &Latin}) == bytes);
  // A handle stream that does not read as handles to its end is kept whole.
  const std::optional<Object> junk = ReadObject(build(start, &start, true), 0, with);
  REQUIRE(junk);
  REQUIRE(Get<Unknown>(*junk).Body);
  CHECK(Get<Unknown>(*junk).Body->Handles.Size == 16);
  // Its own bits are not known where its handles are not.
  const auto late = static_cast<std::uint32_t>(((bytes.size() - 4) * 8) - 4);
  const std::optional<Object> cut = ReadObject(build(late, &start, false), 0, with);
  REQUIRE(cut);
  CHECK_FALSE(Get<Unknown>(*cut).Body);
}

TEST_CASE("objects/read: an object cut inside its data or handles is noted")
{
  Bits::Writer tiny;
  tiny.BS(0x2A);
  std::vector<Issue> issues;
  const Objects::FileContext with{.Release = Version::R2000, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> small = ReadObject(FrameObject(tiny), 0, with);
  REQUIRE(small);
  CHECK(issues.back().What.ends_with("data cut short"));
  CHECK_FALSE(Get<Unknown>(*small).Body);
  const auto build = [](std::uint32_t handles, Handle self) -> std::vector<std::byte>
  {
    Bits::Writer data;
    data.BS(0x2A);
    data.RL(handles);
    data.H(Bits::Reference{.Code = 0, .Value = self});
    data.BS(0);
    data.BL(5);
    data.Fill();
    return FrameObject(data);
  };
  REQUIRE(ReadObject(build(5000, Handle{0x5}), 0, with));
  CHECK(issues.back().What == "object 5 handles beyond its data");
  const std::vector<std::byte> whole = build(0, Handle{0x6});
  const auto bits = static_cast<std::uint32_t>((whole.size() - 4) * 8);
  REQUIRE(ReadObject(build(bits - 4, Handle{0x6}), 0, with));
  CHECK(issues.back().What == "object 6 handles cut short");
  // The handle's code nibble fits; its counter nibble, at the data's end, does not.
  CHECK(issues.back().Offset == 2 + (bits / 8));
}

TEST_CASE("objects/read: an R2007 object whose handles start at its first bit has no data end bit")
{
  Bits::Writer data;
  data.BS(0x2A);
  data.RL(0);
  data.H(Bits::Reference{.Code = 0, .Value = Handle{0x7}});
  data.BS(0);
  data.BL(0);
  data.B(true);
  data.Fill();
  std::vector<Issue> issues;
  const Objects::FileContext with{.Release = Version::R2007, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> read = ReadObject(FrameObject(data), 0, with);
  REQUIRE(read);
  CHECK(BaseOf(*read).Self == Handle{0x7});
}

TEST_CASE("objects/read: issues are dropped where none are kept")
{
  Fixture fixture = Open("foundation-r2000.dwg");
  const Objects::FileContext quiet{
      .Release = Version::R2000, .Text = &Latin, .Classes = fixture.Classes, .Issues = nullptr};
  CHECK_FALSE(ReadObject(fixture.File, fixture.File.size() + 1, quiet));
  const auto entry = std::ranges::find(fixture.Entries, Handle{0xC}, &Mapped::Self);
  REQUIRE(entry != fixture.Entries.end());
  fixture.File[entry->Offset + 3] ^= std::byte{0x01};
  CHECK(ReadObject(fixture.File, entry->Offset, quiet));
}

TEST_CASE("objects/read: a size at the end of the file or without room for the CRC gives nothing")
{
  std::vector<Issue> issues;
  const Objects::FileContext with{.Release = Version::R2000, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::vector<std::byte> tail{std::byte{0x80}, std::byte{0x80}, std::byte{0x80}};
  CHECK_FALSE(ReadObject(tail, 0, with));
  CHECK(issues.back().What == "object at 0 runs past the end");
  const std::vector<std::byte> tight{std::byte{0x03}, std::byte{0x00}, std::byte{1}, std::byte{2}, std::byte{3}};
  CHECK_FALSE(ReadObject(tight, 0, with));
  CHECK(issues.back().What == "object at 0 runs past the end");
}

TEST_CASE("objects/read: a typed record cut inside its data or handles keeps what was read and is noted")
{
  const auto build = [](std::uint32_t handles, bool named, bool linked) -> std::vector<std::byte>
  {
    Bits::Writer data;
    data.BS(67);
    data.RL(handles);
    data.H(Bits::Reference{.Code = 0, .Value = Handle{0x50}});
    data.BS(0);
    data.BL(0);
    if (named)
    {
      data.TV("DWGIO", Version::R2000, Latin);
      data.B(false);
      data.BS(0);
      data.B(false);
      data.RC(0);
    }
    data.Fill();
    data.H(Bits::Reference{.Code = 4, .Value = Handle{0x9}});
    data.H(Bits::Reference{.Code = 3, .Value = Handle{}});
    if (linked)
    {
      data.H(Bits::Reference{.Code = 5, .Value = Handle{}});
    }
    data.Fill();
    return FrameObject(data);
  };
  const auto bits = [](bool named) -> std::uint32_t
  {
    Bits::Writer data;
    data.BS(67);
    data.RL(0);
    data.H(Bits::Reference{.Code = 0, .Value = Handle{0x50}});
    data.BS(0);
    data.BL(0);
    if (named)
    {
      data.TV("DWGIO", Version::R2000, Latin);
      data.B(false);
      data.BS(0);
      data.B(false);
      data.RC(0);
    }
    data.Fill();
    return static_cast<std::uint32_t>(data.Position());
  };
  std::vector<Issue> issues;
  const Objects::FileContext with{.Release = Version::R2000, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> whole = ReadObject(build(bits(true), true, true), 0, with);
  REQUIRE(whole);
  REQUIRE(Holds<Tables::AppId>(*whole));
  const auto &appid = Get<Tables::AppId>(*whole);
  CHECK(appid.Record.Name == "DWGIO");
  CHECK(appid.Record.Base.Self == Handle{0x50});
  CHECK(appid.Record.Base.Owner == Handle{0x9});
  CHECK(appid.Record.Source == Handle{});
  CHECK(appid.Record.XrefIndex == -1);
  CHECK(appid.Unknown1 == 0);
  CHECK(TypeOf(*whole) == "APPID");
  CHECK(issues.empty());
  const std::optional<Object> cut = ReadObject(build(bits(false), false, true), 0, with);
  REQUIRE(cut);
  REQUIRE(Holds<Tables::AppId>(*cut));
  CHECK(Get<Tables::AppId>(*cut).Record.Base.Self == Handle{0x50});
  REQUIRE(issues.size() == 1);
  CHECK(issues.back().What == "object 50 data cut short");
  const std::optional<Object> unlinked = ReadObject(build(bits(true), true, false), 0, with);
  REQUIRE(unlinked);
  REQUIRE(Holds<Tables::AppId>(*unlinked));
  CHECK(Get<Tables::AppId>(*unlinked).Record.Name == "DWGIO");
  REQUIRE(issues.size() == 2);
  CHECK(issues.back().What == "object 50 handles cut short");
}
} // namespace DwgIO::Objects
