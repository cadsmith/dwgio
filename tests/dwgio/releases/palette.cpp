/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/palette.cpp
 *  @brief     The nearest of AutoCAD's color indices to a true color.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/palette.hpp"

#include "dwgio/color.hpp"

#include <doctest/doctest.h>

#include <cstdint>

namespace DwgIO::Releases
{
TEST_CASE("releases/palette: a true color takes the nearest AutoCAD index and the first of equally near ones")
{
  // The indexes AutoCAD gives these true colors when it saves them to R2000.
  CHECK(NearestIndexOf(0xFF0000) == 1); // Red is 1 and 10: the first.
  CHECK(NearestIndexOf(0xFFFFFF) == 7);
  CHECK(NearestIndexOf(0x808080) == 8);
  CHECK(NearestIndexOf(0x787878) == 8);
  CHECK(NearestIndexOf(0x808081) == 8);
  CHECK(NearestIndexOf(0xC0C0C0) == 9);
  CHECK(NearestIndexOf(0x000000) == 18);
  CHECK(NearestIndexOf(0x0000FF) == 5);
  CHECK(NearestIndexOf(0x123456) == 148);
  CHECK(NearestIndexOf(0x654321) == 49);
  CHECK(NearestIndexOf(0xA5A5A5) == 253);
  CHECK(NearestIndexOf(0x505050) == 251);
  CHECK(NearestIndexOf(0x333333) == 250);
  CHECK(NearestIndexOf(0xCC0000) == 12);
  CHECK(NearestIndexOf(0xFF9F7F) == 21);
  // One of each hue's six turns: green up, red down, blue up, green down, red up, blue down.
  CHECK(NearestIndexOf(0xFF3F00) == 20);
  CHECK(NearestIndexOf(0xBFFF00) == 60);
  CHECK(NearestIndexOf(0x00FF3F) == 100);
  CHECK(NearestIndexOf(0x00BFFF) == 140);
  CHECK(NearestIndexOf(0x3F00FF) == 180);
  CHECK(NearestIndexOf(0xFF00BF) == 220);
}

TEST_CASE("releases/palette: a color saved as R2000 matches AutoCAD's index and comes back on upgrading")
{
  CHECK(LowerColor(Color{.Index = 3}) == Color{.Index = 3});
  CHECK(LowerColor(Color{.Index = 0, .Rgb = Unmethodical}) == Color{.Index = UnmethodicalIndex});
  const Color named{
      .Index = 0, .Rgb = static_cast<std::int32_t>(0xC2FFFFFFU), .Names = ColorNames{.Name = "white", .Book = "book"}};
  CHECK(LowerColor(named) == Color{.Index = 7});
  CHECK(RaiseColor(Color{.Index = UnmethodicalIndex}) == Color{.Index = 0, .Rgb = Unmethodical});
  CHECK(RaiseColor(Color{.Index = 7}) == Color{.Index = 7});
}
} // namespace DwgIO::Releases
