/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/trace.cpp
 *  @brief     The TRACE against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/trace.hpp"

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
TEST_CASE("entities/trace: the fixture's trace matches the DXF twin with its corners in the plane")
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
      const auto *trace = GetIf<Trace>(&object);
      if (trace == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(trace->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, trace->Base.Self);
      REQUIRE(opened);
      CHECK(TypeOf(object) == "TRACE");
      for (std::size_t corner = 0; corner < 4; ++corner)
      {
        CHECK(Near(trace->Corners.at(corner), ReadPoint(twin, *opened, 10 + static_cast<int>(corner))));
      }
      CHECK(trace->Thickness == doctest::Approx(ReadReal(twin, *opened, 39)));
      CHECK(Near(trace->Extrusion, Normal(twin, *opened)));
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, trace->Common, trace->Base);
    }
    CHECK(count == 1);
  }
}
} // namespace DwgIO::Entities
