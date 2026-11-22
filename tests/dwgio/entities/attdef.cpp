/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/attdef.cpp
 *  @brief     The ATTDEF against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/attdef.hpp"

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
TEST_CASE("entities/attdef: the fixture's attribute definitions match the DXF twin including a constant one")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  for (const std::string_view name : {"entities-r2000.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    std::size_t constant = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *attdef = GetIf<AttDef>(&object);
      if (attdef == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(attdef->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, attdef->Base.Self);
      REQUIRE(opened);
      CHECK(TypeOf(object) == "ATTDEF");
      CheckLettering(twin, *opened, attdef->Lettering);
      CHECK(attdef->Tag == GroupOf(twin, *opened, 2));
      CHECK(attdef->Prompt == GroupOf(twin, *opened, 3));
      CHECK(attdef->Flags == ReadWhole(twin, *opened, 70));
      constant += (attdef->Flags & 2) != 0 ? 1U : 0U;
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, attdef->Common, attdef->Base);
    }
    CHECK(count == 2);
    CHECK(constant == 1);
  }
}
} // namespace DwgIO::Entities
