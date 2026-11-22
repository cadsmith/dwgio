/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/solid.cpp
 *  @brief     The SOLID against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/solid.hpp"

#include "support/entities.hpp"
#include "support/fixtures.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
TEST_CASE("entities/solid: the fixture's solid matches the DXF twin with its corners in the plane")
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
      const auto *solid = GetIf<Solid>(&object);
      if (solid == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(solid->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, solid->Base.Self);
      REQUIRE(opened);
      CHECK(TypeOf(object) == "SOLID");
      for (std::size_t corner = 0; corner < 4; ++corner)
      {
        CHECK(Near(solid->Corners.at(corner), ReadPoint(twin, *opened, 10 + static_cast<int>(corner))));
      }
      CHECK(solid->Thickness == doctest::Approx(ReadReal(twin, *opened, 39)));
      CHECK(Near(solid->Extrusion, Normal(twin, *opened)));
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, solid->Common, solid->Base);
    }
    CHECK(count == 1);
  }
}
} // namespace DwgIO::Entities
