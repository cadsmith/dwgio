/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/polyline.cpp
 *  @brief     The old-style polylines against their DXF twin, and the type their flags pick.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/polyline.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/polyline.hpp"
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
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  The type a polyline is stored under, from its flags as DXF has them.
 */
auto Kind(int flags) -> std::string_view
{
  const auto bits = static_cast<unsigned>(flags);
  return (bits & 64U) != 0   ? "POLYLINE_PFACE"
         : (bits & 16U) != 0 ? "POLYLINE_MESH"
         : (bits & 8U) != 0  ? "POLYLINE_3D"
                             : "POLYLINE_2D";
}

/**
 *  The vertices the twin lists after the polyline, and the SEQEND that closes them.
 */
struct Chain
{
  std::vector<Handle> Vertices;
  Handle SeqEnd;
};

auto ReadChain(const std::vector<Pair> &twin, std::size_t opened) -> Chain
{
  Chain chain;
  for (std::size_t at = opened + 1; at < twin.size(); ++at)
  {
    if (twin[at].Code != 0)
    {
      continue;
    }
    if (twin[at].Value == "SEQEND")
    {
      chain.SeqEnd = Self(twin, at);
      break;
    }
    chain.Vertices.push_back(Self(twin, at));
  }
  return chain;
}
} // namespace

TEST_CASE("entities/polyline: the fixture's polylines match the DXF twin and own their vertices and SEQEND")
{
  const std::vector<Pair> twin = ReadTwin("curves-r2000.dxf");
  for (const std::string_view name : {"curves-r2000.dwg", "curves-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::map<std::string_view, int> kinds;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *polyline = GetIf<Polyline>(&object);
      if (polyline == nullptr)
      {
        continue;
      }
      CAPTURE(HexOf(polyline->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, polyline->Base.Self);
      REQUIRE(opened);
      const std::size_t at = *opened;
      CHECK(twin[at].Value == "POLYLINE");
      CHECK(polyline->Flags == ReadWhole(twin, at, 70));
      CHECK(TypeOf(object) == Kind(polyline->Flags));
      ++kinds[TypeOf(object)];
      CHECK(polyline->Curve == ReadWhole(twin, at, 75));
      CHECK(polyline->Rows == ReadWhole(twin, at, 71));
      CHECK(polyline->Columns == ReadWhole(twin, at, 72));
      CHECK(polyline->Density == ReadWhole(twin, at, 73));
      CHECK(polyline->Depth == ReadWhole(twin, at, 74));
      CHECK(polyline->StartWidth == doctest::Approx(ReadReal(twin, at, 40)));
      CHECK(polyline->EndWidth == doctest::Approx(ReadReal(twin, at, 41)));
      CHECK(polyline->Thickness == doctest::Approx(ReadReal(twin, at, 39)));
      CHECK(polyline->Elevation == doctest::Approx(ReadReal(twin, at, 30)));
      CHECK(Near(polyline->Extrusion, Normal(twin, at)));
      const Chain chain = ReadChain(twin, at);
      REQUIRE_FALSE(chain.Vertices.empty());
      CHECK(polyline->First == chain.Vertices.front());
      CHECK(polyline->Last == chain.Vertices.back());
      CHECK(polyline->SeqEnd == chain.SeqEnd);
      CheckCommon(TwinPlace{.Twin = &twin, .At = at, .Drawing = &loaded->Drawing}, polyline->Common, polyline->Base);
    }
    CHECK(kinds["POLYLINE_2D"] == 4);
    CHECK(kinds["POLYLINE_3D"] == 2);
    CHECK(kinds["POLYLINE_MESH"] == 2);
    CHECK(kinds["POLYLINE_PFACE"] == 1);
  }
}

TEST_CASE("entities/polyline: a quadratic spline-fit 3D polyline round-trips as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Polyline polyline;
  polyline.Base.Self = Handle{0x100};
  polyline.Common.Space = Space::Model;
  polyline.Common.Layer = Handle{0x10};
  polyline.Common.Lineweight = Lineweight::ByLayer;
  polyline.Flags = 8 | 4 | 1;
  polyline.Curve = 5;
  polyline.First = Handle{0x101};
  polyline.Last = Handle{0x104};
  polyline.SeqEnd = Handle{0x105};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{polyline}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{polyline});
  CHECK(TypeOf(*back) == "POLYLINE_3D");
  CHECK(issues.empty());
}

TEST_CASE("entities/polyline: a flags short carrying another type's bit reads as the type says")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Polyline mesh;
  mesh.Base.Self = Handle{0x100};
  mesh.Common.Space = Space::Model;
  mesh.Common.Layer = Handle{0x10};
  mesh.Common.Lineweight = Lineweight::ByLayer;
  mesh.Flags = 16 | 1;
  mesh.Rows = 2;
  mesh.Columns = 3;
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{mesh}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{mesh});
  CHECK(TypeOf(*back) == "POLYLINE_MESH");
  CHECK(issues.empty());
}

TEST_CASE("entities/polyline: the flags choose the type a polyline is written under")
{
  Polyline polyline;
  CHECK(TypeOf(Object{polyline}) == "POLYLINE_2D");
  polyline.Flags = 9;
  CHECK(TypeOf(Object{polyline}) == "POLYLINE_3D");
  polyline.Flags = 16;
  CHECK(TypeOf(Object{polyline}) == "POLYLINE_MESH");
  polyline.Flags = 64;
  CHECK(TypeOf(Object{polyline}) == "POLYLINE_PFACE");
}

namespace DwgIO
{
TEST_CASE("entities/polyline: a vertex count the data cannot hold is refused before allocating")
{
  Bits::Writer out;
  out.BL(0x7FFFFFFF);
  Bits::Reader in{out.Bytes()};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2004, .Text = &Objects::Latin, .Issues = &issues};
  const Fields::Input reading{.In = &in, .Handles = &in, .With = with};
  Entities::Polyline polyline;
  CHECK_FALSE(Entities::Shapes::ReadVertexCount(polyline, reading));
  CHECK(polyline.Vertices.empty());
}
} // namespace DwgIO
} // namespace DwgIO::Entities
