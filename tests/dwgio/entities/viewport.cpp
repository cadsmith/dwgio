/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/viewport.cpp
 *  @brief     The VIEWPORT against its DXF twin, and its frozen layers cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/viewport.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/viewport.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
constexpr double Degree = std::numbers::pi / 180.0;

auto Pair2(const std::vector<Pair> &twin, std::size_t at, int code) -> Vector
{
  return Vector{.X = ReadReal(twin, at, code), .Y = ReadReal(twin, at, code + 10), .Z = 0};
}

/**
 *  The R2000 fields against the twin's groups.
 */
void CheckView(const std::vector<Pair> &twin, std::size_t at, const Viewport &viewport)
{
  CHECK(Near(viewport.Target, ReadPoint(twin, at, 17)));
  CHECK(Near(viewport.Direction, ReadPoint(twin, at, 16)));
  CHECK(viewport.Twist == doctest::Approx(ReadReal(twin, at, 51) * Degree));
  CHECK(viewport.ViewHeight == doctest::Approx(ReadReal(twin, at, 45)));
  CHECK(viewport.LensLength == doctest::Approx(ReadReal(twin, at, 42)));
  CHECK(viewport.Front == doctest::Approx(ReadReal(twin, at, 43)));
  CHECK(viewport.Back == doctest::Approx(ReadReal(twin, at, 44)));
  CHECK(viewport.SnapAngle == doctest::Approx(ReadReal(twin, at, 50) * Degree));
  CHECK(Near(viewport.ViewCenter, Pair2(twin, at, 12)));
  CHECK(Near(viewport.SnapBase, Pair2(twin, at, 13)));
  CHECK(Near(viewport.Snap, Pair2(twin, at, 14)));
  CHECK(Near(viewport.Grid, Pair2(twin, at, 15)));
  CHECK(viewport.Zoom == ReadWhole(twin, at, 72));
  CHECK(viewport.Flags == ReadWhole(twin, at, 90));
  CHECK(viewport.Sheet == GroupOf(twin, at, 1));
  CHECK(viewport.Render == ReadWhole(twin, at, 281));
  CHECK(viewport.UcsIconAtOrigin == (ReadWhole(twin, at, 74) == 1));
  CHECK(viewport.UcsPerViewport == (ReadWhole(twin, at, 71) == 1));
  CHECK(Near(viewport.Axes.Origin, ReadPoint(twin, at, 110)));
  CHECK(Near(viewport.Axes.X, ReadPoint(twin, at, 111)));
  CHECK(Near(viewport.Axes.Y, ReadPoint(twin, at, 112)));
  CHECK(viewport.Axes.Elevation == doctest::Approx(ReadReal(twin, at, 146)));
  CHECK(viewport.Axes.Ortho == ReadWhole(twin, at, 79));
  CHECK(viewport.Axes.Named == ParseHandle(GroupOf(twin, at, 345)).value_or(Handle{}));
  CHECK(viewport.Axes.Basis == ParseHandle(GroupOf(twin, at, 346)).value_or(Handle{}));
  CHECK(viewport.Clip == ParseHandle(GroupOf(twin, at, 340)).value_or(Handle{}));
  std::vector<Handle> frozen;
  for (const std::string &layer : Every(twin, at, 341))
  {
    frozen.push_back(ParseHandle(layer).value_or(Handle{}));
  }
  CHECK(viewport.Frozen == frozen);
}
} // namespace

TEST_CASE("entities/viewport: the fixture's five viewports match the DXF twin")
{
  const std::vector<Pair> twin = ReadTwin("curves-r2000.dxf");
  const std::optional<std::size_t> hidden = FindOpeningNoCase(twin, "LAYER", "Hidden");
  REQUIRE(hidden);
  for (const std::string_view name : {"curves-r2000.dwg", "curves-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const bool modern = loaded->Drawing.Release >= Version::R2000;
    std::size_t count = 0;
    std::size_t clipped = 0;
    std::size_t freezing = 0;
    std::size_t papers = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *viewport = GetIf<Viewport>(&object);
      if (viewport == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(viewport->Base.Self));
      CHECK(TypeOf(object) == "VIEWPORT");
      const std::optional<std::size_t> opened = Entity(twin, viewport->Base.Self);
      REQUIRE(opened);
      const std::size_t at = *opened;
      CHECK(Near(viewport->Center, ReadPoint(twin, at, 10)));
      CHECK(viewport->Width == doctest::Approx(ReadReal(twin, at, 40)));
      CHECK(viewport->Height == doctest::Approx(ReadReal(twin, at, 41)));
      if (modern)
      {
        CheckView(twin, at, *viewport);
        clipped += viewport->Clip ? 1U : 0U;
        freezing += std::ranges::contains(viewport->Frozen, Self(twin, *hidden)) ? 1U : 0U;
        if (ReadWhole(twin, at, 69) == 1)
        {
          ++papers;
          CHECK_FALSE(viewport->Clip);
          CHECK(viewport->Frozen.empty());
        }
      }
      else
      {
        CHECK(viewport->Header != Handle{});
        CHECK(TypeOf(*std::ranges::find(loaded->Drawing.Objects, viewport->Header,
                                        [](const Object &one) { return BaseOf(one).Self; })) == "VP_ENT_HDR");
      }
      CheckCommon(TwinPlace{.Twin = &twin, .At = at, .Drawing = &loaded->Drawing}, viewport->Common, viewport->Base);
    }
    CHECK(count == 5);
    if (modern)
    {
      CHECK(clipped == 2);
      CHECK(freezing == 1);
      CHECK(papers == 1);
    }
  }
}

TEST_CASE("entities/viewport: a frozen layer count the data cannot hold stops the read")
{
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin};
  Bits::Writer data;
  data.Point3(Vector{});
  data.BD(1);
  data.BD(1);
  data.Point3(Vector{});
  data.Point3(Up);
  for (int at = 0; at < 6; ++at)
  {
    data.BD(0);
  }
  for (int at = 0; at < 8; ++at)
  {
    data.RD(0);
  }
  data.BS(1000);
  data.BL(100000);
  Bits::Reader dataIn{data.Bytes()};
  Fields::Input in{.In = &dataIn, .Handles = nullptr, .With = with};
  Viewport back;
  CHECK_FALSE(Fields::Read(Objects::Shape<Viewport>::Layout(), back, in));
  CHECK(back.Frozen.empty());
}

TEST_CASE("entities/viewport: a frozen layer count the handle stream cannot fill cuts the handles short")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  Viewport viewport;
  viewport.Frozen = {Handle{0x10}, Handle{0x11}, Handle{0x12}, Handle{0x13}, Handle{0x14}};
  viewport.Clip = Handle{0x20};
  Bits::Writer data;
  Bits::Writer handles;
  Fields::Output to{.To = &data, .Handles = &handles, .With = with};
  REQUIRE(Fields::Write(Objects::Shape<Viewport>::Layout(), viewport, to));
  Bits::Reader dataIn{data.Bytes()};
  Bits::Reader handlesIn{handles.Bytes().first(4)};
  Fields::Input in{.In = &dataIn, .Handles = &handlesIn, .With = with};
  Viewport back;
  CHECK_FALSE(Fields::Read(Objects::Shape<Viewport>::Layout(), back, in));
  CHECK(handlesIn.HasFailed());
  CHECK(back.Frozen.size() == 5);
  Bits::Reader wholeIn{handles.Bytes()};
  Bits::Reader again{data.Bytes()};
  Fields::Input whole{.In = &again, .Handles = &wholeIn, .With = with};
  REQUIRE(Fields::Read(Objects::Shape<Viewport>::Layout(), back, whole));
  CHECK(back == viewport);
  CHECK(issues.empty());
}
} // namespace DwgIO::Entities
