/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/ucs.cpp
 *  @brief     The UCS against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/ucs.hpp"

#include "support/fixtures.hpp"
#include "support/twin.hpp"

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

namespace DwgIO::Tables
{
TEST_CASE("tables/ucs: the fixture's coordinate system matches the DXF twin's axes")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const bool modern = loaded->Drawing.Release >= Version::R2000;
    const Ucs *shifted = nullptr;
    for (const Object &object : loaded->Drawing.Objects)
    {
      if (const auto *ucs = GetIf<Ucs>(&object))
      {
        REQUIRE(shifted == nullptr);
        shifted = ucs;
      }
    }
    REQUIRE(shifted != nullptr);
    CHECK(TypeOf(Object{*shifted}) == "UCS");
    const std::optional<std::size_t> opened = FindOpeningNoCase(twin, "UCS", shifted->Record.Name);
    REQUIRE(opened);
    CHECK(shifted->Record.Base.Self == Self(twin, *opened));
    CHECK(shifted->Origin == Vector{.X = 10, .Y = 20, .Z = 0});
    CHECK(shifted->X == Vector{.X = 1, .Y = 0, .Z = 0});
    CHECK(shifted->Y == Vector{.X = 0, .Y = 1, .Z = 0});
    CHECK(shifted->Origin.X == doctest::Approx(ReadReal(twin, *opened, 10)));
    CHECK(shifted->Origin.Y == doctest::Approx(ReadReal(twin, *opened, 20)));
    CHECK(shifted->X.X == doctest::Approx(ReadReal(twin, *opened, 11)));
    CHECK(shifted->Y.Y == doctest::Approx(ReadReal(twin, *opened, 22)));
    CHECK(shifted->Elevation == doctest::Approx(modern ? ReadReal(twin, *opened, 146) : 0.0));
    CHECK(shifted->Ortho == (modern ? ReadWhole(twin, *opened, 79) : 0));
    CHECK(shifted->Orthographic == (modern ? ReadWhole(twin, *opened, 71) : 0));
    CHECK(shifted->Basis == ParseHandle(GroupOf(twin, *opened, 346)).value_or(Handle{}));
    CHECK(shifted->Named == Handle{});
    CHECK(shifted->Record.Source == Handle{});
  }
}
} // namespace DwgIO::Tables
