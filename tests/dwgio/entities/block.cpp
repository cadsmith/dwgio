/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/block.cpp
 *  @brief     The BLOCK, ENDBLK and SEQEND entities against their DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/block.hpp"

#include "support/entities.hpp"
#include "support/fixtures.hpp"

#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/seqend.hpp"
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
TEST_CASE("entities/block: every BLOCK and ENDBLK and SEQEND matches the DXF twin")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  for (const std::string_view name : {"entities-r2000.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const bool modern = loaded->Drawing.Release >= Version::R2000;
    std::size_t begins = 0;
    std::size_t ends = 0;
    std::size_t seqends = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      CAPTURE(HexOf(BaseOf(object).Self));
      if (const auto *begin = GetIf<Block>(&object))
      {
        ++begins;
        CHECK(TypeOf(object) == "BLOCK");
        const std::optional<std::size_t> opened = Entity(twin, begin->Base.Self);
        REQUIRE(opened);
        CHECK(twin[*opened].Value == "BLOCK");
        const std::string named = GroupOf(twin, *opened, 2);
        CHECK(begin->Name == (modern ? named : Upper(named)));
        CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, begin->Common, begin->Base);
      }
      else if (const auto *end = GetIf<EndBlk>(&object))
      {
        ++ends;
        CHECK(TypeOf(object) == "ENDBLK");
        const std::optional<std::size_t> opened = Entity(twin, end->Base.Self);
        REQUIRE(opened);
        CHECK(twin[*opened].Value == "ENDBLK");
        CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, end->Common, end->Base);
      }
      else if (const auto *seqend = GetIf<SeqEnd>(&object))
      {
        ++seqends;
        CHECK(TypeOf(object) == "SEQEND");
        const std::optional<std::size_t> opened = Entity(twin, seqend->Base.Self);
        REQUIRE(opened);
        CHECK(twin[*opened].Value == "SEQEND");
        CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, seqend->Common, seqend->Base);
      }
    }
    CHECK(begins == 5);
    CHECK(ends == 5);
    CHECK(seqends == 1);
  }
}
} // namespace DwgIO::Entities
