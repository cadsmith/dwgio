/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/light.cpp
 *  @brief     The LIGHT against its DXF twin in every release, and a new one written.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/light.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/class.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
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
 *  Degrees as the twin gives an angle, in radians.
 */
auto Radians(double degrees) -> double
{
  return degrees * std::numbers::pi / 180.0;
}
} // namespace

TEST_CASE("entities/light: the point and spot and distant lights match the DXF twin in every release")
{
  for (const auto &[name, twinned] :
       {std::pair{"kinds-r14.dwg", "kinds-r2000.dxf"}, std::pair{"kinds-r2000.dwg", "kinds-r2000.dxf"},
        std::pair{"kinds-r2004.dwg", "kinds-r2004.dxf"}, std::pair{"kinds-r2007.dwg", "kinds-r2007.dxf"},
        std::pair{"kinds-r2010.dwg", "kinds-r2010.dxf"}, std::pair{"kinds-r2013.dwg", "kinds-r2013.dxf"},
        std::pair{"kinds-r2018.dwg", "kinds-r2018.dxf"}})
  {
    CAPTURE(name);
    const std::vector<Pair> twin = ReadTwin(twinned);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    std::vector<std::string> names;
    for (const Object &object : drawing.Objects)
    {
      const auto *light = GetIf<Light>(&object);
      if (light == nullptr)
      {
        continue;
      }
      CAPTURE(HexOf(light->Base.Self));
      CHECK(TypeOf(object) == "LIGHT");
      const std::optional<std::size_t> at = Entity(twin, light->Base.Self);
      REQUIRE(at);
      CheckCommon(TwinPlace{.Twin = &twin, .At = *at, .Drawing = &drawing}, light->Common, light->Base);
      names.push_back(light->Name);
      CHECK(light->Version == ReadWhole(twin, *at, 90));
      CHECK(light->Name == GroupOf(twin, *at, 1));
      CHECK(light->Kind == ReadWhole(twin, *at, 70));
      CHECK(light->Lit == (ReadWhole(twin, *at, 290) == 1));
      // R2000's DXF packs the color in a 90 group; the file keeps the index, and from R2004 the true color.
      if (drawing.Release >= Version::R2004)
      {
        REQUIRE(light->Color.Rgb);
        CHECK((*light->Color.Rgb & 0xFFFFFF) == ReadWhole(twin, *at, 421));
      }
      else
      {
        CHECK(light->Color.Index == 7);
      }
      CHECK(light->Glyph == (ReadWhole(twin, *at, 291) == 1));
      CHECK(light->Intensity == doctest::Approx(ReadReal(twin, *at, 40)));
      CHECK(Near(light->Position, ReadPoint(twin, *at, 10)));
      CHECK(Near(light->Target, ReadPoint(twin, *at, 11)));
      CHECK(light->Fading == ReadWhole(twin, *at, 72));
      CHECK(light->Limited == (ReadWhole(twin, *at, 292) == 1));
      CHECK(light->AttenuationStart == doctest::Approx(ReadReal(twin, *at, 41)));
      CHECK(light->AttenuationEnd == doctest::Approx(ReadReal(twin, *at, 42)));
      CHECK(light->Hotspot == doctest::Approx(Radians(ReadReal(twin, *at, 50))));
      CHECK(light->Falloff == doctest::Approx(Radians(ReadReal(twin, *at, 51))));
      CHECK(light->CastShadows == (ReadWhole(twin, *at, 293) == 1));
      CHECK(light->Shading == ReadWhole(twin, *at, 73));
      CHECK(light->Size == ReadWhole(twin, *at, 91));
      CHECK(light->Softness == ReadWhole(twin, *at, 280));
    }
    CHECK(names == std::vector<std::string>{"Pointlight1", "Spotlight2", "Distantlight3"});
  }
}

TEST_CASE("entities/light: a new light round-trips under its class with a new light's defaults")
{
  std::vector<Issue> issues;
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "LIGHT", .ItemClass = 0x1F2}};
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2007, Version::R2018})
  {
    CAPTURE(NameOf(release));
    const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{
        .Release = release, .Text = &Objects::Latin, .Classes = classes, .Issues = &issues};
    Light light;
    light.Base.Self = Handle{0x40};
    light.Base.Owner = Handle{0x1F};
    light.Common.Layer = Handle{0x10};
    light.Name = "Lamp";
    light.Position = Vector{.X = 1, .Y = 2, .Z = 3};
    const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{light}, with), 0, reading);
    REQUIRE(back);
    REQUIRE(Holds<Light>(*back));
    const Light &read = Get<Light>(*back);
    CHECK(read.Name == "Lamp");
    CHECK(read.Position == light.Position);
    CHECK(read.Kind == 2);
    CHECK(read.Hotspot == doctest::Approx(Radians(45)));
    CHECK(read.Falloff == doctest::Approx(Radians(50)));
    CHECK(read.Color.Index == 7);
    CHECK(read.Size == 256);
  }
  CHECK(issues.empty());
}
} // namespace DwgIO::Entities
