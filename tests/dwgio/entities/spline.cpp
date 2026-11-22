/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/spline.cpp
 *  @brief     The SPLINE against its DXF twin, written back, and its counts cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/spline.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/spline.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  The twin's points of that group, with the two groups ten and twenty above it.
 */
auto Points(const std::vector<Pair> &twin, std::size_t opened, int code) -> std::vector<Vector>
{
  const std::vector<std::string> xs = Every(twin, opened, code);
  const std::vector<std::string> ys = Every(twin, opened, code + 10);
  const std::vector<std::string> zs = Every(twin, opened, code + 20);
  REQUIRE(xs.size() == ys.size());
  REQUIRE(xs.size() == zs.size());
  std::vector<Vector> points;
  for (std::size_t at = 0; at < xs.size(); ++at)
  {
    points.push_back(Vector{.X = std::stod(xs[at]), .Y = std::stod(ys[at]), .Z = std::stod(zs[at])});
  }
  return points;
}

auto Reals(const std::vector<Pair> &twin, std::size_t opened, int code) -> std::vector<double>
{
  std::vector<double> reals;
  for (const std::string &value : Every(twin, opened, code))
  {
    reals.push_back(std::stod(value));
  }
  return reals;
}

void CheckNear(const std::vector<Vector> &ours, const std::vector<Vector> &theirs)
{
  REQUIRE(ours.size() == theirs.size());
  for (std::size_t at = 0; at < ours.size(); ++at)
  {
    CHECK(Near(ours[at], theirs[at]));
  }
}

void CheckNear(const std::vector<double> &ours, const std::vector<double> &theirs)
{
  REQUIRE(ours.size() == theirs.size());
  for (std::size_t at = 0; at < ours.size(); ++at)
  {
    CHECK(ours[at] == doctest::Approx(theirs[at]));
  }
}
} // namespace

TEST_CASE("entities/spline: the fixture's splines by fit points and by control points match the DXF twin")
{
  const std::vector<Pair> twin = ReadTwin("curves-r2000.dxf");
  for (const std::string_view name : {"curves-r2000.dwg", "curves-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    std::size_t fitted = 0;
    std::size_t weighted = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *spline = GetIf<Spline>(&object);
      if (spline == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(spline->Base.Self));
      CHECK(TypeOf(object) == "SPLINE");
      const std::optional<std::size_t> opened = Entity(twin, spline->Base.Self);
      REQUIRE(opened);
      const std::size_t at = *opened;
      CHECK(spline->Curve.Degree == ReadWhole(twin, at, 71));
      if (spline->Curve.FitTolerance)
      {
        ++fitted;
        CHECK(spline->Curve.Flags == 0);
        CHECK(*spline->Curve.FitTolerance == doctest::Approx(ReadReal(twin, at, 44)));
        CHECK(Near(spline->Curve.StartTangent, ReadPoint(twin, at, 12)));
        CHECK(Near(spline->Curve.EndTangent, ReadPoint(twin, at, 13)));
        CHECK(spline->Curve.FitPoints.size() == static_cast<std::size_t>(ReadWhole(twin, at, 74)));
        CheckNear(spline->Curve.FitPoints, Points(twin, at, 11));
        CHECK(spline->Curve.Knots.empty());
        CHECK(spline->Curve.ControlPoints.empty());
      }
      else
      {
        CHECK(spline->Curve.Flags == (ReadWhole(twin, at, 70) & 7));
        CHECK(spline->Curve.KnotTolerance == doctest::Approx(ReadReal(twin, at, 42)));
        CHECK(spline->Curve.ControlTolerance == doctest::Approx(ReadReal(twin, at, 43)));
        CHECK(spline->Curve.Knots.size() == static_cast<std::size_t>(ReadWhole(twin, at, 72)));
        CheckNear(spline->Curve.Knots, Reals(twin, at, 40));
        CHECK(spline->Curve.ControlPoints.size() == static_cast<std::size_t>(ReadWhole(twin, at, 73)));
        CheckNear(spline->Curve.ControlPoints, Points(twin, at, 10));
        CheckNear(spline->Curve.Weights, Reals(twin, at, 41));
        weighted += spline->Curve.Weights.empty() ? 0U : 1U;
        CHECK(spline->Curve.FitPoints.empty());
      }
      CheckCommon(TwinPlace{.Twin = &twin, .At = at, .Drawing = &loaded->Drawing}, spline->Common, spline->Base);
    }
    CHECK(count == 6);
    CHECK(fitted == 3);
    CHECK(weighted == 1);
  }
}

TEST_CASE("entities/spline: splines by fit points and by weighted control points round-trip as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Spline fitted;
  fitted.Base.Self = Handle{0x100};
  fitted.Common.Space = Space::Model;
  fitted.Common.Layer = Handle{0x10};
  fitted.Common.Lineweight = Lineweight::ByLayer;
  fitted.Curve.FitTolerance = 1e-10;
  fitted.Curve.StartTangent = Vector{.X = 1, .Y = 0, .Z = 0};
  fitted.Curve.EndTangent = Vector{.X = 0, .Y = 1, .Z = 0};
  fitted.Curve.FitPoints = {Vector{.X = 0, .Y = 0, .Z = 0}, Vector{.X = 5, .Y = 5, .Z = 0},
                            Vector{.X = 10, .Y = 0, .Z = 0}};
  std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{fitted}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{fitted});
  Spline poled = fitted;
  poled.Curve.FitTolerance.reset();
  poled.Curve.FitPoints.clear();
  poled.Curve.StartTangent = Vector{};
  poled.Curve.EndTangent = Vector{};
  poled.Curve.Flags = 4;
  poled.Curve.Degree = 2;
  poled.Curve.Knots = {0, 0, 0, 1, 1, 1};
  poled.Curve.ControlPoints = {Vector{.X = 0, .Y = 0, .Z = 0}, Vector{.X = 5, .Y = 5, .Z = 0},
                               Vector{.X = 10, .Y = 0, .Z = 0}};
  poled.Curve.Weights = {1, 2, 1};
  back = Objects::ReadObject(Objects::WriteObject(Object{poled}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{poled});
  poled.Curve.Flags = 3;
  poled.Curve.Weights.clear();
  back = Objects::ReadObject(Objects::WriteObject(Object{poled}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{poled});
  CHECK(issues.empty());
}

TEST_CASE("entities/spline: an oversized knot count stops the read and missing weights are written as 1")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  Bits::Writer data;
  data.BL(1);
  data.BL(3);
  data.B(false);
  data.B(false);
  data.B(false);
  data.BD(1e-9);
  data.BD(1e-10);
  data.BL(100000);
  Bits::Reader dataIn{data.Bytes()};
  Fields::Input in{.In = &dataIn, .With = with};
  Spline spline;
  CHECK_FALSE(Fields::Read(Objects::Shape<Spline>::Layout(), spline, in));
  CHECK(spline.Curve.Knots.empty());
  Bits::Writer poles;
  poles.BL(1);
  poles.BL(3);
  poles.B(false);
  poles.B(false);
  poles.B(false);
  poles.BD(1e-9);
  poles.BD(1e-10);
  poles.BL(0);
  poles.BL(100000);
  Bits::Reader polesIn{poles.Bytes()};
  Fields::Input polesReading{.In = &polesIn, .With = with};
  CHECK_FALSE(Fields::Read(Objects::Shape<Spline>::Layout(), spline, polesReading));
  Bits::Writer fits;
  fits.BL(2);
  fits.BL(3);
  fits.BD(1e-10);
  fits.Point3(Vector{});
  fits.Point3(Vector{});
  fits.BL(100000);
  Bits::Reader fitsIn{fits.Bytes()};
  Fields::Input fitsReading{.In = &fitsIn, .With = with};
  CHECK_FALSE(Fields::Read(Objects::Shape<Spline>::Layout(), spline, fitsReading));
  Spline weighted;
  weighted.Curve.Flags = 4;
  weighted.Curve.Degree = 1;
  weighted.Curve.Knots = {0, 0, 1, 1};
  weighted.Curve.ControlPoints = {Vector{.X = 0, .Y = 0, .Z = 0}, Vector{.X = 1, .Y = 1, .Z = 0}};
  weighted.Curve.Weights = {2};
  Bits::Writer out;
  Bits::Writer handles;
  Fields::Output to{.To = &out, .Handles = &handles, .With = with};
  REQUIRE(Fields::Write(Objects::Shape<Spline>::Layout(), weighted, to));
  Bits::Reader outIn{out.Bytes()};
  Fields::Input again{.In = &outIn, .With = with};
  Spline back;
  REQUIRE(Fields::Read(Objects::Shape<Spline>::Layout(), back, again));
  CHECK(back.Curve.Weights == std::vector{2.0, 1.0});
  CHECK(issues.empty());
}

TEST_CASE("entities/spline: a scenario the specification does not name is noted and stops the read")
{
  Bits::Writer data;
  data.BL(3);
  data.BL(3);
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  Bits::Reader dataIn{data.Bytes()};
  Fields::Input in{.In = &dataIn, .With = with};
  Spline spline;
  CHECK_FALSE(Fields::Read(Objects::Shape<Spline>::Layout(), spline, in));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "spline scenario 3 unknown");
}
} // namespace DwgIO::Entities
