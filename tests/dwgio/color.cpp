/***********************************************************************************************************************
 *  @file      tests/dwgio/color.cpp
 *  @brief     A color's method, true color and transparency, against the colors AutoCAD wrote.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/color.hpp"

#include "support/fixtures.hpp"

#include "dwgio/entities/line.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  The lines of the transparent fixture, from the bottom up: alpha 102, transparency ByBlock, true color 0x112233 and
 *  color ByBlock.
 */
auto Lines() -> std::vector<Color>
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("transparent-r2018.dwg"));
  REQUIRE(loaded);
  std::vector<Color> colors(4);
  for (const Object &object : loaded->Drawing.Objects)
  {
    if (const auto *line = GetIf<Entities::Line>(&object); line != nullptr)
    {
      colors.at(static_cast<std::size_t>(line->Start.Y)) = line->Common.Color;
    }
  }
  return colors;
}
} // namespace

TEST_CASE("color: the fixture's colors carry their method true color and transparency")
{
  const std::vector<Color> colors = Lines();
  CHECK(MethodOf(colors[0]) == ColorMethod::ByLayer);
  CHECK(TransparencyMethodOf(colors[0]) == TransparencyMethod::Alpha);
  CHECK(AlphaOf(colors[0]) == 102);
  CHECK(TransparencyMethodOf(colors[1]) == TransparencyMethod::ByBlock);
  CHECK_FALSE(AlphaOf(colors[1]));
  CHECK(MethodOf(colors[2]) == ColorMethod::Rgb);
  CHECK(RgbOf(colors[2]) == 0x112233U);
  CHECK_FALSE(TransparencyMethodOf(colors[2]));
  CHECK_FALSE(AlphaOf(colors[2]));
  CHECK(MethodOf(colors[3]) == ColorMethod::ByBlock);
  CHECK_FALSE(RgbOf(colors[3]));
}

TEST_CASE("color: an index alone gives its method and a true color of another method has no RGB")
{
  CHECK(MethodOf(Colors::ByLayer) == ColorMethod::ByLayer);
  CHECK(MethodOf(Colors::ByBlock) == ColorMethod::ByBlock);
  CHECK(MethodOf(Color{.Index = 1}) == ColorMethod::Index);
  const Color foreground{.Index = 7, .Rgb = static_cast<int>(0xC5000000U)};
  CHECK(MethodOf(foreground) == ColorMethod::Foreground);
  CHECK_FALSE(RgbOf(foreground));
  CHECK(TransparencyMethodOf(Color{.Index = 1, .Rgb = {}, .Transparency = 0}) == TransparencyMethod::ByLayer);
}
} // namespace DwgIO
