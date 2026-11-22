/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/surface.cpp
 *  @brief     The ACIS geometry of the modeler entities written in each release, and cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/geometry.hpp"
#include "entities/shapes/modeler.hpp"
#include "entities/shapes/procedural.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "octets.hpp"
#include "support/reader.hpp"

#include "dwgio/class.hpp"
#include "dwgio/entities/geometry.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/entities/procedural.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  The classes the surfaces are stored under, entities all.
 */
auto SurfaceClasses() -> std::vector<Class>
{
  std::vector<Class> classes;
  int number = 500;
  for (const std::string_view name : {"SURFACE", "PLANESURFACE", "NURBSURFACE", "EXTRUDEDSURFACE", "LOFTEDSURFACE",
                                      "REVOLVEDSURFACE", "SWEPTSURFACE"})
  {
    classes.push_back(Class{.Number = number++, .DxfName = std::string{name}, .ItemClass = 0x1F2});
  }
  return classes;
}

/**
 *  The object written as the release and read back, with the issues of both.
 */
auto RoundTrip(const Object &object, Version release, std::vector<Issue> &issues) -> std::optional<Object>
{
  static const std::vector<Class> classes = SurfaceClasses();
  const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{
      .Release = release, .Text = &Objects::Latin, .Classes = classes, .Issues = &issues};
  return Objects::ReadObject(Objects::WriteObject(object, with), 0, reading);
}

auto MakeProfile(std::int32_t type, std::size_t bits) -> Profile
{
  Profile profile{.Type = type, .Data = {}};
  profile.Data.Size = bits;
  for (std::size_t at = 0; at < (bits + 7) / 8; ++at)
  {
    profile.Data.Bytes.push_back(static_cast<std::byte>(0xA0 + at));
  }
  if (bits % 8 != 0)
  {
    profile.Data.Bytes.back() &= static_cast<std::byte>(0xFF << (8 - (bits % 8)));
  }
  return profile;
}

/**
 *  A geometry with every part: a SAT body long enough for two blocks, a point, wires with and without a transform, a
 *  silhouette, and a second SAB stream.
 */
auto MakeFullGeometry() -> Geometry
{
  Geometry geometry;
  geometry.Body = Acis{.Version = 1, .Textual = true, .Data = BytesOf(std::string(5000, 'a') + "\t~ !\x7F\xE9\n")};
  geometry.Point = Vector{.X = 1, .Y = 2, .Z = 3};
  geometry.Isolines = 4;
  geometry.Wired = true;
  Wire plain{.Type = 1, .Marker = -3, .Color = 256, .Index = -1, .Points = {Vector{}, Vector{.X = 1}}, .Transform = {}};
  Wire placed = plain;
  placed.Transform = Transform{.XAxis = Right,
                               .YAxis = Vector{.Y = 1},
                               .ZAxis = Up,
                               .Translation = Vector{.X = 5},
                               .Scale = 2,
                               .Rotation = true,
                               .Reflection = false,
                               .Shear = true};
  geometry.Wires = {plain, placed};
  geometry.Silhouettes = {Silhouette{.Viewport = 2,
                                     .Target = Vector{},
                                     .Direction = Up,
                                     .Upward = Vector{.Y = 1},
                                     .Perspective = true,
                                     .Wires = {plain}}};
  geometry.Second = Acis{.Version = 2,
                         .Textual = false,
                         .Data = BytesOf(std::string{"ACIS BinaryFile"} + std::string{"\x0E\x03"
                                                                                      "End"
                                                                                      "\x0E\x02"
                                                                                      "of"
                                                                                      "\x0E\x04"
                                                                                      "ACIS"
                                                                                      "\x0D\x04"
                                                                                      "data"})};
  return geometry;
}
} // namespace

TEST_CASE("entities/surface: bodies and wires and silhouettes and a second stream round-trip in each release")
{
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2007, Version::R2013})
  {
    CAPTURE(NameOf(release));
    std::vector<Issue> issues;
    Region region;
    region.Base.Self = Handle{0x100};
    region.Common.Space = Space::Model;
    region.Common.Layer = Handle{0x10};
    region.Common.Lineweight = release >= Version::R2000 ? std::optional{Lineweight::ByLayer} : std::nullopt;
    region.Geometry = MakeFullGeometry();
    if (release >= Version::R2007)
    {
      region.Geometry.Unknown1 = 7;
    }
    if (release >= Version::R2013)
    {
      region.Geometry.Body = Acis{};
      region.Geometry.Stored = true;
      region.Geometry.Guid = "{1b628c31-6140-d449-b88d-b68a5d455c52}";
      region.Geometry.Unknown2 = 9;
    }
    const std::optional<Object> back = RoundTrip(Object{region}, release, issues);
    REQUIRE(back);
    CHECK(*back == Object{region});
    CHECK(issues.empty());
  }
}

TEST_CASE("entities/surface: an empty geometry has no wireframe and R2013 writes the null GUID")
{
  std::vector<Issue> issues;
  Body body;
  body.Base.Self = Handle{0x100};
  body.Common.Space = Space::Model;
  body.Common.Lineweight = Lineweight::ByLayer;
  body.Geometry.Framed = false;
  body.Geometry.Silhouettes = {Silhouette{}};
  std::optional<Object> back = RoundTrip(Object{body}, Version::R2000, issues);
  REQUIRE(back);
  CHECK(*back == Object{body});
  back = RoundTrip(Object{body}, Version::R2013, issues);
  REQUIRE(back);
  CHECK(Get<Body>(*back).Geometry.Guid == Shapes::NoGuid);
  body.Geometry.Guid = "{zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz}";
  REQUIRE(body.Geometry.Guid.size() == Shapes::NoGuid.size());
  back = RoundTrip(Object{body}, Version::R2013, issues);
  REQUIRE(back);
  CHECK(Get<Body>(*back).Geometry.Guid == Shapes::NoGuid);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "GUID {zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz} not understood, zero written");
  body.Geometry.Guid = "{1b628c31}";
  issues.clear();
  back = RoundTrip(Object{body}, Version::R2013, issues);
  REQUIRE(back);
  CHECK(issues.size() == 1);
  body.Geometry.Guid = "{1b628c31-6140-d449-b88d-b68a5d455c5g}";
  issues.clear();
  back = RoundTrip(Object{body}, Version::R2013, issues);
  REQUIRE(back);
  CHECK(issues.size() == 1);
}

TEST_CASE("entities/surface: a body of an unknown version or a SAB with no end leaves the object as bytes")
{
  for (const Acis &acis : {Acis{.Version = 3, .Textual = false, .Data = BytesOf("x")},
                           Acis{.Version = 2, .Textual = false, .Data = BytesOf("ACIS BinaryFile with no end")}})
  {
    CAPTURE(acis.Version);
    for (const bool second : {false, true})
    {
      std::vector<Issue> issues;
      Solid3d solid;
      solid.Base.Self = Handle{0x100};
      solid.Common.Space = Space::Model;
      (second ? solid.Geometry.Second : solid.Geometry.Body) = acis;
      const std::optional<Object> back = RoundTrip(Object{solid}, Version::R2000, issues);
      REQUIRE(back);
      CHECK(Holds<Unknown>(*back));
      REQUIRE_FALSE(issues.empty());
      CHECK(issues.front().What == std::format("ACIS data of version {} not read", acis.Version));
    }
  }
}

TEST_CASE("entities/surface: a SAT block count the data cannot hold or a cut wireframe stops the read")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  Bits::Writer data;
  data.B(false);
  data.B(true);
  data.BS(1);
  data.BL(100000);
  Bits::Reader in{data.Bytes()};
  Fields::Input reading{.In = &in, .With = with};
  Geometry geometry;
  CHECK_FALSE(Shapes::ReadGeometry(geometry, reading));
  Bits::Writer framed;
  framed.B(true); // No body.
  framed.B(true); // The wireframe, cut short.
  Bits::Reader cut{framed.Bytes()};
  Fields::Input cutShort{.In = &cut, .With = with};
  CHECK_FALSE(Shapes::ReadGeometry(geometry, cutShort));
  Bits::Writer wired;
  wired.B(true);
  wired.B(true);
  wired.B(false);
  wired.BL(0);
  wired.B(true);
  wired.BL(100000); // More wires than bits.
  Bits::Reader many{wired.Bytes()};
  Fields::Input wires{.In = &many, .With = with};
  CHECK_FALSE(Shapes::ReadGeometry(geometry, wires));
  Fields::Context newer = with;
  newer.Release = Version::R2013;
  Bits::Reader again{wired.Bytes()};
  again.B(); // From R2013 the wireframe comes first.
  Fields::Input late{.In = &again, .With = newer};
  CHECK_FALSE(Shapes::ReadGeometry(geometry, late));
}

TEST_CASE("entities/surface: a loft with guides and a path and a NURBS surface's own data round-trip")
{
  for (const Version release : {Version::R2000, Version::R2010, Version::R2018})
  {
    CAPTURE(NameOf(release));
    std::vector<Issue> issues;
    LoftedSurface loft;
    loft.Base.Self = Handle{0x100};
    loft.Common.Space = Space::Model;
    loft.Common.Lineweight = Lineweight::ByLayer;
    loft.Data.Sections = {MakeProfile(19, 152), MakeProfile(19, 13)};
    loft.Data.Guides = {MakeProfile(36, 9)};
    loft.Data.Path = MakeProfile(19, 0);
    loft.Data.StartDraft = 1.5;
    loft.Data.Ruled = true;
    loft.Data.Normal = 3;
    loft.Geometry.Guid = release >= Version::R2013 ? std::string{Shapes::NoGuid} : std::string{};
    std::optional<Object> back = RoundTrip(Object{loft}, release, issues);
    REQUIRE(back);
    CHECK(*back == Object{loft});
    NurbSurface nurbs;
    nurbs.Base.Self = Handle{0x101};
    nurbs.Common.Space = Space::Model;
    nurbs.Common.Lineweight = Lineweight::ByLayer;
    nurbs.Geometry.Guid = loft.Geometry.Guid;
    if (release >= Version::R2013)
    {
      nurbs.Data.Hull = 1;
      nurbs.Data.Unknown1 = true;
      nurbs.Data.Points[2] = Vector{.X = 1, .Y = 2, .Z = 3};
    }
    back = RoundTrip(Object{nurbs}, release, issues);
    REQUIRE(back);
    CHECK(*back == Object{nurbs});
    CHECK(TypeOf(*back) == "NURBSURFACE");
    CHECK(issues.empty());
  }
}

TEST_CASE("entities/surface: a SAB read outside an object runs to the reader's end")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const std::string sab = std::string{"ACIS BinaryFile"} + std::string{"\x0E\x03"
                                                                       "End"
                                                                       "\x0E\x02"
                                                                       "of"
                                                                       "\x0E\x03"
                                                                       "ASM"
                                                                       "\x0D\x04"
                                                                       "data"};
  Bits::Writer data;
  Shapes::WriteAcis(Acis{.Version = 2, .Textual = false, .Data = BytesOf(sab)},
                    Fields::Output{.To = &data, .With = with});
  Bits::Reader in{data.Bytes()};
  Acis acis;
  CHECK(Shapes::ReadAcis(acis, Fields::Input{.In = &in, .With = with}));
  CHECK(TextOf(acis.Data) == sab);
}

TEST_CASE("entities/surface: a loft whose guides or options or sections or path overrun the data stops the read")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};

  struct Cut
  {
    std::int32_t Sections;
    std::int32_t Guides;
    bool Path;
    bool Options;
  };

  for (const Cut cut : {Cut{.Sections = 1, .Guides = 100000, .Path = false, .Options = true},
                        Cut{.Sections = 0, .Guides = 0, .Path = false, .Options = false},
                        Cut{.Sections = 1, .Guides = 0, .Path = false, .Options = true},
                        Cut{.Sections = 0, .Guides = 1, .Path = false, .Options = true},
                        Cut{.Sections = 0, .Guides = 0, .Path = true, .Options = true}})
  {
    Bits::Writer data;
    for (int at = 0; at < 16; ++at)
    {
      data.BD(0);
    }
    data.BL(cut.Sections);
    data.BL(cut.Guides);
    data.B(cut.Path);
    if (cut.Options)
    {
      for (int at = 0; at < 4; ++at)
      {
        data.BD(0);
      }
      for (int at = 0; at < 8; ++at)
      {
        data.B(false);
      }
      data.BS(0);
      data.BL(19);
      data.BL(100000); // A profile of more bits than are left.
    }
    Bits::Reader in{data.Bytes()};
    Loft loft;
    CHECK_FALSE(Shapes::ReadLoft(loft, Fields::Input{.In = &in, .With = with}));
  }
}

TEST_CASE("entities/surface: a profile or loft count the data cannot hold stops the read")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  Bits::Writer data;
  data.BL(19);
  data.BL(100000);
  Bits::Reader in{data.Bytes()};
  Fields::Input reading{.In = &in, .With = with};
  Profile profile;
  CHECK_FALSE(Shapes::ReadProfile(profile, reading));
  Bits::Writer counts;
  for (int at = 0; at < 16; ++at)
  {
    counts.BD(0);
  }
  counts.BL(100000);
  Bits::Reader many{counts.Bytes()};
  Fields::Input lofted{.In = &many, .With = with};
  Loft loft;
  CHECK_FALSE(Shapes::ReadLoft(loft, lofted));
  Bits::Writer matrix;
  matrix.BD(0);
  Bits::Reader cut{matrix.Bytes()};
  Fields::Input shorter{.In = &cut, .With = with};
  CHECK_FALSE(Shapes::ReadLoft(loft, shorter));
}

TEST_CASE("entities/surface: each surface kind names the class it is stored under in a release")
{
  using Objects::Shape;
  CHECK(Shape<Surface>::ClassAt(Version::R2010) == "SURFACE");
  CHECK(Shape<Surface>::ClassAt(Version::R2013) == "NURBSURFACE");
  CHECK(Shape<NurbSurface>::ClassAt(Version::R2000) == "SURFACE");
  CHECK(Shape<PlaneSurface>::ClassAt(Version::R2013) == "PLANESURFACE");
  CHECK(Shape<ExtrudedSurface>::ClassAt(Version::R14) == "EXTRUDEDSURFACE");
  CHECK(Shape<SweptSurface>::ClassAt(Version::R2018) == "SWEPTSURFACE");
  CHECK(Shape<RevolvedSurface>::ClassAt(Version::R2004) == "REVOLVEDSURFACE");
  CHECK(Shape<LoftedSurface>::ClassAt(Version::R2007) == "LOFTEDSURFACE");
}
} // namespace DwgIO::Entities
