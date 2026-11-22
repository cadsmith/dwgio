/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/shape.cpp
 *  @brief     The SHAPE against its DXF twin, and the style of its shape file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/shape.hpp"

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
TEST_CASE("entities/shape: the fixture's shape matches the DXF twin and points at the shape file's style")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  // The shape's style is the one over the shape file, which the twin names only by that file.
  Handle style;
  for (std::size_t at = 0; at < twin.size(); ++at)
  {
    if (twin[at].Code == 0 && twin[at].Value == "STYLE" && GroupOf(twin, at, 3) == "ltypeshp.shx")
    {
      style = Self(twin, at);
    }
  }
  REQUIRE(style);
  for (const std::string_view name : {"entities-r2000.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *shape = GetIf<Shape>(&object);
      if (shape == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(shape->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, shape->Base.Self);
      REQUIRE(opened);
      CHECK(TypeOf(object) == "SHAPE");
      CHECK(Near(shape->Position, ReadPoint(twin, *opened, 10)));
      CHECK(shape->Size == doctest::Approx(ReadReal(twin, *opened, 40)));
      CHECK(shape->Rotation == doctest::Approx(ReadReal(twin, *opened, 50) * std::numbers::pi / 180.0));
      CHECK(shape->Width == doctest::Approx(GroupOf(twin, *opened, 41).empty() ? 1.0 : ReadReal(twin, *opened, 41)));
      CHECK(shape->Oblique == doctest::Approx(ReadReal(twin, *opened, 51) * std::numbers::pi / 180.0));
      CHECK(shape->Thickness == doctest::Approx(ReadReal(twin, *opened, 39)));
      CHECK(shape->Number > 0);
      CHECK(Near(shape->Extrusion, Normal(twin, *opened)));
      CHECK(shape->Style == style);
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, shape->Common, shape->Base);
    }
    CHECK(count == 1);
  }
}
} // namespace DwgIO::Entities
