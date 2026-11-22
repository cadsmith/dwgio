/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/leader.cpp
 *  @brief     The LEADER against its DXF twin, and written back as R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/leader.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
TEST_CASE("entities/leader: the fixture's four leaders match the DXF twin with their annotations and paths")
{
  const std::vector<Pair> twin = ReadTwin("curves-r2000.dxf");
  for (const std::string_view name : {"curves-r2000.dwg", "curves-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::vector<int> annotations;
    std::vector<int> paths;
    std::size_t arrowless = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *leader = GetIf<Leader>(&object);
      if (leader == nullptr)
      {
        continue;
      }
      CAPTURE(HexOf(leader->Base.Self));
      CHECK(TypeOf(object) == "LEADER");
      const std::optional<std::size_t> opened = Entity(twin, leader->Base.Self);
      REQUIRE(opened);
      const std::size_t at = *opened;
      CHECK(leader->Arrow == (GroupOf(twin, at, 71).empty() || ReadWhole(twin, at, 71) == 1));
      CHECK(leader->Path == ReadWhole(twin, at, 72));
      CHECK(leader->Annotation == (GroupOf(twin, at, 73).empty() ? 3 : ReadWhole(twin, at, 73)));
      CHECK(leader->Direction == (GroupOf(twin, at, 74).empty() || ReadWhole(twin, at, 74) == 1));
      REQUIRE(leader->Points.size() == static_cast<std::size_t>(ReadWhole(twin, at, 76)));
      const std::vector<std::string> xs = Every(twin, at, 10);
      const std::vector<std::string> ys = Every(twin, at, 20);
      const std::vector<std::string> zs = Every(twin, at, 30);
      REQUIRE(xs.size() == leader->Points.size());
      for (std::size_t index = 0; index < xs.size(); ++index)
      {
        CHECK(Near(leader->Points[index],
                   Vector{.X = std::stod(xs[index]), .Y = std::stod(ys[index]), .Z = std::stod(zs[index])}));
      }
      CHECK(leader->Height == doctest::Approx(ReadReal(twin, at, 40)));
      CHECK(leader->Width == doctest::Approx(ReadReal(twin, at, 41)));
      CHECK(Near(leader->Normal, Normal(twin, at)));
      CHECK(Near(leader->Horizontal, Along(twin, at, 211)));
      CHECK(Near(leader->Block, ReadPoint(twin, at, 212)));
      CHECK(Near(leader->Offset, ReadPoint(twin, at, 213)));
      CHECK(leader->Note == ParseHandle(GroupOf(twin, at, 340)).value_or(Handle{}));
      CHECK((leader->Annotation == 3) == (leader->Note == Handle{}));
      const std::optional<std::size_t> style = FindOpeningNoCase(twin, "DIMSTYLE", GroupOf(twin, at, 3));
      REQUIRE(style);
      CHECK(leader->Style == Self(twin, *style));
      CHECK_FALSE(leader->Unknown1);
      annotations.push_back(leader->Annotation);
      paths.push_back(leader->Path);
      arrowless += leader->Arrow ? 0U : 1U;
      CheckCommon(TwinPlace{.Twin = &twin, .At = at, .Drawing = &loaded->Drawing}, leader->Common, leader->Base);
    }
    CHECK(annotations == std::vector{0, 3, 3, 2});
    CHECK(paths == std::vector{0, 1, 0, 0});
    CHECK(arrowless == 1);
  }
}

TEST_CASE("entities/leader: a leader with every value set round-trips as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Leader leader;
  leader.Base.Self = Handle{0x100};
  leader.Common.Space = Space::Model;
  leader.Common.Layer = Handle{0x10};
  leader.Common.Lineweight = Lineweight::ByLayer;
  leader.Annotation = 0;
  leader.Path = 1;
  leader.Points = {Vector{.X = 0, .Y = 0, .Z = 0}, Vector{.X = 5, .Y = 5, .Z = 0}, Vector{.X = 9, .Y = 5, .Z = 0}};
  leader.Origin = leader.Points.front();
  leader.Block = Vector{.X = 1, .Y = 2, .Z = 3};
  leader.Offset = Vector{.X = 4, .Y = 5, .Z = 6};
  leader.Height = 2.5;
  leader.Width = 10;
  leader.Direction = false;
  leader.Arrow = false;
  leader.ArrowheadType = 1;
  leader.Unknown8 = true;
  leader.Note = Handle{0x200};
  leader.Style = Handle{0x27};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{leader}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{leader});
  CHECK(issues.empty());
}
} // namespace DwgIO::Entities
