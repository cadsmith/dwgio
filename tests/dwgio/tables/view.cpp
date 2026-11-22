/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/view.cpp
 *  @brief     The VIEW against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/view.hpp"

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
TEST_CASE("tables/view: the fixture's view matches the DXF twin's camera")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const bool modern = loaded->Drawing.Release >= Version::R2000;
    const View *plan = nullptr;
    for (const Object &object : loaded->Drawing.Objects)
    {
      if (const auto *view = GetIf<View>(&object))
      {
        REQUIRE(plan == nullptr);
        plan = view;
      }
    }
    REQUIRE(plan != nullptr);
    CHECK(TypeOf(Object{*plan}) == "VIEW");
    const std::optional<std::size_t> opened = FindOpeningNoCase(twin, "VIEW", plan->Record.Name);
    REQUIRE(opened);
    CHECK(plan->Record.Base.Self == Self(twin, *opened));
    CHECK(plan->ViewHeight == doctest::Approx(ReadReal(twin, *opened, 40)));
    CHECK(plan->ViewWidth == doctest::Approx(ReadReal(twin, *opened, 41)));
    CHECK(plan->ViewCenter.X == doctest::Approx(ReadReal(twin, *opened, 10)));
    CHECK(plan->ViewCenter.Y == doctest::Approx(ReadReal(twin, *opened, 20)));
    CHECK(Near(plan->Direction, ReadPoint(twin, *opened, 11)));
    CHECK(Near(plan->Target, ReadPoint(twin, *opened, 12)));
    CHECK(plan->Twist == doctest::Approx(ReadReal(twin, *opened, 50) * std::numbers::pi / 180.0));
    CHECK(plan->LensLength == doctest::Approx(ReadReal(twin, *opened, 42)));
    CHECK(plan->Front == doctest::Approx(ReadReal(twin, *opened, 43)));
    CHECK(plan->Back == doctest::Approx(ReadReal(twin, *opened, 44)));
    CHECK(plan->Mode == ReadWhole(twin, *opened, 71));
    CHECK(plan->Paper == ((ReadWhole(twin, *opened, 70) & 1) != 0));
    CHECK(plan->Render == (modern ? ReadWhole(twin, *opened, 281) : 0));
    CHECK(plan->Associated == (modern && ReadWhole(twin, *opened, 72) != 0));
    if (plan->Associated)
    {
      CHECK(Near(plan->Axes.Origin, ReadPoint(twin, *opened, 110)));
      CHECK(Near(plan->Axes.X, ReadPoint(twin, *opened, 111)));
      CHECK(Near(plan->Axes.Y, ReadPoint(twin, *opened, 112)));
      CHECK(plan->Axes.Elevation == doctest::Approx(ReadReal(twin, *opened, 146)));
      CHECK(plan->Axes.Ortho == ReadWhole(twin, *opened, 79));
    }
    CHECK(plan->Axes.Basis == ParseHandle(GroupOf(twin, *opened, 346)).value_or(Handle{}));
    CHECK(plan->Axes.Named == ParseHandle(GroupOf(twin, *opened, 345)).value_or(Handle{}));
    CHECK(plan->Record.Source == Handle{});
  }
}
} // namespace DwgIO::Tables
