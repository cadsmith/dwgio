/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/insert.cpp
 *  @brief     The INSERT and MINSERT against their DXF twin, written back, and counts refused.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/insert.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/insert.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/issue.hpp"
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
namespace
{
/**
 *  Checks an insert against its twin: the block through the BLOCK_RECORD table, the placing, the grid.
 */
void CheckInsert(const std::vector<Pair> &twin, std::size_t at, const Insert &insert)
{
  const std::optional<std::size_t> record = FindOpeningNoCase(twin, "BLOCK_RECORD", GroupOf(twin, at, 2));
  REQUIRE(record);
  CHECK(insert.Block == Self(twin, *record));
  CHECK(Near(insert.Position, ReadPoint(twin, at, 10)));
  const auto scale = [&](int code) -> double
  {
    return GroupOf(twin, at, code).empty() ? 1.0 : ReadReal(twin, at, code);
  };
  CHECK(Near(insert.Scale, Vector{.X = scale(41), .Y = scale(42), .Z = scale(43)}));
  CHECK(insert.Rotation == doctest::Approx(ReadReal(twin, at, 50) * std::numbers::pi / 180.0));
  CHECK(Near(insert.Extrusion, Normal(twin, at)));
  CHECK(insert.Follows == (ReadWhole(twin, at, 66) == 1));
  if (twin[at].Value == "INSERT" && GroupOf(twin, at, 70).empty())
  {
    CHECK_FALSE(insert.Columns);
    CHECK_FALSE(insert.Rows);
  }
  else
  {
    REQUIRE(insert.Columns);
    REQUIRE(insert.Rows);
    CHECK(*insert.Columns == ReadWhole(twin, at, 70));
    CHECK(*insert.Rows == ReadWhole(twin, at, 71));
    CHECK(insert.ColumnSpacing == doctest::Approx(ReadReal(twin, at, 44)));
    CHECK(insert.RowSpacing == doctest::Approx(ReadReal(twin, at, 45)));
  }
}
} // namespace

TEST_CASE("entities/insert: the fixture's insert owns its attribute and SEQEND and the minsert has its grid")
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
      const auto *insert = GetIf<Insert>(&object);
      if (insert == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(insert->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, insert->Base.Self);
      REQUIRE(opened);
      CheckInsert(twin, *opened, *insert);
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, insert->Common, insert->Base);
      if (insert->Columns)
      {
        CHECK(TypeOf(object) == "MINSERT");
        CHECK(*insert->Columns == 2);
        CHECK(*insert->Rows == 3);
        CHECK(insert->ColumnSpacing == 10.0);
        CHECK(insert->RowSpacing == 15.0);
        CHECK_FALSE(insert->Follows);
      }
      else
      {
        CHECK(TypeOf(object) == "INSERT");
        CHECK(insert->Follows);
        CHECK(insert->First == Handle{0x2F1});
        CHECK(insert->Last == Handle{0x2F1});
        CHECK(insert->SeqEnd == Handle{0x2F0});
      }
    }
    CHECK(count == 2);
  }
}

TEST_CASE("entities/insert: the tables fixture's three inserts have no attributes and no chain")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *insert = GetIf<Insert>(&object);
      if (insert == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(insert->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, insert->Base.Self);
      REQUIRE(opened);
      CheckInsert(twin, *opened, *insert);
      CHECK_FALSE(insert->Follows);
      CHECK(insert->First == Handle{});
      CHECK(insert->Last == Handle{});
      CHECK(insert->SeqEnd == Handle{});
    }
    CHECK(count == 3);
  }
}

TEST_CASE("entities/insert: every scale form and the attribute chain and the grid round-trip as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Insert insert;
  insert.Base.Self = Handle{0x100};
  insert.Common.Space = Space::Model;
  insert.Common.Layer = Handle{0x10};
  insert.Common.Lineweight = Lineweight::ByLayer;
  insert.Position = Vector{.X = 1, .Y = 2, .Z = 3};
  insert.Block = Handle{0x1F};
  for (const Vector scale :
       {Vector{.X = 1, .Y = 1, .Z = 1}, Vector{.X = 2, .Y = 2, .Z = 2}, Vector{.X = 2, .Y = 3, .Z = 4},
        Vector{.X = 1, .Y = 2, .Z = 3}, Vector{.X = 1, .Y = 1, .Z = 3}, Vector{.X = 2, .Y = 2, .Z = 4}})
  {
    CAPTURE(scale.X);
    CAPTURE(scale.Y);
    CAPTURE(scale.Z);
    insert.Scale = scale;
    const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{insert}, with), 0, reading);
    REQUIRE(back);
    CHECK(*back == Object{insert});
  }
  insert.Follows = true;
  insert.First = Handle{0x101};
  insert.Last = Handle{0x102};
  insert.SeqEnd = Handle{0x103};
  std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{insert}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{insert});
  CHECK(TypeOf(*back) == "INSERT");
  insert.Columns = 4;
  insert.Rows = 5;
  insert.ColumnSpacing = 6;
  insert.RowSpacing = 7;
  back = Objects::ReadObject(Objects::WriteObject(Object{insert}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{insert});
  CHECK(TypeOf(*back) == "MINSERT");
  CHECK(issues.empty());
}

namespace DwgIO
{
TEST_CASE("entities/insert: an attribute count the data cannot hold is refused before allocating")
{
  Bits::Writer out;
  out.BL(0x7FFFFFFF);
  Bits::Reader in{out.Bytes()};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2004, .Text = &Objects::Latin, .Issues = &issues};
  const Fields::Input reading{.In = &in, .Handles = &in, .With = with};
  Entities::Insert insert;
  CHECK_FALSE(Entities::Shapes::ReadOwnedCount(insert, reading));
  CHECK(insert.Owned.empty());
}
} // namespace DwgIO
} // namespace DwgIO::Entities
