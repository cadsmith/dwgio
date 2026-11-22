/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/ellipse.cpp
 *  @brief     The ELLIPSE against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/ellipse.hpp"

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
TEST_CASE("entities/ellipse: the fixture's whole ellipse and elliptical arc match the DXF twin")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  for (const std::string_view name : {"entities-r2000.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    std::size_t whole = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *ellipse = GetIf<Ellipse>(&object);
      if (ellipse == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(ellipse->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, ellipse->Base.Self);
      REQUIRE(opened);
      CHECK(TypeOf(object) == "ELLIPSE");
      CHECK(Near(ellipse->Center, ReadPoint(twin, *opened, 10)));
      CHECK(Near(ellipse->Axis, ReadPoint(twin, *opened, 11)));
      CHECK(Near(ellipse->Extrusion, Normal(twin, *opened)));
      CHECK(ellipse->Ratio == doctest::Approx(ReadReal(twin, *opened, 40)));
      CHECK(ellipse->Start == doctest::Approx(ReadReal(twin, *opened, 41)));
      CHECK(ellipse->End == doctest::Approx(ReadReal(twin, *opened, 42)));
      whole += ellipse->End == doctest::Approx(Turn) ? 1U : 0U;
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, ellipse->Common, ellipse->Base);
    }
    CHECK(count == 2);
    CHECK(whole == 1);
  }
}

TEST_CASE("entities/ellipse: a default ellipse is a whole circle")
{
  const Ellipse ellipse;
  CHECK(ellipse.Ratio == 1.0);
  CHECK(ellipse.Start == 0.0);
  CHECK(ellipse.End == doctest::Approx(Turn));
}
} // namespace DwgIO::Entities
