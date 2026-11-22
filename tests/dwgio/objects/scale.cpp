/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/scale.cpp
 *  @brief     The SCALE against its DXF twin, and written back under its class.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/scale.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"

#include "dwgio/class.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/scale: the fixture's scales match the DXF twin with three added and one deleted")
{
  const std::vector<Pair> twin = ReadTwin("objects-r2000.dxf");
  for (const std::string_view name : {"objects-r2000.dwg", "objects-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    std::vector<std::string> names;
    std::size_t unit = 0;
    for (const Object &object : drawing.Objects)
    {
      const auto *scale = GetIf<Scale>(&object);
      if (scale == nullptr)
      {
        continue;
      }
      names.push_back(scale->Name);
      CAPTURE(HexOf(scale->Base.Self));
      CHECK(TypeOf(object) == "SCALE");
      const std::optional<std::size_t> opened = FindHandled(twin, scale->Base.Self);
      REQUIRE(opened);
      CHECK(scale->Flags == ReadWhole(twin, *opened, 70));
      CHECK(scale->Name == GroupOf(twin, *opened, 300));
      CHECK(scale->Paper == doctest::Approx(ReadReal(twin, *opened, 140)));
      CHECK(scale->Drawing == doctest::Approx(ReadReal(twin, *opened, 141)));
      CHECK(scale->Unit == (ReadWhole(twin, *opened, 290) == 1));
      unit += scale->Unit ? 1U : 0U;
    }
    CHECK(names.size() == 19);
    CHECK(unit == 1);
    CHECK(std::ranges::contains(names, "3:1"));
    CHECK(std::ranges::contains(names, "1:250"));
    CHECK_FALSE(std::ranges::contains(names, "1:7"));
  }
}

TEST_CASE("objects/scale: a scale round-trips under its class and is not written without it")
{
  std::vector<Issue> issues;
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "SCALE"}};
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{.Release = Version::R2000, .Text = &Latin, .Classes = classes, .Issues = &issues};
  Scale scale;
  scale.Base.Self = Handle{0x100};
  scale.Base.Owner = Handle{0x10C};
  scale.Name = "1:7";
  scale.Drawing = 7;
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{scale}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{scale});
  CHECK(TypeOf(*back) == "SCALE");
  CHECK(issues.empty());
  const Fields::Context without{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  CHECK(Objects::WriteObject(Object{scale}, without).empty());
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "class SCALE not declared");
  const Fields::Context silent{.Release = Version::R2000, .Text = &Latin};
  CHECK(Objects::WriteObject(Object{scale}, silent).empty());
  CHECK(issues.size() == 1);
  // Classes declared, none of them SCALE's.
  const std::vector<Class> others{Class{.Number = 500, .DxfName = "LAYOUT"}};
  const Fields::Context elsewhere{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Classes = &others};
  CHECK(Objects::WriteObject(Object{scale}, elsewhere).empty());
  REQUIRE(issues.size() == 2);
  CHECK(issues.back().What == "class SCALE not declared");
}
} // namespace DwgIO::Objects
