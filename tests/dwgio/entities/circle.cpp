/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/circle.cpp
 *  @brief     The CIRCLE against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/circle.hpp"

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
TEST_CASE("entities/circle: the fixture's circles match the DXF twin")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  for (const std::string_view name : {"entities-r2000.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    std::size_t thick = 0;
    std::size_t tilted = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *circle = GetIf<Circle>(&object);
      if (circle == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(circle->Base.Self));
      CHECK(TypeOf(object) == "CIRCLE");
      const std::optional<std::size_t> opened = Entity(twin, circle->Base.Self);
      REQUIRE(opened);
      CHECK(Near(circle->Center, ReadPoint(twin, *opened, 10)));
      CHECK(circle->Radius == doctest::Approx(ReadReal(twin, *opened, 40)));
      CHECK(circle->Thickness == doctest::Approx(ReadReal(twin, *opened, 39)));
      CHECK(Near(circle->Extrusion, Normal(twin, *opened)));
      thick += circle->Thickness == 2.0 ? 1U : 0U;
      tilted += circle->Extrusion.Y == doctest::Approx(-1.0) ? 1U : 0U;
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, circle->Common, circle->Base);
    }
    CHECK(count == 4);
    CHECK(thick == 1);
    CHECK(tilted == 1);
  }
}
} // namespace DwgIO::Entities
