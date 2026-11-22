/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/attrib.cpp
 *  @brief     The ATTRIB against its DXF twin, and a multiline one kept as bytes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/attrib.hpp"

#include "support/entities.hpp"
#include "support/fixtures.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/unknown.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
TEST_CASE("entities/attrib: the fixture's attribute matches the DXF twin and belongs to its insert")
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
      const auto *attrib = GetIf<Attrib>(&object);
      if (attrib == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(attrib->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, attrib->Base.Self);
      REQUIRE(opened);
      CHECK(TypeOf(object) == "ATTRIB");
      CheckLettering(twin, *opened, attrib->Lettering);
      CHECK(attrib->Tag == GroupOf(twin, *opened, 2));
      CHECK(attrib->Lettering.Value == "Alice");
      CHECK(attrib->Flags == ReadWhole(twin, *opened, 70));
      const auto owner = std::ranges::find(loaded->Drawing.Objects, attrib->Base.Owner,
                                           [](const Object &one) { return BaseOf(one).Self; });
      REQUIRE(owner != loaded->Drawing.Objects.end());
      CHECK(TypeOf(*owner) == "INSERT");
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, attrib->Common, attrib->Base);
    }
    CHECK(count == 1);
  }
}

TEST_CASE("entities/attrib: an R2018 multiline attribute loads as bytes and saves back unchanged")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("entities-r2018.dwg"));
  REQUIRE(loaded);
  Drawing drawing = loaded->Drawing;
  Handle self{};
  for (Object &object : drawing.Objects)
  {
    if (auto *attrib = GetIf<Attrib>(&object))
    {
      attrib->Type = 2; // A multiline attribute, whose embedded text dwgio does not read.
      self = attrib->Base.Self;
    }
  }
  REQUIRE(self != Handle{});
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  REQUIRE(back->Issues.size() == 1);
  CHECK(back->Issues.front().What == "multiline attribute of type 2 not read");
  const Object *kept = Find(back->Drawing, self);
  REQUIRE(kept != nullptr);
  CHECK(Holds<Unknown>(*kept));
  const std::expected<Saved, Failure> again = Save(back->Drawing);
  REQUIRE(again);
  CHECK(again->Bytes == saved->Bytes);
}
} // namespace DwgIO::Entities
