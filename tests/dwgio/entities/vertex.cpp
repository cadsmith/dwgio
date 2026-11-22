/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/vertex.cpp
 *  @brief     The VERTEX kinds against their DXF twin, their widths, and the type their flags pick.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/vertex.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/vertex.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
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
#include <map>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  The type a vertex is stored under, from its flags as DXF has them.
 */
auto Kind(int flags) -> std::string_view
{
  const auto bits = static_cast<unsigned>(flags);
  if ((bits & 128U) != 0)
  {
    return (bits & 64U) != 0 ? "VERTEX_PFACE" : "VERTEX_PFACE_FACE";
  }
  return (bits & 64U) != 0 ? "VERTEX_MESH" : (bits & 32U) != 0 ? "VERTEX_3D" : "VERTEX_2D";
}

/**
 *  The width groups of the vertex, or the polyline's defaults where the twin leaves them out.
 */
auto WidthPair(const std::vector<Pair> &twin, std::size_t vertex, std::size_t polyline) -> std::pair<double, double>
{
  const auto width = [&](int code) -> double
  {
    return GroupOf(twin, vertex, code).empty() ? ReadReal(twin, polyline, code) : ReadReal(twin, vertex, code);
  };
  return {width(40), width(41)};
}

auto FirstVertex(const std::vector<Pair> &twin, std::size_t polyline) -> Handle
{
  for (std::size_t at = polyline + 1; at < twin.size(); ++at)
  {
    if (twin[at].Code == 0)
    {
      return Self(twin, at);
    }
  }
  return {};
}

/**
 *  Writes the vertex through its layout as R2000 and reads it back; the bits it took.
 */
auto Written(const Vertex &vertex, Vertex &back) -> std::size_t
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  Bits::Writer data;
  Bits::Writer handles;
  Fields::Output to{.To = &data, .Handles = &handles, .With = with};
  REQUIRE(Fields::Write(Objects::Shape<Vertex>::Layout(), vertex, to));
  Bits::Reader dataIn{data.Bytes()};
  Bits::Reader handlesIn{handles.Bytes()};
  Fields::Input in{.In = &dataIn, .Handles = &handlesIn, .With = with};
  REQUIRE(Fields::Read(Objects::Shape<Vertex>::Layout(), back, in));
  CHECK(issues.empty());
  return data.Position();
}
} // namespace

TEST_CASE("entities/vertex: the fixture's vertices and face records match the DXF twin")
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
      const auto *vertex = GetIf<Vertex>(&object);
      if (vertex == nullptr)
      {
        continue;
      }
      CAPTURE(HexOf(vertex->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, vertex->Base.Self);
      REQUIRE(opened);
      const std::size_t at = *opened;
      CHECK(twin[at].Value == "VERTEX");
      CHECK(vertex->Flags == ReadWhole(twin, at, 70));
      CHECK(TypeOf(object) == Kind(vertex->Flags));
      ++kinds[TypeOf(object)];
      const std::optional<std::size_t> owner = Entity(twin, vertex->Base.Owner);
      REQUIRE(owner);
      if (TypeOf(object) == "VERTEX_PFACE_FACE")
      {
        CHECK(vertex->First == ReadWhole(twin, at, 71));
        CHECK(vertex->Second == ReadWhole(twin, at, 72));
        CHECK(vertex->Third == ReadWhole(twin, at, 73));
        CHECK(vertex->Fourth == ReadWhole(twin, at, 74));
        CHECK(vertex->Position == Vector{});
      }
      else if (TypeOf(object) == "VERTEX_2D")
      {
        CHECK(vertex->Position.X == doctest::Approx(ReadReal(twin, at, 10)));
        CHECK(vertex->Position.Y == doctest::Approx(ReadReal(twin, at, 20)));
        // AutoCAD leaves the elevation in the first vertex's Z of a raised polyline and zero in the others'.
        const bool first = FirstVertex(twin, *owner) == vertex->Base.Self;
        CHECK(vertex->Position.Z == doctest::Approx(first ? ReadReal(twin, *owner, 30) : 0.0));
        const auto [start, end] = WidthPair(twin, at, *owner);
        CHECK(vertex->StartWidth == doctest::Approx(start));
        CHECK(vertex->EndWidth == doctest::Approx(end));
        CHECK(vertex->Bulge == doctest::Approx(ReadReal(twin, at, 42)));
        if (!GroupOf(twin, at, 50).empty())
        {
          CHECK(vertex->Tangent == doctest::Approx(ReadReal(twin, at, 50) * std::numbers::pi / 180.0));
        }
      }
      else
      {
        CHECK(Near(vertex->Position, ReadPoint(twin, at, 10)));
      }
      CheckCommon(TwinPlace{.Twin = &twin, .At = at, .Drawing = &loaded->Drawing}, vertex->Common, vertex->Base);
    }
    CHECK(kinds["VERTEX_2D"] == 27);
    CHECK(kinds["VERTEX_3D"] == 16);
    CHECK(kinds["VERTEX_MESH"] == 64);
    CHECK(kinds["VERTEX_PFACE"] == 5);
    CHECK(kinds["VERTEX_PFACE_FACE"] == 2);
  }
}

TEST_CASE("entities/vertex: equal widths pack into one negative value and others write both")
{
  Vertex equal;
  equal.Position = Vector{.X = 1, .Y = 2, .Z = 0};
  equal.StartWidth = 0.5;
  equal.EndWidth = 0.5;
  Vertex back;
  const std::size_t packed = Written(equal, back);
  CHECK(back == equal);
  Vertex unequal = equal;
  unequal.EndWidth = 0.4;
  const std::size_t spread = Written(unequal, back);
  CHECK(back == unequal);
  CHECK(packed < spread);
  Vertex bare = equal;
  bare.StartWidth = 0;
  bare.EndWidth = 0;
  CHECK(Written(bare, back) < spread);
  CHECK(back == bare);
}

TEST_CASE("entities/vertex: a flags byte that contradicts the type reads and writes back under the type")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Vertex vertex;
  vertex.Base.Self = Handle{0x100};
  vertex.Common.Space = Space::Owner;
  vertex.Base.Owner = Handle{0xFF};
  vertex.Common.Layer = Handle{0x10};
  vertex.Common.Lineweight = Lineweight::ByLayer;
  vertex.Flags = 32;
  vertex.Position = Vector{.X = 1, .Y = 2, .Z = 3};
  std::vector<std::byte> bytes = Objects::WriteObject(Object{vertex}, with);
  std::optional<Object> back = Objects::ReadObject(bytes, 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{vertex});
  // The same bytes with the flags byte cleared: still a VERTEX_3D by its type code.
  Vertex bare = vertex;
  bare.Flags = 0;
  const std::vector<std::byte> stripped = Objects::WriteObject(Object{bare}, with);
  CHECK(stripped != bytes);
  CHECK(TypeOf(Object{bare}) == "VERTEX_2D");
  CHECK(issues.empty());
}

TEST_CASE("entities/vertex: the flags choose the type a vertex is written under")
{
  Vertex vertex;
  CHECK(TypeOf(Object{vertex}) == "VERTEX_2D");
  vertex.Flags = 32;
  CHECK(TypeOf(Object{vertex}) == "VERTEX_3D");
  vertex.Flags = 64;
  CHECK(TypeOf(Object{vertex}) == "VERTEX_MESH");
  vertex.Flags = 192;
  CHECK(TypeOf(Object{vertex}) == "VERTEX_PFACE");
  vertex.Flags = 128;
  CHECK(TypeOf(Object{vertex}) == "VERTEX_PFACE_FACE");
}
} // namespace DwgIO::Entities
