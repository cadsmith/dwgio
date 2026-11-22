/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/point.cpp
 *  @brief     The POINT against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/point.hpp"

#include "support/entities.hpp"
#include "support/fixtures.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <numbers>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
TEST_CASE("entities/point: the fixture's points match the DXF twin")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  for (const std::string_view name : {"entities-r2000.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *point = GetIf<Point>(&object);
      if (point == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(point->Base.Self));
      CHECK(TypeOf(object) == "POINT");
      const std::optional<std::size_t> opened = Entity(twin, point->Base.Self);
      REQUIRE(opened);
      CHECK(Near(point->Position, ReadPoint(twin, *opened, 10)));
      CHECK(point->Thickness == doctest::Approx(ReadReal(twin, *opened, 39)));
      CHECK(Near(point->Extrusion, Normal(twin, *opened)));
      CHECK(point->Angle == doctest::Approx(ReadReal(twin, *opened, 50) * std::numbers::pi / 180.0));
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, point->Common, point->Base);
    }
    CHECK(count == 1);
  }
}
} // namespace DwgIO::Entities
