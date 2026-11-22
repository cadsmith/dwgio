/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/tolerance.cpp
 *  @brief     The TOLERANCE against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/tolerance.hpp"

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
TEST_CASE("entities/tolerance: the fixture's tolerance matches the DXF twin with its frame in AutoCAD's codes")
{
  const std::vector<Pair> twin = ReadTwin("curves-r2000.dxf");
  for (const std::string_view name : {"curves-r2000.dwg", "curves-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *tolerance = GetIf<Tolerance>(&object);
      if (tolerance == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(tolerance->Base.Self));
      CHECK(TypeOf(object) == "TOLERANCE");
      const std::optional<std::size_t> opened = Entity(twin, tolerance->Base.Self);
      REQUIRE(opened);
      const std::size_t at = *opened;
      CHECK(Near(tolerance->Position, ReadPoint(twin, at, 10)));
      CHECK(Near(tolerance->Direction, Along(twin, at, 11)));
      CHECK(Near(tolerance->Normal, Normal(twin, at)));
      CHECK(tolerance->Text == GroupOf(twin, at, 1));
      CHECK(tolerance->Text == "{\\Fgdt;j}%%v{\\Fgdt;n}0.1{\\Fgdt;m}");
      const std::optional<std::size_t> style = FindOpeningNoCase(twin, "DIMSTYLE", GroupOf(twin, at, 3));
      REQUIRE(style);
      CHECK(tolerance->Style == Self(twin, *style));
      CheckCommon(TwinPlace{.Twin = &twin, .At = at, .Drawing = &loaded->Drawing}, tolerance->Common, tolerance->Base);
    }
    CHECK(count == 1);
  }
}
} // namespace DwgIO::Entities
