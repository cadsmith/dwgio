/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/material.cpp
 *  @brief     The MATERIAL against its DXF twin in every release, and written back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/material.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/class.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
/**
 *  The groups of one map in the twin.
 */
struct Codes
{
  Materials::Map Materials::Maps::*Map;
  int Blend;
  int Source;
  int File;
  int Projection;
  int Tiling;
  int Automatic;
  int Transform;
};

constexpr std::array<Codes, 6> Channels{{
    {&Materials::Maps::Diffuse, 42, 72, 3, 73, 74, 75, 43},
    {&Materials::Maps::Specular, 46, 77, 4, 78, 79, 170, 47},
    {&Materials::Maps::Reflection, 48, 171, 6, 172, 173, 174, 49},
    {&Materials::Maps::Opacity, 141, 175, 7, 176, 177, 178, 142},
    {&Materials::Maps::Bump, 143, 179, 8, 270, 271, 272, 144},
    {&Materials::Maps::Refraction, 146, 273, 9, 274, 275, 276, 147},
}};

/**
 *  A group's whole value, or the fallback AutoCAD's DXF leaves it out at.
 */
auto WholeOr(const std::vector<Pair> &twin, std::size_t at, int code, int fallback) -> int
{
  const std::string value = GroupOf(twin, at, code);
  return value.empty() ? fallback : std::stoi(value);
}

auto RealOr(const std::vector<Pair> &twin, std::size_t at, int code, double fallback) -> double
{
  const std::string value = GroupOf(twin, at, code);
  return value.empty() ? fallback : std::stod(value);
}

void CheckShade(const Materials::MaterialColor &shade, const std::vector<Pair> &twin, std::size_t at,
                const std::array<int, 3> &codes)
{
  CHECK(shade.Method == WholeOr(twin, at, codes[0], 0));
  CHECK(shade.Factor == doctest::Approx(RealOr(twin, at, codes[1], 1)));
  CHECK(shade.Color == WholeOr(twin, at, codes[2], -1040187392));
}

void CheckMaterial(const Material &material, const std::vector<Pair> &twin, std::size_t at, Version release)
{
  CHECK(Upper(material.Name) == Upper(GroupOf(twin, at, 1))); // R14 keeps names in upper case.
  CHECK(material.Description == GroupOf(twin, at, 2));
  CheckShade(material.Ambient, twin, at, {70, 40, 90});
  CheckShade(material.Diffuse, twin, at, {71, 41, 91});
  CheckShade(material.Specular, twin, at, {76, 45, 92});
  CHECK(material.Gloss == doctest::Approx(RealOr(twin, at, 44, 0.5)));
  CHECK(material.Opacity == doctest::Approx(RealOr(twin, at, 140, 1)));
  CHECK(material.Refraction == doctest::Approx(RealOr(twin, at, 145, 1)));
  for (const Codes &codes : Channels)
  {
    const Materials::Map &map = material.Maps.*codes.Map;
    CAPTURE(codes.Blend);
    const int source = &codes == &Channels.back() ? 0 : 1;
    CHECK(map.Blend == doctest::Approx(RealOr(twin, at, codes.Blend, 1)));
    CHECK(map.Source == WholeOr(twin, at, codes.Source, source));
    CHECK(map.File == GroupOf(twin, at, codes.File));
    CHECK(map.Projection == WholeOr(twin, at, codes.Projection, 2));
    CHECK(map.Tiling == WholeOr(twin, at, codes.Tiling, 1));
    CHECK(map.Automatic == WholeOr(twin, at, codes.Automatic, 1));
    const std::vector<std::string> transform = Every(twin, at, codes.Transform);
    for (std::size_t one = 0; one < map.Transform.size(); ++one)
    {
      const double expected = transform.empty() ? DwgIO::Identity.at(one) : std::stod(transform.at(one));
      CHECK(map.Transform.at(one) == doctest::Approx(expected));
    }
  }
  if (release >= Version::R2007)
  {
    CHECK(material.Translucence == doctest::Approx(RealOr(twin, at, 148, 0)));
    CHECK(material.Illumination == doctest::Approx(RealOr(twin, at, 149, 0)));
    CHECK(material.Reflectivity == doctest::Approx(RealOr(twin, at, 468, 0)));
    CHECK(material.Model == WholeOr(twin, at, 93, 0));
    CHECK(material.Channels == WholeOr(twin, at, 94, 0));
    CHECK(material.Mode == WholeOr(twin, at, 282, 0));
  }
}
} // namespace

TEST_CASE("objects/material: the materials of kinds and of the material fixture match the DXF twin")
{
  for (const std::string_view fixture : {"kinds", "material"})
  {
    for (const auto &[release, twinned] :
         {std::pair{"r14", "r2000"}, std::pair{"r2000", "r2000"}, std::pair{"r2004", "r2004"},
          std::pair{"r2007", "r2007"}, std::pair{"r2010", "r2010"}, std::pair{"r2013", "r2013"},
          std::pair{"r2018", "r2018"}})
    {
      const std::string name = std::string{fixture} + "-" + release + ".dwg";
      CAPTURE(name);
      const std::vector<Pair> twin = ReadTwin(std::string{fixture} + "-" + twinned + ".dxf");
      const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
      REQUIRE(loaded);
      CHECK(loaded->Issues.empty());
      std::vector<std::string> names;
      for (const Object &object : loaded->Drawing.Objects)
      {
        const auto *material = GetIf<Material>(&object);
        if (material == nullptr)
        {
          continue;
        }
        CAPTURE(HexOf(material->Base.Self));
        CHECK(TypeOf(object) == "MATERIAL");
        const std::optional<std::size_t> at = FindHandled(twin, material->Base.Self);
        REQUIRE(at);
        CheckMaterial(*material, twin, *at, loaded->Drawing.Release);
        names.push_back(Upper(material->Name));
      }
      CHECK(names.size() == (fixture == "material" ? 4 : 3));
      CHECK(std::ranges::contains(names, "GLOBAL"));
    }
  }
}

TEST_CASE("objects/material: the material fixture sets every channel and round-trips in every release")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("material-r2018.dwg"));
  REQUIRE(loaded);
  const Object *looks = nullptr;
  for (const Object &object : loaded->Drawing.Objects)
  {
    const auto *material = GetIf<Material>(&object);
    looks = material != nullptr && material->Name == "Looks" ? &object : looks;
  }
  REQUIRE(looks != nullptr);
  const Material &material = Get<Material>(*looks);
  CHECK(material.Ambient.Method == 1);
  CHECK(material.Maps.Diffuse.File == "dot.png");
  CHECK(material.Maps.Reflection.Source == 0);
  CHECK(material.Maps.Reflection.File.empty());
  CHECK(material.Maps.Refraction.File == "refraction.png");
  CHECK(material.Channels == 21);
  std::vector<Issue> issues;
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "MATERIAL"}};
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2007, Version::R2018})
  {
    CAPTURE(NameOf(release));
    const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = classes, .Issues = &issues};
    const Material &written = material;
    std::optional<Object> back = ReadObject(WriteObject(Object{written}, with), 0, reading);
    REQUIRE(back);
    Material &read = Get<Material>(*back);
    if (release < Version::R2007) // Before R2007 the file keeps none of the later fields.
    {
      read.Translucence = written.Translucence;
      read.Illumination = written.Illumination;
      read.Reflectivity = written.Reflectivity;
      read.Model = written.Model;
      read.Channels = written.Channels;
      read.Mode = written.Mode;
    }
    CHECK(read == written);
  }
  CHECK(issues.empty());
}
} // namespace DwgIO::Objects
