/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/lwpolyline.cpp
 *  @brief     The LWPOLYLINE against its DXF twin, written back, and its counts cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/lwpolyline.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/lwpolyline.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
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
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  The twin's vertices of a lightweight polyline: each 10 group opens one, the 20, 40, 41 and 42 after it fill it.
 */
auto Vertices(const std::vector<Pair> &twin, std::size_t opened) -> std::vector<LwPolyline::Vertex>
{
  std::vector<LwPolyline::Vertex> segments;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    const int code = twin[at].Code;
    if (code == 10)
    {
      segments.push_back(LwPolyline::Vertex{.Position = Vector{.X = std::stod(twin[at].Value), .Y = 0, .Z = 0}});
      continue;
    }
    if (segments.empty())
    {
      continue;
    }
    LwPolyline::Vertex &last = segments.back();
    if (code == 20)
    {
      last.Position.Y = std::stod(twin[at].Value);
    }
    else if (code == 42)
    {
      last.Bulge = std::stod(twin[at].Value);
    }
    else if (code == 40 || code == 41)
    {
      DwgIO::Widths pair = last.Widths.value_or(DwgIO::Widths{});
      (code == 40 ? pair.Start : pair.End) = std::stod(twin[at].Value);
      last.Widths = pair;
    }
  }
  return segments;
}

/**
 *  Writes the polyline through its layout in that release and reads it back; the bits it took.
 */
auto Written(const LwPolyline &polyline, Version release, LwPolyline &back) -> std::size_t
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues};
  Bits::Writer data;
  Bits::Writer handles;
  Fields::Output to{.To = &data, .Handles = &handles, .With = with};
  REQUIRE(Fields::Write(Objects::Shape<LwPolyline>::Layout(), polyline, to));
  Bits::Reader dataIn{data.Bytes()};
  Bits::Reader handlesIn{handles.Bytes()};
  Fields::Input in{.In = &dataIn, .Handles = &handlesIn, .With = with};
  back = LwPolyline{};
  REQUIRE(Fields::Read(Objects::Shape<LwPolyline>::Layout(), back, in));
  CHECK(issues.empty());
  return data.Position();
}
} // namespace

TEST_CASE("entities/lwpolyline: the fixtures' lightweight polylines match the DXF twin vertex by vertex")
{
  for (const auto &[name, dxf, expected] :
       {std::tuple{std::string_view{"curves-r2000.dwg"}, std::string_view{"curves-r2000.dxf"}, 6UZ},
        std::tuple{std::string_view{"curves-r14.dwg"}, std::string_view{"curves-r2000.dxf"}, 6UZ},
        std::tuple{std::string_view{"foundation-r2000.dwg"}, std::string_view{"foundation-r2000.dxf"}, 1UZ},
        std::tuple{std::string_view{"foundation-r14.dwg"}, std::string_view{"foundation-r2000.dxf"}, 1UZ}})
  {
    CAPTURE(name);
    const std::vector<Pair> twin = ReadTwin(dxf);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *polyline = GetIf<LwPolyline>(&object);
      if (polyline == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(polyline->Base.Self));
      CHECK(TypeOf(object) == "LWPOLYLINE");
      const std::optional<std::size_t> opened = Entity(twin, polyline->Base.Self);
      REQUIRE(opened);
      const std::size_t at = *opened;
      CHECK(polyline->Flags == ReadWhole(twin, at, 70));
      // The twin writes a 43 of zero for a polyline the file keeps no constant width for.
      CHECK(polyline->Width.value_or(0.0) == doctest::Approx(ReadReal(twin, at, 43)));
      CHECK(polyline->Width.has_value() == (ReadReal(twin, at, 43) != 0));
      CHECK(polyline->Elevation == doctest::Approx(ReadReal(twin, at, 38)));
      CHECK(polyline->Thickness == doctest::Approx(ReadReal(twin, at, 39)));
      CHECK(Near(polyline->Extrusion, Normal(twin, at)));
      const std::vector<LwPolyline::Vertex> vertices = Vertices(twin, at);
      CHECK(vertices.size() == static_cast<std::size_t>(ReadWhole(twin, at, 90)));
      REQUIRE(polyline->Vertices.size() == vertices.size());
      for (std::size_t index = 0; index < vertices.size(); ++index)
      {
        CAPTURE(index);
        const LwPolyline::Vertex &segment = polyline->Vertices[index];
        CHECK(Near(segment.Position, vertices[index].Position));
        CHECK(segment.Bulge == doctest::Approx(vertices[index].Bulge));
        CHECK(segment.Widths.has_value() == vertices[index].Widths.has_value());
        if (segment.Widths && vertices[index].Widths)
        {
          CHECK(segment.Widths->Start == doctest::Approx(vertices[index].Widths->Start));
          CHECK(segment.Widths->End == doctest::Approx(vertices[index].Widths->End));
        }
      }
      CheckCommon(TwinPlace{.Twin = &twin, .At = at, .Drawing = &loaded->Drawing}, polyline->Common, polyline->Base);
    }
    CHECK(count == expected);
  }
}

TEST_CASE("entities/lwpolyline: bare points write only the flag and a full polyline writes every list")
{
  LwPolyline bare;
  bare.Vertices = {LwPolyline::Vertex{.Position = Vector{.X = 1, .Y = 2, .Z = 0}},
                   LwPolyline::Vertex{.Position = Vector{.X = 3, .Y = 4, .Z = 0}},
                   LwPolyline::Vertex{.Position = Vector{.X = 5, .Y = 6, .Z = 0}}};
  LwPolyline back;
  const std::size_t least = Written(bare, Version::R2000, back);
  CHECK(back == bare);
  Written(bare, Version::R14, back);
  CHECK(back == bare);
  LwPolyline full = bare;
  full.Flags = 129;
  full.Width = 0.3;
  full.Elevation = 2;
  full.Thickness = 1;
  full.Extrusion = Vector{.X = 0, .Y = -1, .Z = 0};
  full.Vertices[1].Bulge = 0.5;
  for (LwPolyline::Vertex &segment : full.Vertices)
  {
    segment.Widths = DwgIO::Widths{};
  }
  full.Vertices[2].Widths = DwgIO::Widths{.Start = 0.2, .End = 0.4};
  const std::size_t most = Written(full, Version::R2000, back);
  CHECK(back == full);
  CHECK(most > least);
  Written(full, Version::R14, back);
  CHECK(back == full);
  // A pair on one vertex alone comes back on every vertex, since the file keeps all or none.
  LwPolyline mixed = bare;
  mixed.Vertices[2].Widths = DwgIO::Widths{.Start = 0.2, .End = 0.4};
  Written(mixed, Version::R2000, back);
  CHECK(back != mixed);
  REQUIRE(back.Vertices[0].Widths);
  CHECK(*back.Vertices[0].Widths == DwgIO::Widths{});
  CHECK(back.Vertices[2].Widths == mixed.Vertices[2].Widths);
}

namespace
{
/**
 *  Reads a lightweight polyline from those bits as R2000.
 */
auto ReadBits(const Bits::Writer &data, LwPolyline &polyline) -> bool
{
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin};
  Bits::Reader dataIn{data.Bytes()};
  Fields::Input in{.In = &dataIn, .With = with};
  return Fields::Read(Objects::Shape<LwPolyline>::Layout(), polyline, in);
}
} // namespace

TEST_CASE("entities/lwpolyline: a negative or oversized count or an already failed reader stops the read")
{
  LwPolyline polyline;
  Bits::Writer many;
  many.BS(0);
  many.BL(1000);
  CHECK_FALSE(ReadBits(many, polyline));
  CHECK(polyline.Vertices.empty());
  Bits::Writer negative;
  negative.BS(0);
  negative.BL(-1);
  CHECK_FALSE(ReadBits(negative, polyline));
  Bits::Writer cut;
  cut.BS(0);
  CHECK_FALSE(ReadBits(cut, polyline));
  Bits::Writer bulges;
  bulges.BS(16);
  bulges.BL(1);
  bulges.BL(1000);
  CHECK_FALSE(ReadBits(bulges, polyline));
  Bits::Writer widths;
  widths.BS(32);
  widths.BL(1);
  widths.BL(1000);
  CHECK_FALSE(ReadBits(widths, polyline));
}

TEST_CASE("entities/lwpolyline: bulges and width pairs beyond the points are read and dropped")
{
  Bits::Writer data;
  data.BS(16 | 32);
  data.BL(1);
  data.BL(2);
  data.BL(2);
  data.RD(1);
  data.RD(2);
  data.BD(0.5);
  data.BD(0.25);
  data.BD(1);
  data.BD(2);
  data.BD(3);
  data.BD(4);
  LwPolyline polyline;
  REQUIRE(ReadBits(data, polyline));
  REQUIRE(polyline.Vertices.size() == 1);
  CHECK(polyline.Vertices.front().Bulge == 0.5);
  CHECK(polyline.Vertices.front().Widths == DwgIO::Widths{.Start = 1, .End = 2});
}
} // namespace DwgIO::Entities
