/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/xline.cpp
 *  @brief     The XLINE against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/xline.hpp"

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
TEST_CASE("entities/xline: the fixture's xline matches the DXF twin")
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
      const auto *xline = GetIf<XLine>(&object);
      if (xline == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(xline->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, xline->Base.Self);
      REQUIRE(opened);
      CHECK(TypeOf(object) == "XLINE");
      CHECK(Near(xline->Start, ReadPoint(twin, *opened, 10)));
      CHECK(Near(xline->Direction, ReadPoint(twin, *opened, 11)));
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, xline->Common, xline->Base);
    }
    CHECK(count == 1);
  }
}
} // namespace DwgIO::Entities
