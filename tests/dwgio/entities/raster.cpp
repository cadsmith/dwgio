/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/raster.cpp
 *  @brief     The IMAGE and WIPEOUT against their DXF twin, their clips written back and cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/raster.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/raster.hpp"
#include "fields/context.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "releases/object.hpp"
#include "releases/omission.hpp"
#include "support/acad.hpp"
#include "support/cursor.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/saves.hpp"

#include "dwgio/class.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
auto Point3(Cursor &cursor, int code) -> Vector
{
  Vector point;
  point.X = cursor.Real(code);
  point.Y = cursor.Real(code + 10);
  point.Z = cursor.Real(code + 20);
  return point;
}

/**
 *  Checks a raster against its subclass in the twin, group by group, the clip mode only where the twin has it.
 */
void CheckRaster(const std::vector<Pair> &twin, std::size_t opened, const Raster &raster)
{
  Cursor cursor{twin, opened};
  cursor.Until(100);
  cursor.Take(100);
  cursor.Until(100);
  cursor.Take(100);
  CHECK(raster.Version == cursor.Whole(90));
  CHECK(Near(raster.Origin, Point3(cursor, 10)));
  CHECK(Near(raster.UVector, Point3(cursor, 11)));
  CHECK(Near(raster.VVector, Point3(cursor, 12)));
  CHECK(Near(raster.Size, cursor.Point(13)));
  CHECK(raster.Definition == ParseHandle(cursor.Take(340)).value_or(Handle{}));
  CHECK(raster.Flags == cursor.Whole(70));
  CHECK(raster.Clipped == (cursor.Whole(280) == 1));
  CHECK(raster.Brightness == cursor.Whole(281));
  CHECK(raster.Contrast == cursor.Whole(282));
  CHECK(raster.Fade == cursor.Whole(283));
  if (cursor.Code() == 290)
  {
    CHECK(raster.Outside == (cursor.Whole(290) == 1));
  }
  CHECK(raster.Reactor == ParseHandle(cursor.Take(360)).value_or(Handle{}));
  CHECK(raster.ClipShape == cursor.Whole(71));
  // DXF closes a polygon with its first vertex again, which the file leaves out.
  const std::size_t closing = raster.ClipShape == 1 ? 0 : 1;
  REQUIRE(raster.Corners.size() + closing == static_cast<std::size_t>(cursor.Whole(91)));
  for (const Pixel &corner : raster.Corners)
  {
    CHECK(Near(corner.Position, cursor.Point(14)));
  }
  if (closing == 1)
  {
    CHECK(Near(raster.Corners.front().Position, cursor.Point(14)));
  }
}
} // namespace

TEST_CASE("entities/raster: every release's two images and two wipeouts match the DXF twin")
{
  for (const std::string_view year : {"14", "2000", "2004", "2007", "2010", "2013", "2018"})
  {
    CAPTURE(year);
    const std::vector<Pair> twin = ReadTwin(std::format("rasters-r{}.dxf", year == "14" ? "2000" : year));
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("rasters-r{}.dwg", year)));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t images = 0;
    std::size_t wipeouts = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const Raster *raster = nullptr;
      const Common *common = CommonOf(object);
      if (const auto *image = GetIf<Image>(&object))
      {
        ++images;
        raster = &image->Raster;
        CHECK(TypeOf(object) == "IMAGE");
      }
      else if (const auto *wipeout = GetIf<Wipeout>(&object))
      {
        ++wipeouts;
        raster = &wipeout->Raster;
        CHECK(TypeOf(object) == "WIPEOUT");
      }
      else
      {
        continue;
      }
      CAPTURE(HexOf(BaseOf(object).Self));
      const std::optional<std::size_t> opened = Entity(twin, BaseOf(object).Self);
      REQUIRE(opened);
      CheckRaster(twin, *opened, *raster);
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, *common, BaseOf(object));
    }
    CHECK(images == 2);
    CHECK(wipeouts == 2);
  }
}

namespace
{
/**
 *  An object written in the release and read back, its classes those of the rasters fixtures.
 */
auto RoundTrip(const Object &object, Version release, std::vector<Issue> &issues) -> Object
{
  static const std::vector<Class> classes{Class{.Number = 500, .DxfName = "IMAGE", .ItemClass = 0x1F2},
                                          Class{.Number = 501, .DxfName = "WIPEOUT", .ItemClass = 0x1F2}};
  const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{
      .Release = release, .Text = &Objects::Latin, .Classes = classes, .Issues = &issues};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(object, with), 0, reading);
  REQUIRE(back);
  return *back;
}

auto Drawn() -> Common
{
  Common common;
  common.Space = Space::Model;
  common.Layer = Handle{0x10};
  common.Lineweight = Lineweight::ByLayer;
  return common;
}
} // namespace

TEST_CASE("entities/raster: a clipped image and a polygon wipeout round-trip in every release")
{
  for (const Version release : FixtureReleases)
  {
    CAPTURE(NameOf(release));
    std::vector<Issue> issues;
    Image image{.Base = {.Self = Handle{0x100}}, .Common = Drawn(), .Raster = {}};
    image.Raster.Origin = Vector{.X = 1, .Y = 2, .Z = 3};
    image.Raster.UVector = Vector{.X = 0.5, .Y = 0, .Z = 0};
    image.Raster.VVector = Vector{.X = 0, .Y = 0.5, .Z = 0};
    image.Raster.Size = Vector{.X = 640, .Y = 480, .Z = 0};
    image.Raster.Definition = Handle{0x50};
    image.Raster.Reactor = Handle{0x51};
    image.Raster.Clipped = true;
    image.Raster.Fade = 30;
    image.Raster.Outside = release >= Version::R2010;
    image.Raster.Corners = {Pixel{.Position = {.X = -0.5, .Y = -0.5, .Z = 0}},
                            Pixel{.Position = {.X = 639.5, .Y = 479.5, .Z = 0}}};
    CHECK(Get<Image>(RoundTrip(Object{image}, release, issues)).Raster == image.Raster);
    Wipeout wipeout{.Base = {.Self = Handle{0x101}}, .Common = Drawn(), .Raster = {}};
    wipeout.Raster.ClipShape = 2;
    wipeout.Raster.Clipped = true;
    wipeout.Raster.Corners = {Pixel{.Position = {.X = -0.5, .Y = 0.5, .Z = 0}},
                              Pixel{.Position = {.X = 0.5, .Y = 0.5, .Z = 0}},
                              Pixel{.Position = {.X = 0, .Y = -0.5, .Z = 0}}};
    CHECK(Get<Wipeout>(RoundTrip(Object{wipeout}, release, issues)).Raster == wipeout.Raster);
    CHECK(issues.empty());
  }
}

TEST_CASE("entities/raster: a clip rectangle without two corners is written with two and noted")
{
  std::vector<Issue> issues;
  Image image{.Base = {.Self = Handle{0x100}}, .Common = Drawn(), .Raster = {}};
  image.Raster.Corners = {Pixel{.Position = {.X = 4, .Y = 5, .Z = 0}}};
  const Object back = RoundTrip(Object{image}, Version::R2000, issues);
  const std::vector<Pixel> corners{Pixel{.Position = {.X = 4, .Y = 5, .Z = 0}}, Pixel{}};
  CHECK(Get<Image>(back).Raster.Corners == corners);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "clipping rectangle of 1 corners written with 2");
}

TEST_CASE("entities/raster: a negative or oversized polygon count or a missing boundary stops the read")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const auto read = [&with](const Bits::Writer &data) -> bool
  {
    Bits::Reader in{data.Bytes()};
    Fields::Input reading{.In = &in, .With = with};
    Raster raster;
    return Fields::Read(Fields::Layout(Fields::Custom(Shapes::ReadBoundary, Shapes::WriteBoundary)), raster, reading);
  };
  Bits::Writer big;
  big.BS(2);
  big.BL(1000);
  CHECK_FALSE(read(big));
  Bits::Writer negative;
  negative.BS(2);
  negative.BL(-1);
  CHECK_FALSE(read(negative));
  REQUIRE(issues.size() == 2);
  CHECK(issues[0].What == "clipping boundary count 1000 out of range");
  CHECK(issues[1].What == "clipping boundary count -1 out of range");
  CHECK_FALSE(read(Bits::Writer{}));
  CHECK(issues.size() == 2);
}

TEST_CASE("entities/raster: below R2010 an inverted clip is cleared and noted")
{
  Drawing drawing = Load(ReadFixture("rasters-r2018.dwg")).value().Drawing;
  Object *found = nullptr;
  for (Object &object : drawing.Objects)
  {
    found = Holds<Image>(object) ? &object : found;
  }
  REQUIRE(found != nullptr);
  Image &image = Get<Image>(*found);
  image.Raster.Outside = true;
  const DwgIO::Releases::Translation lowered = DwgIO::Releases::ConvertObject(*found, drawing, Version::R2007);
  CHECK_FALSE(Get<Image>(lowered.Object).Raster.Outside);
  REQUIRE(lowered.Issues.size() == 1);
  CHECK(lowered.Issues.front().What ==
        std::format("inverted clip of IMAGE {} not kept before R2010", HexOf(image.Base.Self)));
  CHECK(Get<Image>(DwgIO::Releases::ConvertObject(*found, drawing, Version::R2010).Object).Raster.Outside);
  Object detached = *found;
  std::vector<Issue> issues;
  const std::vector<Handle> leftOut{image.Raster.Definition, image.Raster.Reactor};
  DwgIO::Releases::ClearPointers(detached, leftOut, issues);
  CHECK(Get<Image>(detached).Raster.Definition == Handle{});
  CHECK(Get<Image>(detached).Raster.Reactor == Handle{});
  REQUIRE(issues.size() == 2);
  CHECK(issues.front().What == std::format("image definition of IMAGE {} cleared, {} left out", HexOf(image.Base.Self),
                                           HexOf(image.Raster.Definition)));
}

TEST_CASE("entities/raster: every rasters fixture saves to its own release byte for byte")
{
  for (const std::string_view year : {"14", "2000", "2004", "2007", "2010", "2013", "2018"})
  {
    CheckSameSave(std::format("rasters-r{}.dwg", year));
  }
}

TEST_CASE("entities/raster: the rasters fixtures saved to other releases keep their images and wipeouts")
{
  struct Crossing
  {
    std::string_view From;
    Version To;
  };

  for (const Crossing crossing : {Crossing{.From = "rasters-r2018.dwg", .To = Version::R2000},
                                  Crossing{.From = "rasters-r2000.dwg", .To = Version::R2018},
                                  Crossing{.From = "rasters-r2007.dwg", .To = Version::R2013},
                                  Crossing{.From = "rasters-r2004.dwg", .To = Version::R14},
                                  Crossing{.From = "rasters-r14.dwg", .To = Version::R2010},
                                  Crossing{.From = "rasters-r2013.dwg", .To = Version::R2007}})
  {
    const Drawing back = CheckSaveAs(crossing.From, crossing.To);
    std::size_t images = 0;
    std::size_t wipeouts = 0;
    for (const Object &object : back.Objects)
    {
      images += Holds<Image>(object) ? 1U : 0U;
      wipeouts += Holds<Wipeout>(object) ? 1U : 0U;
    }
    CHECK(images == 2);
    CHECK(wipeouts == 2);
  }
}

TEST_CASE("entities/raster: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Entities
