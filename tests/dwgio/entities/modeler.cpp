/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/modeler.cpp
 *  @brief     The ACIS entities against their DXF twin in every release, and saved byte for byte.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/modeler.hpp"

#include "file/coded/layout.hpp"
#include "file/paged/layout.hpp"
#include "octets.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"

#include "dwgio/entities/geometry.hpp"
#include "dwgio/entities/procedural.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  The ACIS entities of a kinds fixture: three solids, a region, a body and eight surfaces.
 */
constexpr std::size_t Modeled = 13;

auto ReleaseOf(std::string_view name) -> Version
{
  for (std::size_t at = 0; at < FixtureSuffixes.size(); ++at)
  {
    if (FixtureSuffixes.at(at) == name)
    {
      return FixtureReleases.at(at);
    }
  }
  return Version::R2018;
}

/**
 *  The groups of a record after its subclass marker, to the next subclass or record.
 */
auto Sub(const std::vector<Pair> &twin, std::size_t opened, std::string_view marker) -> std::vector<Pair>
{
  std::vector<Pair> groups;
  bool inside = false;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    if (twin[at].Code == 100)
    {
      inside = twin[at].Value == marker;
      continue;
    }
    if (inside)
    {
      groups.push_back(twin[at]);
    }
  }
  return groups;
}

auto Of(const std::vector<Pair> &groups, int code) -> std::vector<std::string>
{
  std::vector<std::string> values;
  for (const Pair &pair : groups)
  {
    if (pair.Code == code)
    {
      values.push_back(pair.Value);
    }
  }
  return values;
}

auto RealOf(const std::vector<Pair> &groups, int code) -> double
{
  return std::stod(Of(groups, code).at(0));
}

auto WholeOf(const std::vector<Pair> &groups, int code) -> int
{
  return std::stoi(Of(groups, code).at(0));
}

auto PointOf(const std::vector<Pair> &groups, int code) -> Vector
{
  return Vector{.X = RealOf(groups, code), .Y = RealOf(groups, code + 10), .Z = RealOf(groups, code + 20)};
}

void CheckMatrix(const Matrix &matrix, const std::vector<Pair> &groups, int code)
{
  const std::vector<std::string> values = Of(groups, code);
  REQUIRE(values.size() == matrix.size());
  for (std::size_t at = 0; at < matrix.size(); ++at)
  {
    CHECK(matrix.at(at) == doctest::Approx(std::stod(values.at(at))));
  }
}

/**
 *  The hexadecimal text of a profile's bytes, as group 310 has it.
 */
auto HexOf(const Profile &profile) -> std::string
{
  std::string text;
  for (const std::byte one : profile.Data.Bytes)
  {
    text += std::format("{:02X}", std::to_integer<unsigned>(one));
  }
  return text;
}

/**
 *  The profiles of a procedural surface's groups, in order: each a 90 or 91 of its type, a 90 of its bits and its 310
 *  groups.
 */
auto Profiles(const std::vector<Pair> &groups) -> std::vector<Profile>
{
  std::vector<Profile> profiles;
  std::vector<std::string> hexes;
  for (std::size_t at = 0; at + 1 < groups.size(); ++at)
  {
    if ((groups[at].Code == 90 || groups[at].Code == 91) && groups[at + 1].Code == 90)
    {
      Profile profile{.Type = std::stoi(groups[at].Value), .Data = {}};
      profile.Data.Size = std::stoul(groups[at + 1].Value);
      profiles.push_back(profile);
      hexes.emplace_back();
      ++at;
    }
    else if (groups[at].Code == 310 && !hexes.empty())
    {
      hexes.back() += groups[at].Value;
    }
  }
  if (!groups.empty() && groups.back().Code == 310 && !hexes.empty())
  {
    hexes.back() += groups.back().Value;
  }
  for (std::size_t at = 0; at < profiles.size(); ++at)
  {
    for (std::size_t one = 0; one < hexes.at(at).size(); one += 2)
    {
      profiles.at(at).Data.Bytes.push_back(
          static_cast<std::byte>(std::stoul(hexes.at(at).substr(one, 2), nullptr, 16)));
    }
  }
  return profiles;
}

void CheckProfile(const Profile &ours, const Profile &theirs)
{
  CHECK(ours.Type == theirs.Type);
  CHECK(ours.Data.Size == theirs.Data.Size);
  // DXF writes a byte more than the bits fill, which the file has not.
  CHECK(HexOf(theirs).starts_with(HexOf(ours)));
  CHECK(HexOf(theirs).size() == HexOf(ours).size() + 2);
}

void CheckOptions(const SweepOptions &options, const std::vector<Pair> &groups)
{
  CHECK(options.Draft == doctest::Approx(RealOf(groups, 42)));
  CHECK(options.StartDraft == doctest::Approx(RealOf(groups, 43)));
  CHECK(options.EndDraft == doctest::Approx(RealOf(groups, 44)));
  CHECK(options.Twist == doctest::Approx(RealOf(groups, 45)));
  CHECK(options.Scale == doctest::Approx(RealOf(groups, 48)));
  CHECK(options.Align == doctest::Approx(RealOf(groups, 49)));
  CHECK(options.Solid == (WholeOf(groups, 290) == 1));
  CHECK(options.Alignment == WholeOf(groups, 70));
  CHECK(options.Miter == WholeOf(groups, 71));
  CHECK(options.AlignStart == (WholeOf(groups, 292) == 1));
  CHECK(options.Bank == (WholeOf(groups, 293) == 1));
  CHECK(options.BasePoint == (WholeOf(groups, 294) == 1));
  CHECK(options.ProfileComputed == (WholeOf(groups, 295) == 1));
  CHECK(options.PathComputed == (WholeOf(groups, 296) == 1));
  CHECK(Near(options.Reference, PointOf(groups, 11)));
  CheckMatrix(options.ProfileTransform, groups, 46);
  CheckMatrix(options.PathTransform, groups, 47);
}

/**
 *  The SAT text DXF's groups 1 and 3 carry, decrypted, a caret escaped as a caret and a space taken back.
 */
auto Sat(const std::vector<Pair> &groups) -> std::string
{
  std::vector<std::string> lines;
  for (const Pair &pair : groups)
  {
    if (pair.Code == 1)
    {
      lines.push_back(pair.Value);
    }
    else if (pair.Code == 3)
    {
      lines.back() += pair.Value;
    }
  }
  std::string text;
  for (const std::string &line : lines)
  {
    for (std::size_t at = 0; at < line.size(); ++at)
    {
      const auto one = static_cast<unsigned char>(line[at]);
      if (one == '^' && at + 1 < line.size() && line[at + 1] == ' ')
      {
        ++at;
      }
      text.push_back(one >= 0x21 && one <= 0x7E ? static_cast<char>(0x9F - one) : static_cast<char>(one));
    }
    text.push_back('\n');
  }
  return text;
}

/**
 *  The SAT text from its third line, which drops the date the file keeps and DXF does not, tabs as DXF's spaces and no
 *  line's blanks at either end, which the twin's reader trims.
 */
auto Lines(std::string text) -> std::string
{
  std::ranges::replace(text, '\t', ' ');
  const std::size_t first = text.find('\n');
  std::string kept;
  std::string line;
  for (const char one : text.substr(text.find('\n', first + 1) + 1))
  {
    if (one != '\n')
    {
      line.push_back(one);
      continue;
    }
    const std::size_t start = std::min(line.find_first_not_of(' '), line.size());
    kept += line.substr(start, line.find_last_not_of(' ') + 1 - start) + "\n";
    line.clear();
  }
  return kept;
}

/**
 *  Checks the geometry against the twin's AcDbModelerGeometry groups for the release.
 */
void CheckGeometry(const Geometry &geometry, const std::vector<Pair> &groups, Version release)
{
  CHECK(geometry.Second.Version == (release >= Version::R2013 ? geometry.Second.Version : 0));
  if (release >= Version::R2013)
  {
    CHECK(geometry.Body.Version == 0);
    CHECK(geometry.Stored == (WholeOf(groups, 290) == 1));
    CHECK(geometry.Guid == Of(groups, 2).at(0));
    return;
  }
  CHECK(WholeOf(groups, 70) == 1);
  CHECK(geometry.Guid.empty());
  if (release >= Version::R2004)
  {
    CHECK(geometry.Body.Version == 2);
    CHECK_FALSE(geometry.Body.Textual);
    CHECK(TextOf(geometry.Body.Data).starts_with("ACIS BinaryFile"));
    CHECK(TextOf(geometry.Body.Data).ends_with("data"));
    CHECK_FALSE(Of(groups, 1).empty());
    return;
  }
  CHECK(geometry.Body.Version == 1);
  CHECK(geometry.Body.Textual);
  if (release == Version::R2000)
  {
    CHECK(Lines(TextOf(geometry.Body.Data)) == Lines(Sat(groups)));
  }
  else
  {
    CHECK(TextOf(geometry.Body.Data).starts_with("106 ")); // R14's ACIS 1.6, where the twin of R2000 has 4.0.
  }
}

/**
 *  Checks a surface's isolines and its kind's own data against the twin.
 */
template <SurfaceKind KindV>
void CheckSurface(const SurfaceOf<KindV> &surface, const std::vector<Pair> &twin, std::size_t at, Version release)
{
  const std::vector<Pair> sheet = Sub(twin, at, "AcDbSurface");
  CHECK(surface.UIsolines == WholeOf(sheet, 71));
  CHECK(surface.VIsolines == WholeOf(sheet, 72));
  if constexpr (KindV == SurfaceKind::Nurbs)
  {
    const std::vector<Pair> own = Sub(twin, at, "AcDbNurbSurface");
    CHECK(release >= Version::R2013);
    CHECK(surface.Data.Hull == WholeOf(own, 170));
    CHECK(surface.Data.Unknown1 == (WholeOf(own, 290) == 1));
    for (std::size_t one = 0; one < surface.Data.Points.size(); ++one)
    {
      CHECK(Near(surface.Data.Points.at(one), PointOf(own, 10 + static_cast<int>(one))));
    }
  }
  if constexpr (KindV == SurfaceKind::Extruded)
  {
    const std::vector<Pair> own = Sub(twin, at, "AcDbExtrudedSurface");
    CHECK(Near(surface.Data.Direction, PointOf(own, 10)));
    CheckMatrix(surface.Data.Transform, own, 40);
    CheckOptions(surface.Data.Options, own);
    const std::vector<Profile> profiles = Profiles(own);
    REQUIRE(profiles.size() == 1);
    CheckProfile(surface.Data.Profile, profiles.at(0));
  }
  if constexpr (KindV == SurfaceKind::Swept)
  {
    const std::vector<Pair> own = Sub(twin, at, "AcDbSweptSurface");
    CheckMatrix(surface.Data.ProfileTransform, own, 40);
    CheckMatrix(surface.Data.PathTransform, own, 41);
    CheckOptions(surface.Data.Options, own);
    const std::vector<Profile> profiles = Profiles(own);
    REQUIRE(profiles.size() == 2);
    CheckProfile(surface.Data.Profile, profiles.at(0));
    CheckProfile(surface.Data.Path, profiles.at(1));
  }
  if constexpr (KindV == SurfaceKind::Revolved)
  {
    const std::vector<Pair> own = Sub(twin, at, "AcDbRevolvedSurface");
    CHECK(Near(surface.Data.Point, PointOf(own, 10)));
    CHECK(Near(surface.Data.Axis, PointOf(own, 11)));
    CHECK(surface.Data.Angle == doctest::Approx(RealOf(own, 40)));
    CHECK(surface.Data.Start == doctest::Approx(RealOf(own, 41)));
    CheckMatrix(surface.Data.Transform, own, 42);
    CHECK(surface.Data.Draft == doctest::Approx(RealOf(own, 43)));
    CHECK(surface.Data.StartDraft == doctest::Approx(RealOf(own, 44)));
    CHECK(surface.Data.EndDraft == doctest::Approx(RealOf(own, 45)));
    CHECK(surface.Data.Twist == doctest::Approx(RealOf(own, 46)));
    CHECK(surface.Data.Solid == (WholeOf(own, 290) == 1));
    CHECK(surface.Data.Close == (WholeOf(own, 291) == 1));
    const std::vector<Profile> profiles = Profiles(own);
    REQUIRE(profiles.size() == 1);
    CheckProfile(surface.Data.Profile, profiles.at(0));
  }
  if constexpr (KindV == SurfaceKind::Lofted)
  {
    const std::vector<Pair> own = Sub(twin, at, "AcDbLoftedSurface");
    const Loft &loft = surface.Data;
    CheckMatrix(loft.Transform, own, 40);
    CHECK(loft.Normal == WholeOf(own, 70));
    CHECK(loft.StartDraft == doctest::Approx(RealOf(own, 41)));
    CHECK(loft.EndDraft == doctest::Approx(RealOf(own, 42)));
    CHECK(loft.StartMagnitude == doctest::Approx(RealOf(own, 43)));
    CHECK(loft.EndMagnitude == doctest::Approx(RealOf(own, 44)));
    const std::array<bool, 8> bits{loft.ArcLength, loft.NoTwist, loft.AlignDirection, loft.Simple,
                                   loft.Closed,    loft.Solid,   loft.Ruled,          loft.VirtualGuide};
    for (std::size_t one = 0; one < bits.size(); ++one)
    {
      CHECK(bits.at(one) == (WholeOf(own, 290 + static_cast<int>(one)) == 1));
    }
    const std::vector<Profile> profiles = Profiles(own);
    REQUIRE(profiles.size() == loft.Sections.size());
    for (std::size_t one = 0; one < profiles.size(); ++one)
    {
      CheckProfile(loft.Sections.at(one), profiles.at(one));
    }
    CHECK(loft.Guides.empty());
    CHECK_FALSE(loft.Path);
  }
}
} // namespace

TEST_CASE("entities/modeler: the ACIS entities of every release match the DXF twin")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    const Version version = ReleaseOf(release);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("kinds-{}.dwg", release)));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    // R14 has no twin; its entities have the R2000 twin's handles.
    const std::vector<Pair> twin =
        ReadTwin(std::format("kinds-{}.dxf", version == Version::R14 ? std::string_view{"r2000"} : release));
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      CAPTURE(HexOf(BaseOf(object).Self));
      Visit(
          [&](const auto &kind) -> void
          {
            using KindT = std::remove_cvref_t<decltype(kind)>;
            if constexpr (requires { kind.Geometry; })
            {
              ++count;
              const std::optional<std::size_t> opened = Entity(twin, kind.Base.Self);
              REQUIRE(opened);
              CHECK(TypeOf(object) == twin.at(*opened).Value);
              CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, kind.Common, kind.Base);
              const std::vector<Pair> geometry = Sub(twin, *opened, "AcDbModelerGeometry");
              CheckGeometry(kind.Geometry, geometry, version);
              if constexpr (std::is_same_v<KindT, Solid3d>)
              {
                const std::string history = GroupOf(twin, *opened, 350);
                CHECK(kind.History == (version >= Version::R2007 ? ParseHandle(history).value_or(Handle{}) : Handle{}));
              }
              if constexpr (requires { kind.UIsolines; })
              {
                CheckSurface(kind, twin, *opened, version);
              }
            }
          },
          object);
    }
    CHECK(count == Modeled);
  }
}

TEST_CASE("entities/modeler: an R2013 NURBS surface keeps a second SAB stream after its wires")
{
  const Drawing drawing = Load(ReadFixture("kinds-r2018.dwg")).value().Drawing;
  std::size_t nurbs = 0;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *surface = GetIf<NurbSurface>(&object))
    {
      ++nurbs;
      CHECK(surface->Geometry.Second.Version == 2);
      CHECK(TextOf(surface->Geometry.Second.Data)
                .ends_with(std::string_view{"\x0E\x03"
                                            "End"
                                            "\x0E\x02"
                                            "of"
                                            "\x0E\x03"
                                            "ASM"
                                            "\x0D\x04"
                                            "data"}));
      CHECK(surface->Geometry.Wires.size() == 12);
    }
  }
  CHECK(nurbs == 3);
}

TEST_CASE("entities/modeler: every kinds fixture saved to its own release loads back equal section for section")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    const std::vector<std::byte> bytes = ReadFixture(std::format("kinds-{}.dwg", release));
    const std::expected<Loaded, Failure> loaded = Load(bytes);
    REQUIRE(loaded);
    const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> again = Load(saved->Bytes);
    REQUIRE(again);
    CHECK(again->Issues.empty());
    CHECK(again->Drawing == loaded->Drawing);
    const Version version = loaded->Drawing.Release;
    if (version < Version::R2004)
    {
      CHECK(saved->Bytes == bytes);
      continue;
    }
    std::vector<Issue> issues;
    const std::vector<std::byte> theirs = ReadFixture(std::format("kinds-{}.dwg", release));
    const std::vector<std::byte> ours{reinterpret_cast<const std::byte *>(saved->Bytes.data()),
                                      reinterpret_cast<const std::byte *>(saved->Bytes.data() + saved->Bytes.size())};
    std::vector<std::pair<std::string, std::vector<std::byte>>> mine;
    std::vector<std::pair<std::string, std::vector<std::byte>>> its;
    const auto sections = [&issues](const std::vector<std::byte> &file, Version of)
    {
      std::vector<std::pair<std::string, std::vector<std::byte>>> named;
      if (of == Version::R2007)
      {
        const File::Coded::Layout layout = *File::Coded::ReadLayout(file, issues);
        for (const File::Paged::Section &section : layout.Sections)
        {
          named.emplace_back(section.Name, section.Bytes);
        }
        return named;
      }
      const File::Paged::Layout layout = *File::Paged::ReadLayout(file, issues);
      for (const File::Paged::Section &section : layout.Sections)
      {
        named.emplace_back(section.Name, section.Bytes);
      }
      return named;
    };
    mine = sections(ours, version);
    its = sections(theirs, version);
    CHECK(issues.empty());
    REQUIRE(mine.size() == its.size());
    for (std::size_t at = 0; at < its.size(); ++at)
    {
      CAPTURE(its.at(at).first);
      CHECK(mine.at(at).first == its.at(at).first);
      // R2007's preview names file addresses; every save makes the application information again.
      if ((version != Version::R2007 || its.at(at).first != "preview") && !RemakesSection(its.at(at).first))
      {
        CHECK(mine.at(at).second == its.at(at).second);
      }
    }
  }
}

} // namespace DwgIO::Entities
