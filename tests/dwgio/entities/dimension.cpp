/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/dimension.cpp
 *  @brief     Every kind of dimension against its DXF twin, and written back in every release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/dimension.hpp"

#include "classes/catalog.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/class.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <expected>
#include <format>
#include <map>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
constexpr double Degree = std::numbers::pi / 180.0;

/**
 *  The variant index each kind of group 70 stands for: 0 rotated, 1 aligned, 2 angular, 3 diameter, 4 radius, 5
 *  three-point angular, 6 ordinate.
 */
constexpr std::array<std::size_t, 7> Indices{1, 2, 4, 6, 5, 3, 0};

constexpr std::array<std::string_view, 7> Names{"DIMENSION_ORDINATE", "DIMENSION_LINEAR", "DIMENSION_ALIGNED",
                                                "DIMENSION_ANG3PT",   "DIMENSION_ANG2LN", "DIMENSION_RADIUS",
                                                "DIMENSION_DIAMETER"};

auto Angle(const std::vector<Pair> &twin, std::size_t at, int code) -> double
{
  return ReadReal(twin, at, code) * Degree;
}

void CheckForm(const std::vector<Pair> &twin, std::size_t at, const Forms::Form &form)
{
  if (const auto *ordinate = std::get_if<Forms::Ordinate>(&form))
  {
    CHECK(Near(ordinate->Feature, ReadPoint(twin, at, 13)));
    CHECK(Near(ordinate->Leader, ReadPoint(twin, at, 14)));
  }
  else if (const auto *linear = std::get_if<Forms::Linear>(&form))
  {
    CHECK(Near(linear->Start, ReadPoint(twin, at, 13)));
    CHECK(Near(linear->End, ReadPoint(twin, at, 14)));
    CHECK(linear->Oblique == doctest::Approx(Angle(twin, at, 52)));
    CHECK(linear->Angle == doctest::Approx(Angle(twin, at, 50)));
  }
  else if (const auto *aligned = std::get_if<Forms::Aligned>(&form))
  {
    CHECK(Near(aligned->Start, ReadPoint(twin, at, 13)));
    CHECK(Near(aligned->End, ReadPoint(twin, at, 14)));
    CHECK(aligned->Oblique == doctest::Approx(Angle(twin, at, 52)));
  }
  else if (const auto *points = std::get_if<Forms::Angular3Point>(&form))
  {
    CHECK(Near(points->Start, ReadPoint(twin, at, 13)));
    CHECK(Near(points->End, ReadPoint(twin, at, 14)));
    CHECK(Near(points->Vertex, ReadPoint(twin, at, 15)));
  }
  else if (const auto *angular = std::get_if<Forms::Angular2Line>(&form))
  {
    CHECK(Near(angular->FirstStart, ReadPoint(twin, at, 13)));
    CHECK(Near(angular->FirstEnd, ReadPoint(twin, at, 14)));
    CHECK(Near(angular->SecondStart, ReadPoint(twin, at, 15)));
    CHECK(Near(angular->ArcPoint, ReadPoint(twin, at, 16)));
  }
  else if (const auto *radial = std::get_if<Forms::Radial>(&form))
  {
    CHECK(Near(radial->Chord, ReadPoint(twin, at, 15)));
    CHECK(radial->Leader == doctest::Approx(ReadReal(twin, at, 40)));
  }
  else if (const auto *diameter = std::get_if<Forms::Diameter>(&form))
  {
    CHECK(Near(diameter->Chord, ReadPoint(twin, at, 15)));
    CHECK(diameter->Leader == doctest::Approx(ReadReal(twin, at, 40)));
  }
}

/**
 *  The R2000 fields take their defaults in an R14 file, the measurement among them.
 */
void CheckMeasure(const TwinPlace &seen, const Measure &measure)
{
  const std::vector<Pair> &twin = *seen.Twin;
  const std::size_t at = seen.At;
  const bool modern = seen.Drawing->Release >= Version::R2000;
  CHECK(Near(measure.Normal, Normal(twin, at)));
  CHECK(Near(measure.TextMiddle, ReadPoint(twin, at, 11)));
  CHECK(Near(measure.CloneInsertion, ReadPoint(twin, at, 12)));
  CHECK(Near(measure.DefinitionPoint, ReadPoint(twin, at, 10)));
  // The low four bits name the kind: 0 to 6 a DIMENSION's, 5 an arc length's, 9 a jogged radius's.
  CHECK(measure.Flags == (ReadWhole(twin, at, 70) & ~15));
  CHECK(measure.Unknown1 == 8);
  CHECK(measure.Text == GroupOf(twin, at, 1));
  CHECK(measure.TextRotation == doctest::Approx(Angle(twin, at, 53)));
  CHECK(measure.HorizontalDirection == doctest::Approx(Angle(twin, at, 51)));
  CHECK(measure.Scale == Vector{.X = 1, .Y = 1, .Z = 1});
  CHECK(measure.BlockRotation == doctest::Approx(Angle(twin, at, 54)));
  CHECK(measure.Attachment == ReadWhole(twin, at, 71));
  CHECK(measure.LineSpacingStyle == (GroupOf(twin, at, 72).empty() ? 1 : ReadWhole(twin, at, 72)));
  CHECK(measure.LineSpacingFactor == doctest::Approx(GroupOf(twin, at, 41).empty() ? 1.0 : ReadReal(twin, at, 41)));
  CHECK(measure.Measured == doctest::Approx(modern ? ReadReal(twin, at, 42) : -1));
}
} // namespace

TEST_CASE("entities/dimension: the fixture's nine dimensions match the DXF twin each under its kind")
{
  const std::vector<Pair> twin = ReadTwin("curves-r2000.dxf");
  for (const std::string_view name : {"curves-r2000.dwg", "curves-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::map<std::string_view, int> kinds;
    std::size_t worded = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *dimension = GetIf<Dimension>(&object);
      if (dimension == nullptr)
      {
        continue;
      }
      CAPTURE(HexOf(dimension->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, dimension->Base.Self);
      REQUIRE(opened);
      const std::size_t at = *opened;
      CHECK(twin[at].Value == "DIMENSION");
      const auto kind = static_cast<std::size_t>(ReadWhole(twin, at, 70) & 7);
      CHECK(dimension->Form.index() == Indices.at(kind));
      CHECK(TypeOf(object) == Names.at(dimension->Form.index()));
      ++kinds[TypeOf(object)];
      CheckMeasure(TwinPlace{.Twin = &twin, .At = at, .Drawing = &loaded->Drawing}, dimension->Measure);
      CheckForm(twin, at, dimension->Form);
      worded += dimension->Measure.Text == "Width <>" ? 1U : 0U;
      const std::optional<std::size_t> style = FindOpeningNoCase(twin, "DIMSTYLE", GroupOf(twin, at, 3));
      REQUIRE(style);
      CHECK(dimension->Style == Self(twin, *style));
      const std::optional<std::size_t> block = FindOpeningNoCase(twin, "BLOCK_RECORD", GroupOf(twin, at, 2));
      REQUIRE(block);
      CHECK(dimension->Block == Self(twin, *block));
      CheckCommon(TwinPlace{.Twin = &twin, .At = at, .Drawing = &loaded->Drawing}, dimension->Common, dimension->Base);
    }
    CHECK(kinds["DIMENSION_ORDINATE"] == 2);
    CHECK(kinds["DIMENSION_LINEAR"] == 2);
    CHECK(kinds["DIMENSION_ALIGNED"] == 1);
    CHECK(kinds["DIMENSION_ANG3PT"] == 1);
    CHECK(kinds["DIMENSION_ANG2LN"] == 1);
    CHECK(kinds["DIMENSION_RADIUS"] == 1);
    CHECK(kinds["DIMENSION_DIAMETER"] == 1);
    CHECK(worded == 1);
  }
}

TEST_CASE("entities/dimension: a dimension of each form writes as R2000 under its code and reads back equal")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Dimension dimension;
  dimension.Base.Self = Handle{0x100};
  dimension.Common.Space = Space::Model;
  dimension.Common.Layer = Handle{0x10};
  dimension.Common.Lineweight = Lineweight::ByLayer;
  dimension.Measure.TextMiddle = Vector{.X = 1, .Y = 2, .Z = 3};
  dimension.Measure.CloneInsertion = Vector{.X = 4, .Y = 5, .Z = 3};
  dimension.Measure.DefinitionPoint = Vector{.X = 6, .Y = 7, .Z = 8};
  dimension.Measure.Flags = 32 | 128;
  dimension.Measure.Text = "<> mm";
  dimension.Style = Handle{0x27};
  dimension.Block = Handle{0x1F0};
  const Vector one{.X = 1, .Y = 1, .Z = 1};
  const Vector two{.X = 2, .Y = 2, .Z = 2};
  const Vector three{.X = 3, .Y = 3, .Z = 3};
  const std::array<Forms::Form, 7> forms{
      Forms::Ordinate{.Feature = one, .Leader = two},
      Forms::Linear{.Start = one, .End = two, .Oblique = 0.1, .Angle = 0.2},
      Forms::Aligned{.Start = one, .End = two, .Oblique = 0.1},
      Forms::Angular3Point{.Start = one, .End = two, .Vertex = three},
      Forms::Angular2Line{
          .FirstStart = one, .FirstEnd = two, .SecondStart = three, .ArcPoint = Vector{.X = 9, .Y = 9, .Z = 3}},
      Forms::Radial{.Chord = one, .Leader = 0.5},
      Forms::Diameter{.Chord = one, .Leader = 0.5}};
  for (std::size_t index = 0; index < forms.size(); ++index)
  {
    CAPTURE(index);
    dimension.Form = forms.at(index);
    dimension.Measure.Flags = index == 0 ? 64 : index == 1 ? 0 : 32 | 128;
    const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{dimension}, with), 0, reading);
    REQUIRE(back);
    CHECK(*back == Object{dimension});
    CHECK(TypeOf(*back) == Names.at(index));
  }
  CHECK(issues.empty());
}

namespace
{
/**
 *  The groups of one subclass of the record opened at that index, after its 100 marker, as a record of its own the
 *  twin's helpers read: the record's 0 group, the subclass's groups, then a 0 group to end it.
 */
auto Subclass(const std::vector<Pair> &twin, std::size_t opened, std::string_view marker) -> std::vector<Pair>
{
  std::vector<Pair> part{twin[opened]};
  bool inside = false;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    if (twin[at].Code == 100)
    {
      inside = twin[at].Value == marker;
    }
    else if (inside)
    {
      part.push_back(twin[at]);
    }
  }
  part.push_back(Pair{.Code = 0, .Value = "EOF"});
  return part;
}

/**
 *  The proxy graphics the twin gives a record, its 310 groups' hex as one string.
 */
auto CollectGraphics(const std::vector<Pair> &twin, std::size_t opened) -> std::string
{
  std::string hex;
  for (const std::string &chunk : Every(twin, opened, 310))
  {
    hex += chunk;
  }
  return hex;
}

auto HexOf(const std::vector<std::byte> &bytes) -> std::string
{
  std::string hex;
  for (const std::byte one : bytes)
  {
    hex += std::format("{:02X}", std::to_integer<unsigned>(one));
  }
  return hex;
}

/**
 *  What an arc length or jogged radius dimension shares with the others, checked against the twin: the measure, the
 *  style and the block, the common data and the proxy graphics.
 */
template <typename KindT>
void CheckShared(const TwinPlace &seen, const KindT &kind)
{
  const std::vector<Pair> &twin = *seen.Twin;
  const std::vector<Pair> measure = Subclass(twin, seen.At, "AcDbDimension");
  CheckMeasure(TwinPlace{.Twin = &measure, .At = 0, .Drawing = seen.Drawing}, kind.Measure);
  const std::optional<std::size_t> style = FindOpeningNoCase(twin, "DIMSTYLE", GroupOf(measure, 0, 3));
  REQUIRE(style);
  CHECK(kind.Style == Self(twin, *style));
  const std::optional<std::size_t> block = FindOpeningNoCase(twin, "BLOCK_RECORD", GroupOf(measure, 0, 2));
  REQUIRE(block);
  CHECK(kind.Block == Self(twin, *block));
  CheckCommon(seen, kind.Common, kind.Base);
  // An R14 file's proxy graphics are of R14's own shape, where its twin is R2000's.
  if (seen.Drawing->Release >= Version::R2000)
  {
    CHECK(HexOf(kind.Common.Graphics) == CollectGraphics(twin, seen.At));
  }
  else
  {
    CHECK_FALSE(kind.Common.Graphics.empty());
  }
}
} // namespace

TEST_CASE("entities/dimension: the arc length and jogged radius dimensions of every release match the DXF twin")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    const std::vector<Pair> twin = ReadTwin(std::format("dimensions-{}.dxf", release == "r14" ? "r2000" : release));
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("dimensions-{}.dwg", release)));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t arcs = 0;
    std::size_t jogs = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const std::optional<std::size_t> opened = Entity(twin, BaseOf(object).Self);
      const TwinPlace seen{.Twin = &twin, .At = opened.value_or(0), .Drawing = &loaded->Drawing};
      if (const auto *arc = GetIf<ArcDimension>(&object))
      {
        ++arcs;
        REQUIRE(opened);
        CHECK(twin[*opened].Value == "ARC_DIMENSION");
        CHECK(TypeOf(object) == "ARC_DIMENSION");
        CheckShared(seen, *arc);
        const std::vector<Pair> part = Subclass(twin, *opened, "AcDbArcDimension");
        CHECK(Near(arc->Start, ReadPoint(part, 0, 13)));
        CHECK(Near(arc->End, ReadPoint(part, 0, 14)));
        CHECK(Near(arc->Center, ReadPoint(part, 0, 15)));
        CHECK(arc->Partial == (ReadWhole(part, 0, 70) != 0));
        CHECK(arc->StartAngle == doctest::Approx(ReadReal(part, 0, 40)));
        CHECK(arc->EndAngle == doctest::Approx(ReadReal(part, 0, 41)));
        CHECK(arc->HasLeader == (ReadWhole(part, 0, 71) != 0));
        CHECK(Near(arc->FirstLeaderPoint, ReadPoint(part, 0, 16)));
        CHECK(Near(arc->SecondLeaderPoint, ReadPoint(part, 0, 17)));
      }
      else if (const auto *jog = GetIf<LargeRadialDimension>(&object))
      {
        ++jogs;
        REQUIRE(opened);
        CHECK(twin[*opened].Value == "LARGE_RADIAL_DIMENSION");
        CHECK(TypeOf(object) == "LARGE_RADIAL_DIMENSION");
        CheckShared(seen, *jog);
        // The file keeps the chord point, the jog's angle, the center drawn from and the jog; DXF writes the center
        // drawn from as 13, the jog as 14 and the chord point as 15.
        const std::vector<Pair> part = Subclass(twin, *opened, "AcDbRadialDimensionLarge");
        CHECK(Near(jog->Chord, ReadPoint(part, 0, 15)));
        CHECK(Near(jog->Center, ReadPoint(part, 0, 13)));
        CHECK(Near(jog->JogPoint, ReadPoint(part, 0, 14)));
        CHECK(jog->JogAngle == doctest::Approx(ReadReal(part, 0, 40)));
        // The chord point lies on the circle of radius 10 the dimension measures, centered on the definition point.
        const double across = jog->Chord.X - jog->Measure.DefinitionPoint.X;
        const double up = jog->Chord.Y - jog->Measure.DefinitionPoint.Y;
        CHECK((across * across) + (up * up) == doctest::Approx(100.0));
      }
    }
    CHECK(arcs == 1);
    CHECK(jogs == 1);
  }
}

TEST_CASE("entities/dimension: arc length and jogged radius dimensions round-trip in every release")
{
  ArcDimension arc;
  arc.Base.Self = Handle{0x100};
  arc.Common.Space = Space::Model;
  arc.Common.Layer = Handle{0x10};
  arc.Measure.TextMiddle = Vector{.X = 1, .Y = 2, .Z = 3};
  arc.Measure.CloneInsertion = Vector{.X = 4, .Y = 5, .Z = 3};
  arc.Measure.DefinitionPoint = Vector{.X = 6, .Y = 7, .Z = 8};
  arc.Measure.Flags = 32;
  arc.Measure.Text = "<> long";
  arc.Start = Vector{.X = 20, .Y = 0, .Z = 0};
  arc.Center = Vector{.X = 10, .Y = 0, .Z = 0};
  arc.Partial = true;
  arc.StartAngle = 0.25;
  arc.EndAngle = 2.5;
  arc.HasLeader = true;
  arc.FirstLeaderPoint = Vector{.X = 1, .Y = 1, .Z = 0};
  arc.SecondLeaderPoint = Vector{.X = 2, .Y = 2, .Z = 0};
  arc.Style = Handle{0x27};
  arc.Block = Handle{0x1F0};
  LargeRadialDimension jog;
  jog.Base.Self = Handle{0x101};
  jog.Common.Space = Space::Model;
  jog.Common.Layer = Handle{0x10};
  jog.Measure.TextMiddle = Vector{.X = 61, .Y = 5, .Z = 0};
  jog.Measure.DefinitionPoint = Vector{.X = 50, .Y = 0, .Z = 0};
  jog.Measure.Flags = 160;
  jog.Chord = Vector{.X = 58.9, .Y = 4.5, .Z = 0};
  jog.JogAngle = 0.75;
  jog.Center = Vector{.X = 70, .Y = 20, .Z = 0};
  jog.JogPoint = Vector{.X = 55, .Y = 7.5, .Z = 0};
  jog.Style = Handle{0x27};
  jog.Block = Handle{0x1F1};
  const std::vector<Class> classes{*Classes::FindDeclared("ARC_DIMENSION"),
                                   *Classes::FindDeclared("LARGE_RADIAL_DIMENSION")};
  for (const Version release : FixtureReleases)
  {
    CAPTURE(NameOf(release));
    std::vector<Issue> issues;
    const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{
        .Release = release, .Text = &Objects::Latin, .Classes = classes, .Issues = &issues};
    // Each release writes what it has: R14 no lineweight or measurement, R2000 no flipped arrows.
    arc.Common.Lineweight = release >= Version::R2000 ? std::optional{Lineweight::ByLayer} : std::nullopt;
    jog.Common.Lineweight = arc.Common.Lineweight;
    arc.Measure.Measured = release >= Version::R2000 ? 31.4 : -1;
    jog.Measure.Measured = release >= Version::R2000 ? 10 : -1;
    arc.Measure.FlipFirst = release >= Version::R2007;
    const std::optional<Object> arcBack = Objects::ReadObject(Objects::WriteObject(Object{arc}, with), 0, reading);
    REQUIRE(arcBack);
    CHECK(*arcBack == Object{arc});
    CHECK(TypeOf(*arcBack) == "ARC_DIMENSION");
    const std::optional<Object> jogBack = Objects::ReadObject(Objects::WriteObject(Object{jog}, with), 0, reading);
    REQUIRE(jogBack);
    CHECK(*jogBack == Object{jog});
    CHECK(TypeOf(*jogBack) == "LARGE_RADIAL_DIMENSION");
    CHECK(issues.empty());
  }
}
} // namespace DwgIO::Entities
