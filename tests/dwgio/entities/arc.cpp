/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/arc.cpp
 *  @brief     The ARC against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/arc.hpp"

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
TEST_CASE("entities/arc: the fixture's arcs match the DXF twin")
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
      const auto *arc = GetIf<Arc>(&object);
      if (arc == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(arc->Base.Self));
      CHECK(TypeOf(object) == "ARC");
      const std::optional<std::size_t> opened = Entity(twin, arc->Base.Self);
      REQUIRE(opened);
      CHECK(Near(arc->Center, ReadPoint(twin, *opened, 10)));
      CHECK(arc->Radius == doctest::Approx(ReadReal(twin, *opened, 40)));
      CHECK(arc->Thickness == doctest::Approx(ReadReal(twin, *opened, 39)));
      CHECK(Near(arc->Extrusion, Normal(twin, *opened)));
      CHECK(arc->Start == doctest::Approx(ReadReal(twin, *opened, 50) * std::numbers::pi / 180.0));
      CHECK(arc->End == doctest::Approx(ReadReal(twin, *opened, 51) * std::numbers::pi / 180.0));
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, arc->Common, arc->Base);
    }
    CHECK(count == 2);
  }
}
} // namespace DwgIO::Entities
