/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/helix.cpp
 *  @brief     The HELIX against its DXF twin in every release, and its spline's flags on upgrading.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/helix.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "releases/object.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/class.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
auto Marker(const std::vector<Pair> &twin, std::size_t opened, const std::string &name) -> std::size_t
{
  std::size_t at = opened + 1;
  while (twin[at].Code != 0 && !(twin[at].Code == 100 && twin[at].Value == name))
  {
    ++at;
  }
  REQUIRE(twin[at].Code == 100);
  return at;
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
} // namespace

TEST_CASE("entities/helix: the helix matches the DXF twin in every release as a spline by control points")
{
  for (const auto &[name, twinned] :
       {std::pair{"kinds-r14.dwg", "kinds-r2000.dxf"}, std::pair{"kinds-r2000.dwg", "kinds-r2000.dxf"},
        std::pair{"kinds-r2004.dwg", "kinds-r2004.dxf"}, std::pair{"kinds-r2007.dwg", "kinds-r2007.dxf"},
        std::pair{"kinds-r2010.dwg", "kinds-r2010.dxf"}, std::pair{"kinds-r2013.dwg", "kinds-r2013.dxf"},
        std::pair{"kinds-r2018.dwg", "kinds-r2018.dxf"}})
  {
    CAPTURE(name);
    const std::vector<Pair> twin = ReadTwin(twinned);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    std::size_t seen = 0;
    for (const Object &object : drawing.Objects)
    {
      const auto *helix = GetIf<Helix>(&object);
      if (helix == nullptr)
      {
        continue;
      }
      ++seen;
      CHECK(TypeOf(object) == "HELIX");
      const std::optional<std::size_t> at = Entity(twin, helix->Base.Self);
      REQUIRE(at);
      CheckCommon(TwinPlace{.Twin = &twin, .At = *at, .Drawing = &drawing}, helix->Common, helix->Base);
      const std::size_t spline = Marker(twin, *at, "AcDbSpline");
      const Curve &curve = helix->Curve;
      CHECK(curve.Flags == ReadWhole(twin, spline, 70));
      CHECK(curve.Degree == ReadWhole(twin, spline, 71));
      CHECK(curve.KnotTolerance == doctest::Approx(ReadReal(twin, spline, 42)));
      CHECK(curve.ControlTolerance == doctest::Approx(ReadReal(twin, spline, 43)));
      CHECK_FALSE(curve.FitTolerance);
      std::vector<double> knots = Reals(twin, spline, 40); // The helix's radius is a 40 group too.
      knots.resize(static_cast<std::size_t>(ReadWhole(twin, spline, 72)));
      REQUIRE(curve.Knots.size() == knots.size());
      for (std::size_t one = 0; one < knots.size(); ++one)
      {
        CHECK(curve.Knots[one] == doctest::Approx(knots[one]));
      }
      REQUIRE(curve.ControlPoints.size() == static_cast<std::size_t>(ReadWhole(twin, spline, 73)));
      const std::vector<double> xs = Reals(twin, spline, 10);
      for (std::size_t one = 0; one < curve.ControlPoints.size(); ++one)
      {
        CHECK(curve.ControlPoints[one].X == doctest::Approx(xs[one]));
      }
      CHECK(curve.FitPoints.empty());
      CHECK(curve.Options == 0);
      CHECK(curve.Parameter == (drawing.Release >= Version::R2013 ? 15 : 0));
      const std::size_t coiled = Marker(twin, *at, "AcDbHelix");
      const HelixData &coil = helix->Data;
      CHECK(coil.Major == ReadWhole(twin, coiled, 90));
      CHECK(coil.Minor == ReadWhole(twin, coiled, 91));
      CHECK(Near(coil.BasePoint, ReadPoint(twin, coiled, 10)));
      CHECK(Near(coil.Start, ReadPoint(twin, coiled, 11)));
      CHECK(Near(coil.Axis, ReadPoint(twin, coiled, 12)));
      CHECK(coil.Radius == doctest::Approx(ReadReal(twin, coiled, 40)));
      CHECK(coil.Turns == doctest::Approx(ReadReal(twin, coiled, 41)));
      CHECK(coil.Rise == doctest::Approx(ReadReal(twin, coiled, 42)));
      CHECK(coil.RightHanded == (ReadWhole(twin, coiled, 290) == 1));
      CHECK(coil.Constraint == ReadWhole(twin, coiled, 280));
    }
    CHECK(seen == 1);
  }
}

TEST_CASE("entities/helix: a helix by fit points round-trips and takes AutoCAD's spline flags in R2013")
{
  std::vector<Issue> issues;
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "HELIX", .ItemClass = 0x1F2}};
  Helix helix;
  helix.Base.Self = Handle{0x40};
  helix.Base.Owner = Handle{0x1F};
  helix.Common.Layer = Handle{0x10};
  helix.Common.Lineweight = Lineweight::ByLayer;
  helix.Curve.FitTolerance = 1e-10;
  helix.Curve.FitPoints = {Vector{}, Vector{.X = 1, .Y = 1, .Z = 1}};
  helix.Data.Radius = 2;
  for (const Version release : {Version::R2004, Version::R2018})
  {
    CAPTURE(NameOf(release));
    const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{
        .Release = release, .Text = &Objects::Latin, .Classes = classes, .Issues = &issues};
    const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{helix}, with), 0, reading);
    REQUIRE(back);
    const Helix &read = Get<Helix>(*back);
    CHECK(read.Curve.FitTolerance == helix.Curve.FitTolerance);
    CHECK(read.Curve.FitPoints == helix.Curve.FitPoints);
    CHECK(read.Data == helix.Data);
  }
  CHECK(issues.empty());
  Drawing drawing;
  drawing.Release = Version::R2004;
  drawing.Objects.emplace_back(helix);
  const Releases::Translation upgraded = Releases::ConvertObject(drawing.Objects.front(), drawing, Version::R2018);
  CHECK(Get<Helix>(upgraded.Object).Curve.Options == 9);
  CHECK(Get<Helix>(upgraded.Object).Curve.Parameter == 0);
  helix.Curve.FitTolerance.reset();
  drawing.Objects.front() = helix;
  const Releases::Translation poles = Releases::ConvertObject(drawing.Objects.front(), drawing, Version::R2013);
  CHECK(Get<Helix>(poles.Object).Curve.Options == 0);
  CHECK(Get<Helix>(poles.Object).Curve.Parameter == 15);
  const Releases::Translation lowered = Releases::ConvertObject(drawing.Objects.front(), drawing, Version::R2000);
  CHECK(Get<Helix>(lowered.Object).Curve == helix.Curve);
}
} // namespace DwgIO::Entities
