/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/appid.cpp
 *  @brief     The APPID records against their DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/appid.hpp"

#include "support/fixtures.hpp"
#include "support/twin.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Tables
{
TEST_CASE("tables/appid: the fixture's applications are typed and named like the DXF twin's")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::vector<const AppId *> appids;
    for (const Object &object : loaded->Drawing.Objects)
    {
      if (const auto *appid = GetIf<AppId>(&object))
      {
        appids.push_back(appid);
        CHECK(TypeOf(object) == "APPID");
        CHECK(BaseOf(object).Self == appid->Record.Base.Self);
      }
    }
    REQUIRE(appids.size() == 9);
    for (const AppId *appid : appids)
    {
      CAPTURE(appid->Record.Name);
      // An R14 file keeps a table name in upper case; the twin has the case AutoCAD shows.
      const std::optional<std::size_t> opened = FindOpeningNoCase(twin, "APPID", appid->Record.Name);
      REQUIRE(opened);
      CHECK(appid->Record.Base.Self == Self(twin, *opened));
      CHECK(appid->Record.Base.Owner == ParseHandle(GroupOf(twin, *opened, 330)));
      CHECK(appid->Record.XrefIndex == -1);
      CHECK_FALSE(appid->Record.Dependent);
      CHECK(appid->Record.Source == Handle{});
      CHECK(appid->Unknown1 == 0);
    }
    CHECK(std::ranges::any_of(appids, [](const AppId *one) { return one->Record.Name == "DWGIO"; }));
  }
}
} // namespace DwgIO::Tables
