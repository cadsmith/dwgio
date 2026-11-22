/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/later.cpp
 *  @brief     What R2010 and later add to MTEXT, attributes, polylines and splines, and cuts of it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/hatch.hpp"
#include "entities/shapes/lwpolyline.hpp"
#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "file/paged/head.hpp"
#include "objects/items.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/reader.hpp"
#include "variables/layout.hpp"

#include "dwgio/entities/attdef.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/hatch.hpp"
#include "dwgio/entities/lwpolyline.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
/**
 *  An object written as a release and read back as it, with the issues of both.
 */
auto RoundTrip(const Object &object, Version release, std::vector<Issue> &issues) -> Object
{
  const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues};
  const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> back = ReadObject(WriteObject(object, with), 0, reading);
  REQUIRE(back);
  return *back;
}

auto RoundTrip(const Object &object, Version release) -> Object
{
  std::vector<Issue> issues;
  const Object back = RoundTrip(object, release, issues);
  CHECK(issues.empty());
  return back;
}

template <typename KindT>
auto Entity() -> KindT
{
  KindT kind;
  kind.Base.Self = Handle{0x40};
  kind.Common.Layer = Handle{0x10};
  kind.Common.Lineweight = Lineweight::ByLayer;
  return kind;
}
} // namespace

TEST_CASE("objects/later: an R2018 MTEXT keeps its static and dynamic columns and an annotative one has none")
{
  auto mtext = Entity<Entities::MText>();
  mtext.Style = Handle{0x11};
  mtext.Defined = 7;
  mtext.Application = Handle{0x12};
  mtext.Columns = Entities::Columns{
      .Kind = 2, .Count = 2, .Breadth = 3, .Gutter = 0.5, .Automatic = false, .Reversed = true, .Heights = {4, 5}};
  CHECK(Get<Entities::MText>(RoundTrip(mtext, Version::R2018)) == mtext);
  mtext.Columns.Kind = 1;
  mtext.Columns.Heights.clear();
  CHECK(Get<Entities::MText>(RoundTrip(mtext, Version::R2018)) == mtext);
  // Dynamic columns of one height keep none of their own.
  mtext.Columns.Kind = 2;
  mtext.Columns.Automatic = true;
  CHECK(Get<Entities::MText>(RoundTrip(mtext, Version::R2018)) == mtext);
  // An annotative text stores nothing of the frame; R2013 none of it, and R2004 not the rectangle's height.
  auto annotative = Entity<Entities::MText>();
  annotative.Annotative = true;
  annotative.Style = Handle{0x11};
  CHECK(Get<Entities::MText>(RoundTrip(annotative, Version::R2018)) == annotative);
  const auto older = Get<Entities::MText>(RoundTrip(mtext, Version::R2013));
  CHECK(older.Columns == Entities::Columns{});
  CHECK(older.Application == Handle{});
  CHECK(older.Defined == 7);
  CHECK(Get<Entities::MText>(RoundTrip(mtext, Version::R2004)).Defined == 0);
  // A count of column heights the data cannot hold stops the read.
  mtext.Columns.Automatic = false;
  mtext.Columns.Count = 100000;
  mtext.Columns.Heights.clear();
  std::vector<Issue> issues;
  static_cast<void>(RoundTrip(mtext, Version::R2018, issues));
  REQUIRE_FALSE(issues.empty());
  CHECK(issues.front().What == "object 40 data cut short");
}

TEST_CASE("objects/later: an R2018 multiline attribute is noted and kept as bytes")
{
  auto attrib = Entity<Entities::Attrib>();
  attrib.Type = 2;
  attrib.Tag = "T";
  std::vector<Issue> issues;
  const Object back = RoundTrip(attrib, Version::R2018, issues);
  REQUIRE(Holds<Unknown>(back));
  CHECK(Get<Unknown>(back).Type == 2);
  CHECK(Get<Unknown>(back).Base.Self == attrib.Base.Self);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "multiline attribute of type 2 not read");
  // A multiline definition the same way.
  auto attdef = Entity<Entities::AttDef>();
  attdef.Type = 4;
  issues.clear();
  REQUIRE(Holds<Unknown>(RoundTrip(attdef, Version::R2018, issues)));
  CHECK(issues.front().What == "multiline attribute of type 4 not read");
  attdef.Type = 1;
  CHECK(Get<Entities::AttDef>(RoundTrip(attdef, Version::R2018)) == attdef);
  // A single line one keeps its version and lock from R2010, the lock alone in R2007.
  attrib.Type = 1;
  attrib.Revision = 0;
  attrib.Locked = true;
  CHECK(Get<Entities::Attrib>(RoundTrip(attrib, Version::R2018)) == attrib);
  CHECK(Get<Entities::Attrib>(RoundTrip(attrib, Version::R2007)).Locked);
}

TEST_CASE("objects/later: a lightweight polyline keeps vertex identifiers from R2010 and drops them before")
{
  auto polyline = Entity<Entities::LwPolyline>();
  polyline.Vertices = {
      Entities::LwPolyline::Vertex{.Position = {.X = 1, .Y = 2, .Z = 0}, .Bulge = 0, .Widths = {}, .Id = 7},
      Entities::LwPolyline::Vertex{.Position = {.X = 3, .Y = 4, .Z = 0}, .Bulge = 0, .Widths = {}, .Id = 8}};
  CHECK(Get<Entities::LwPolyline>(RoundTrip(polyline, Version::R2010)) == polyline);
  const auto older = Get<Entities::LwPolyline>(RoundTrip(polyline, Version::R2004));
  CHECK_FALSE(older.Vertices.front().Id);
  CHECK_FALSE(older.Vertices.back().Id);
}

TEST_CASE("objects/later: surplus vertex identifiers are dropped and an oversized count stops the read")
{
  const auto read = [](const Bits::Writer &data, Entities::LwPolyline &polyline) -> bool
  {
    const Fields::Context with{.Release = Version::R2010, .Text = &Latin};
    Bits::Reader dataIn{data.Bytes()};
    Fields::Input in{.In = &dataIn, .With = with};
    return Fields::Read(Shape<Entities::LwPolyline>::Layout(), polyline, in);
  };
  Bits::Writer beyond;
  beyond.BS(1024);
  beyond.BL(1);
  beyond.BL(2);
  beyond.RD(1);
  beyond.RD(2);
  beyond.BL(5);
  beyond.BL(6);
  Entities::LwPolyline polyline;
  REQUIRE(read(beyond, polyline));
  REQUIRE(polyline.Vertices.size() == 1);
  CHECK(polyline.Vertices.front().Id == 5);
  Bits::Writer many;
  many.BS(1024);
  many.BL(1);
  many.BL(1000);
  CHECK_FALSE(read(many, polyline));
}

TEST_CASE("objects/later: a hatch's spline edge keeps its fit points and tangents from R2010")
{
  Entities::Edges::Spline spline{.Degree = 3,
                                 .Rational = false,
                                 .Periodic = false,
                                 .Knots = {0, 0, 0, 0, 1, 1, 1, 1},
                                 .ControlPoints = std::vector<Entities::Edges::Node>(4),
                                 .FitPoints = {{.X = 1, .Y = 1, .Z = 0}, {.X = 2, .Y = 3, .Z = 0}},
                                 .StartTangent = {.X = 1, .Y = 0, .Z = 0},
                                 .EndTangent = {.X = 0, .Y = 1, .Z = 0}};
  const Fields::Context with{.Release = Version::R2010, .Text = &Latin};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  Entities::Shapes::WriteSplineEdge(spline, to);
  Bits::Reader dataIn{out.Bytes()};
  Fields::Input in{.In = &dataIn, .With = with};
  Entities::Edges::Spline back;
  REQUIRE(Entities::Shapes::ReadSplineEdge(back, in));
  CHECK(back == spline);
  // With no fit points there are no tangents either.
  Entities::Edges::Spline plain = spline;
  plain.FitPoints.clear();
  plain.StartTangent = {};
  plain.EndTangent = {};
  Bits::Writer bare;
  Fields::Output toBare{.To = &bare, .With = with};
  Entities::Shapes::WriteSplineEdge(plain, toBare);
  Bits::Reader bareIn{bare.Bytes()};
  Fields::Input fromBare{.In = &bareIn, .With = with};
  REQUIRE(Entities::Shapes::ReadSplineEdge(back, fromBare));
  CHECK(back == plain);
  // A fit count the data cannot hold stops the read.
  Bits::Writer cut;
  cut.BL(3);
  cut.B(false);
  cut.B(false);
  cut.BL(0);
  cut.BL(0);
  cut.BL(1000);
  Bits::Reader cutIn{cut.Bytes()};
  Fields::Input short_{.In = &cutIn, .With = with};
  CHECK_FALSE(Entities::Shapes::ReadSplineEdge(back, short_));
}

TEST_CASE("objects/later: an R2010 xrecord's text is UTF-16 capped at 65535 units")
{
  const std::vector<DwgIO::Tag> groups{DwgIO::Tag{.Code = 1, .Value = std::string{"caf\xc3\xa9"}},
                                       DwgIO::Tag{.Code = 3, .Value = std::string(70000, 'x')}};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2010, .Text = &Latin, .Issues = &issues};
  Bits::Writer out;
  const Fields::Output to{.To = &out, .With = with};
  WriteGroups(groups, {}, to);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "XRECORD text of 70000 code units cut to 65535");
  Bits::Reader in{out.Bytes()};
  const Fields::Input reading{.In = &in, .With = with};
  const auto [read, rest] = ReadGroups(reading, out.Bytes().size());
  REQUIRE(read.size() == 2);
  CHECK(read.front() == groups.front());
  CHECK(std::get<std::string>(read.back().Value) == std::string(65535, 'x'));
  CHECK(rest.empty());
  // A text whose count needs more units than the data has, and one with no room for its count.
  for (const std::vector<std::byte> &bytes :
       {std::vector<std::byte>{std::byte{0x01}, std::byte{0x00}, std::byte{0x09}, std::byte{0x00}, std::byte{0x41}},
        std::vector<std::byte>{std::byte{0x01}, std::byte{0x00}, std::byte{0x09}}})
  {
    std::vector<Issue> noted;
    const Fields::Context cut{.Release = Version::R2010, .Text = &Latin, .Issues = &noted};
    Bits::Reader bits{bytes};
    const Fields::Input short_{.In = &bits, .With = cut};
    const auto [none, kept] = ReadGroups(short_, bytes.size());
    CHECK(none.empty());
    CHECK(kept == bytes);
    REQUIRE(noted.size() == 1);
    CHECK(noted.front().What.ends_with("cut short"));
  }
}

TEST_CASE("objects/later: an R2010 linetype's text runs to the end of its strings area without a NUL")
{
  Tables::LType ltype;
  ltype.Record.Base.Self = Handle{0x40};
  ltype.Record.Name = "Texted";
  ltype.Dashes = {Tables::Dash{.Length = 1, .Shape = 508, .Flags = 2}};
  ltype.Strings = std::vector<std::byte>(512, std::byte{0});
  ltype.Strings[508] = std::byte{'A'};
  ltype.Strings[510] = std::byte{'B'};
  const auto back = Get<Tables::LType>(RoundTrip(ltype, Version::R2010));
  CHECK(back.Dashes.front().Text == "AB");
  CHECK(back.Strings == ltype.Strings);
}

TEST_CASE("objects/later: the header frame has a second RL in R2010 and R2013 after maintenance 3 and in R2018")
{
  CHECK_FALSE(File::Paged::IsWide(Version::R2004, 104));
  CHECK_FALSE(File::Paged::IsWide(Version::R2010, 3));
  CHECK(File::Paged::IsWide(Version::R2010, 4));
  CHECK(File::Paged::IsWide(Version::R2013, 125));
  CHECK(File::Paged::IsWide(Version::R2018, 0));
}

TEST_CASE("objects/later: a header handle from R2007 with no handle stream stays with the data")
{
  DwgIO::Header header;
  header.Clayer = Handle{0x99};
  header.Handseed = Handle{0x400};
  const Fields::Context with{.Release = Version::R2010, .Text = &Latin};
  Bits::Writer out;
  Fields::Output to{.To = &out, .With = with};
  REQUIRE(Fields::Write(Variables::Layout(), header, to));
  Bits::Reader dataIn{out.Bytes()};
  Fields::Input in{.In = &dataIn, .With = with};
  DwgIO::Header back;
  REQUIRE(Fields::Read(Variables::Layout(), back, in));
  CHECK(back.Clayer == Handle{0x99});
  CHECK(back.Handseed == Handle{0x400});
}
} // namespace DwgIO::Objects
