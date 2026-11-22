/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/vport.cpp
 *  @brief     The VPORT against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/vport.hpp"

#include "support/fixtures.hpp"
#include "support/twin.hpp"
#include "support/vector.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Tables
{
namespace
{
auto Radians(double degrees) -> double
{
  return degrees * std::numbers::pi / 180.0;
}
} // namespace

TEST_CASE("tables/vport: the fixture's viewports match the DXF twin's views and snaps and grids")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const bool modern = loaded->Drawing.Release >= Version::R2000;
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *vport = GetIf<VPort>(&object);
      if (vport == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(vport->Record.Name);
      CHECK(TypeOf(object) == "VPORT");
      const std::optional<std::size_t> opened = FindOpeningBy(twin, "VPORT", vport->Record.Base.Self);
      REQUIRE(opened);
      CHECK(Near(vport->Lower, ReadPoint(twin, *opened, 10)));
      CHECK(Near(vport->Upper, ReadPoint(twin, *opened, 11)));
      CHECK(Near(vport->ViewCenter, ReadPoint(twin, *opened, 12)));
      CHECK(Near(vport->SnapBase, ReadPoint(twin, *opened, 13)));
      CHECK(Near(vport->Snap, ReadPoint(twin, *opened, 14)));
      CHECK(Near(vport->Grid, ReadPoint(twin, *opened, 15)));
      CHECK(Near(vport->Direction, ReadPoint(twin, *opened, 16)));
      CHECK(Near(vport->Target, ReadPoint(twin, *opened, 17)));
      CHECK(vport->ViewHeight == doctest::Approx(ReadReal(twin, *opened, 40)));
      CHECK(vport->ViewWidth == doctest::Approx(ReadReal(twin, *opened, 41) * ReadReal(twin, *opened, 40)));
      CHECK(vport->LensLength == doctest::Approx(ReadReal(twin, *opened, 42)));
      CHECK(vport->Front == doctest::Approx(ReadReal(twin, *opened, 43)));
      CHECK(vport->Back == doctest::Approx(ReadReal(twin, *opened, 44)));
      CHECK(vport->SnapAngle == doctest::Approx(Radians(ReadReal(twin, *opened, 50))));
      CHECK(vport->Twist == doctest::Approx(Radians(ReadReal(twin, *opened, 51))));
      CHECK(vport->Mode == ReadWhole(twin, *opened, 71));
      CHECK(vport->Zoom == ReadWhole(twin, *opened, 72));
      CHECK(vport->FastZoom == (ReadWhole(twin, *opened, 73) != 0));
      CHECK(vport->UcsIcon == ReadWhole(twin, *opened, 74));
      CHECK(vport->SnapOn == (ReadWhole(twin, *opened, 75) != 0));
      CHECK(vport->GridOn == (ReadWhole(twin, *opened, 76) != 0));
      CHECK(vport->Isometric == (ReadWhole(twin, *opened, 77) != 0));
      CHECK(vport->Isoplane == ReadWhole(twin, *opened, 78));
      CHECK(vport->Record.Source == Handle{});
      if (!modern)
      {
        continue;
      }
      CHECK(vport->Render == ReadWhole(twin, *opened, 281));
      CHECK(vport->UcsPerViewport == (ReadWhole(twin, *opened, 65) != 0));
      CHECK(Near(vport->Axes.Origin, ReadPoint(twin, *opened, 110)));
      CHECK(Near(vport->Axes.X, ReadPoint(twin, *opened, 111)));
      CHECK(Near(vport->Axes.Y, ReadPoint(twin, *opened, 112)));
      CHECK(vport->Axes.Elevation == doctest::Approx(ReadReal(twin, *opened, 146)));
      CHECK(vport->Axes.Ortho == ReadWhole(twin, *opened, 79));
      CHECK(vport->Axes.Named == ParseHandle(GroupOf(twin, *opened, 345)).value_or(Handle{}));
      CHECK(vport->Axes.Basis == ParseHandle(GroupOf(twin, *opened, 346)).value_or(Handle{}));
    }
    CHECK(count == 4);
  }
}
} // namespace DwgIO::Tables
