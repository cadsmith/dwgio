/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/hatch.cpp
 *  @brief     The HATCH against its DXF twin, written back, and its counts cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/hatch.hpp"

#include "entities/shapes/hatch.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/cursor.hpp"
#include "support/cut.hpp"
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
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
constexpr double Degree = std::numbers::pi / 180.0;

void CheckEdge(Cursor &cursor, const Edges::Edge &edge)
{
  const int type = cursor.Whole(72);
  CHECK(edge.index() + 1 == static_cast<std::size_t>(type));
  if (const auto *line = std::get_if<Edges::Line>(&edge))
  {
    CHECK(Near(line->Start, cursor.Point(10)));
    CHECK(Near(line->End, cursor.Point(11)));
  }
  else if (const auto *arc = std::get_if<Edges::Arc>(&edge))
  {
    CHECK(Near(arc->Center, cursor.Point(10)));
    CHECK(arc->Radius == doctest::Approx(cursor.Real(40)));
    CHECK(arc->Start == doctest::Approx(cursor.Real(50) * Degree));
    CHECK(arc->End == doctest::Approx(cursor.Real(51) * Degree));
    CHECK(arc->CounterClockwise == (cursor.Whole(73) == 1));
  }
  else if (const auto *ellipse = std::get_if<Edges::Ellipse>(&edge))
  {
    CHECK(Near(ellipse->Center, cursor.Point(10)));
    CHECK(Near(ellipse->Major, cursor.Point(11)));
    CHECK(ellipse->Ratio == doctest::Approx(cursor.Real(40)));
    CHECK(ellipse->Start == doctest::Approx(cursor.Real(50) * Degree));
    CHECK(ellipse->End == doctest::Approx(cursor.Real(51) * Degree));
    CHECK(ellipse->CounterClockwise == (cursor.Whole(73) == 1));
  }
  else if (const auto *spline = std::get_if<Edges::Spline>(&edge))
  {
    CHECK(spline->Degree == cursor.Whole(94));
    CHECK(spline->Rational == (cursor.Whole(73) == 1));
    CHECK(spline->Periodic == (cursor.Whole(74) == 1));
    REQUIRE(spline->Knots.size() == static_cast<std::size_t>(cursor.Whole(95)));
    REQUIRE(spline->ControlPoints.size() == static_cast<std::size_t>(cursor.Whole(96)));
    for (const double knot : spline->Knots)
    {
      CHECK(knot == doctest::Approx(cursor.Real(40)));
    }
    for (const Edges::Node &pole : spline->ControlPoints)
    {
      CHECK(Near(pole.Position, cursor.Point(10)));
      if (spline->Rational)
      {
        CHECK(pole.Weight == doctest::Approx(cursor.Real(42)));
      }
    }
    // A 97 followed by fit points belongs to the spline; the path's own 97 counts its sources.
    if (cursor.Code() == 97)
    {
      Cursor ahead = cursor;
      const int fits = ahead.Whole(97);
      if (ahead.Code() == 11)
      {
        cursor = ahead;
        for (int at = 0; at < fits; ++at)
        {
          cursor.Point(11);
        }
        if (cursor.Code() == 12)
        {
          cursor.Point(12);
          cursor.Point(13);
        }
      }
    }
  }
}

void CheckPath(Cursor &cursor, const Path &path)
{
  CHECK(path.Flags == cursor.Whole(92));
  if ((static_cast<unsigned>(path.Flags) & 2U) != 0)
  {
    CHECK(path.Bulged == (cursor.Whole(72) == 1));
    CHECK(path.Closed == (cursor.Whole(73) == 1));
    REQUIRE(path.Vertices.size() == static_cast<std::size_t>(cursor.Whole(93)));
    CHECK(path.Edges.empty());
    for (const Edges::Node &vertex : path.Vertices)
    {
      CHECK(Near(vertex.Position, cursor.Point(10)));
      if (path.Bulged)
      {
        CHECK(vertex.Weight == doctest::Approx(cursor.Real(42)));
      }
    }
  }
  else
  {
    REQUIRE(path.Edges.size() == static_cast<std::size_t>(cursor.Whole(93)));
    CHECK(path.Vertices.empty());
    for (const Edges::Edge &edge : path.Edges)
    {
      CheckEdge(cursor, edge);
    }
  }
  REQUIRE(path.Sources.size() == static_cast<std::size_t>(cursor.Whole(97)));
  for (const Handle source : path.Sources)
  {
    CHECK(source == ParseHandle(cursor.Take(330)));
  }
}

void CheckHatch(const std::vector<Pair> &twin, std::size_t opened, const Hatch &hatch)
{
  Cursor cursor{twin, opened};
  cursor.Until(10);
  CHECK(cursor.Real(10) == 0.0);
  CHECK(cursor.Real(20) == 0.0);
  CHECK(hatch.Elevation == doctest::Approx(cursor.Real(30)));
  Vector normal;
  normal.X = cursor.Real(210);
  normal.Y = cursor.Real(220);
  normal.Z = cursor.Real(230);
  CHECK(Near(hatch.Normal, normal));
  CHECK(hatch.Name == cursor.Take(2));
  CHECK(hatch.Solid == (cursor.Whole(70) == 1));
  CHECK(hatch.Associative == (cursor.Whole(71) == 1));
  REQUIRE(hatch.Paths.size() == static_cast<std::size_t>(cursor.Whole(91)));
  for (const Path &path : hatch.Paths)
  {
    CheckPath(cursor, path);
  }
  CHECK(hatch.Style == cursor.Whole(75));
  CHECK(hatch.Kind == cursor.Whole(76));
  if (!hatch.Solid)
  {
    CHECK(hatch.Angle == doctest::Approx(cursor.Real(52) * Degree));
    CHECK(hatch.Scale == doctest::Approx(cursor.Real(41)));
    CHECK(hatch.Doubled == (cursor.Whole(77) == 1));
    REQUIRE(hatch.PatternLines.size() == static_cast<std::size_t>(cursor.Whole(78)));
    for (const PatternLine &stripe : hatch.PatternLines)
    {
      CHECK(stripe.Angle == doctest::Approx(cursor.Real(53) * Degree));
      CHECK(stripe.X == doctest::Approx(cursor.Real(43)));
      CHECK(stripe.Y == doctest::Approx(cursor.Real(44)));
      CHECK(stripe.Run == doctest::Approx(cursor.Real(45)));
      CHECK(stripe.Rise == doctest::Approx(cursor.Real(46)));
      REQUIRE(stripe.Dashes.size() == static_cast<std::size_t>(cursor.Whole(79)));
      for (const double dash : stripe.Dashes)
      {
        CHECK(dash == doctest::Approx(cursor.Real(49)));
      }
    }
  }
  else
  {
    CHECK(hatch.PatternLines.empty());
  }
  if (cursor.Code() == 47)
  {
    CHECK(hatch.Pixel == doctest::Approx(cursor.Real(47)));
  }
  REQUIRE(hatch.Seeds.size() == static_cast<std::size_t>(cursor.Whole(98)));
  for (const Vector &seed : hatch.Seeds)
  {
    CHECK(Near(seed, cursor.Point(10)));
  }
}

/**
 *  What the fixture's five hatches hold, each told apart by its boundary.
 *
 *  AutoCAD stores every boundary of the fixture as edges, a polyline's among them, so no path has the polyline flag.
 */
struct Tally
{
  int Polylines{};
  int Lines{};
  int Arcs{};
  int Ellipses{};
  int Splines{};
  int Islands{};
};

auto Add(const Hatch &hatch, Tally &seen) -> void
{
  for (const Path &path : hatch.Paths)
  {
    seen.Polylines += path.Vertices.empty() ? 0 : 1;
    for (const Edges::Edge &edge : path.Edges)
    {
      seen.Lines += std::holds_alternative<Edges::Line>(edge) ? 1 : 0;
      seen.Arcs += std::holds_alternative<Edges::Arc>(edge) ? 1 : 0;
      seen.Ellipses += std::holds_alternative<Edges::Ellipse>(edge) ? 1 : 0;
      if (std::holds_alternative<Edges::Spline>(edge))
      {
        ++seen.Splines;
        CHECK_FALSE(hatch.Associative);
        CHECK(path.Sources.empty());
      }
    }
  }
  if (hatch.Paths.size() == 2)
  {
    ++seen.Islands;
    CHECK(hatch.Style == 1);
    CHECK(hatch.PatternLines.size() == 2);
  }
}
} // namespace

TEST_CASE("entities/hatch: the fixture's hatches match the DXF twin path by path and edge by edge")
{
  const std::vector<Pair> twin = ReadTwin("curves-r2000.dxf");
  for (const std::string_view name : {"curves-r2000.dwg", "curves-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    Tally seen;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *hatch = GetIf<Hatch>(&object);
      if (hatch == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(hatch->Base.Self));
      CHECK(TypeOf(object) == "HATCH");
      const std::optional<std::size_t> opened = Entity(twin, hatch->Base.Self);
      REQUIRE(opened);
      CheckHatch(twin, *opened, *hatch);
      Add(*hatch, seen);
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, hatch->Common, hatch->Base);
    }
    CHECK(count == 5);
    CHECK(seen.Polylines == 0);
    CHECK(seen.Lines == 10);
    CHECK(seen.Arcs == 2);
    CHECK(seen.Ellipses == 1);
    CHECK(seen.Splines == 1);
    CHECK(seen.Islands == 1);
  }
}

TEST_CASE("entities/hatch: a derived boundary and a polyline path and a solid fill round-trip as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Hatch hatch;
  hatch.Base.Self = Handle{0x100};
  hatch.Common.Space = Space::Model;
  hatch.Common.Layer = Handle{0x10};
  hatch.Common.Lineweight = Lineweight::ByLayer;
  hatch.Name = "SOLID";
  hatch.Solid = true;
  Path derived;
  derived.Flags = 1 | 4;
  derived.Edges = {
      Edges::Line{.Start = Vector{.X = 0, .Y = 0, .Z = 0}, .End = Vector{.X = 1, .Y = 0, .Z = 0}},
      Edges::Arc{.Center = Vector{.X = 1, .Y = 1, .Z = 0}, .Radius = 1, .Start = 0, .End = 1, .CounterClockwise = true},
      Edges::Spline{.Degree = 2,
                    .Rational = true,
                    .Periodic = false,
                    .Knots = {0, 0, 0, 1, 1, 1},
                    .ControlPoints = {Edges::Node{.Position = Vector{.X = 1, .Y = 2, .Z = 0}, .Weight = 1},
                                      Edges::Node{.Position = Vector{.X = 0, .Y = 2, .Z = 0}, .Weight = 2},
                                      Edges::Node{.Position = Vector{.X = 0, .Y = 0, .Z = 0}, .Weight = 1}}}};
  derived.Sources = {Handle{0x20}, Handle{0x21}};
  Path polyline;
  polyline.Flags = 2 | 16;
  polyline.Bulged = true;
  polyline.Closed = true;
  polyline.Vertices = {Edges::Node{.Position = Vector{.X = 0, .Y = 0, .Z = 0}, .Weight = 0},
                       Edges::Node{.Position = Vector{.X = 1, .Y = 0, .Z = 0}, .Weight = 1},
                       Edges::Node{.Position = Vector{.X = 1, .Y = 1, .Z = 0}, .Weight = 0}};
  Path straight = polyline;
  straight.Bulged = false;
  straight.Vertices[1].Weight = 0;
  hatch.Paths = {derived, polyline, straight};
  hatch.Pixel = 0.5;
  hatch.Seeds = {Vector{.X = 0.5, .Y = 0.5, .Z = 0}};
  std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{hatch}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{hatch});
  CHECK(issues.empty());
}

namespace
{
/**
 *  The bits of a hatch up to its path count, solid or patterned.
 */
void Head(Bits::Writer &data, bool solid)
{
  data.BD(0);
  data.Point3(Up);
  data.T("SOLID", Objects::Latin);
  data.B(solid);
  data.B(false);
}

/**
 *  Reads a hatch from those bits as R2000, noting into the issues.
 */
auto ReadBits(const Bits::Writer &data, Hatch &hatch, std::vector<Issue> &issues) -> bool
{
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  Bits::Reader dataIn{data.Bytes()};
  Fields::Input in{.In = &dataIn, .With = with};
  return Fields::Read(Objects::Shape<Hatch>::Layout(), hatch, in);
}
} // namespace

TEST_CASE("entities/hatch: a count the data cannot hold in any of the lists stops the read")
{
  std::vector<Issue> issues;
  Hatch hatch;
  const std::int32_t many = 100000;
  Bits::Writer paths;
  Head(paths, true);
  paths.BL(many);
  CHECK_FALSE(ReadBits(paths, hatch, issues));
  CHECK(hatch.Paths.empty());
  Bits::Writer edges;
  Head(edges, true);
  edges.BL(1);
  edges.BL(1);
  edges.BL(many);
  CHECK_FALSE(ReadBits(edges, hatch, issues));
  Bits::Writer vertices;
  Head(vertices, true);
  vertices.BL(1);
  vertices.BL(2);
  vertices.B(false);
  vertices.B(true);
  vertices.BL(many);
  CHECK_FALSE(ReadBits(vertices, hatch, issues));
  Bits::Writer sources;
  Head(sources, true);
  sources.BL(1);
  sources.BL(2);
  sources.B(false);
  sources.B(true);
  sources.BL(0);
  sources.BL(many);
  CHECK_FALSE(ReadBits(sources, hatch, issues));
  for (const int inside : {0, 1})
  {
    Bits::Writer knots;
    Head(knots, true);
    knots.BL(1);
    knots.BL(1);
    knots.BL(1);
    knots.RC(4);
    knots.BL(3);
    knots.B(false);
    knots.B(false);
    knots.BL(inside == 0 ? many : 0);
    knots.BL(many);
    CHECK_FALSE(ReadBits(knots, hatch, issues));
  }
  Bits::Writer stripes;
  Head(stripes, false);
  stripes.BL(0);
  stripes.BS(0);
  stripes.BS(1);
  stripes.BD(0);
  stripes.BD(1);
  stripes.B(false);
  stripes.BS(30000);
  CHECK_FALSE(ReadBits(stripes, hatch, issues));
  Bits::Writer dashes;
  Head(dashes, false);
  dashes.BL(0);
  dashes.BS(0);
  dashes.BS(1);
  dashes.BD(0);
  dashes.BD(1);
  dashes.B(false);
  dashes.BS(1);
  dashes.BD(0);
  dashes.Point2(Vector{});
  dashes.Point2(Vector{});
  dashes.BS(30000);
  CHECK_FALSE(ReadBits(dashes, hatch, issues));
  Bits::Writer seeds;
  Head(seeds, true);
  seeds.BL(0);
  seeds.BS(0);
  seeds.BS(1);
  seeds.BL(many);
  CHECK_FALSE(ReadBits(seeds, hatch, issues));
  CHECK(issues.empty());
}

TEST_CASE("entities/hatch: an edge type the specification lacks is noted and stops the read")
{
  std::vector<Issue> issues;
  Hatch hatch;
  Bits::Writer odd;
  Head(odd, true);
  odd.BL(1);
  odd.BL(1);
  odd.BL(1);
  odd.RC(9);
  CHECK_FALSE(ReadBits(odd, hatch, issues));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "hatch edge type 9 unknown");
}

TEST_CASE("entities/hatch: an R2004 hatch cut inside its gradient is noted and not read")
{
  // Some cut lands in the gradient's shade count, and the reader says so rather than sizing a list.
  Hatch hatch;
  hatch.Base.Self = Handle{.Value = 0x50};
  CHECK(Objects::CutShort<Hatch>(Object{hatch}, Version::R2004) > 0);
}
} // namespace DwgIO::Entities
