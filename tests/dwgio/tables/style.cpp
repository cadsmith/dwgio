/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/style.cpp
 *  @brief     The STYLE against its DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/style.hpp"

#include "support/fixtures.hpp"
#include "support/twin.hpp"

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

namespace DwgIO::Tables
{
namespace
{
auto Styles(const Drawing &drawing) -> std::vector<const Style *>
{
  std::vector<const Style *> styles;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *style = GetIf<Style>(&object))
    {
      styles.push_back(style);
    }
  }
  return styles;
}

auto FindByName(const std::vector<const Style *> &styles, std::string_view name) -> const Style &
{
  const auto found = std::ranges::find(styles, name, [](const Style *one) { return one->Record.Name; });
  REQUIRE(found != styles.end());
  return **found;
}
} // namespace

TEST_CASE("tables/style: the fixture's text styles carry their fonts and proportions")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const bool modern = loaded->Drawing.Release >= Version::R2000;
    const std::vector<const Style *> styles = Styles(loaded->Drawing);
    REQUIRE(styles.size() == 5);
    const Style &notes = FindByName(styles, modern ? "Notes" : "NOTES");
    CHECK(notes.Font == "Arial.ttf");
    CHECK(notes.Width == doctest::Approx(0.8));
    CHECK(notes.Oblique == doctest::Approx(15.0 * std::numbers::pi / 180.0));
    CHECK(notes.Height == 0.0);
    CHECK_FALSE(notes.Shape);
    CHECK_FALSE(notes.Vertical);
    const Style &mono = FindByName(styles, modern ? "Mono" : "MONO");
    CHECK(mono.Font == "txt.shx");
    CHECK(mono.Height == doctest::Approx(2.5));
    const Style &shapes = FindByName(styles, "");
    CHECK(shapes.Shape);
    CHECK(shapes.Font == "ltypeshp.shx");
    CHECK(TypeOf(Object{shapes}) == "STYLE");
    for (const Style *style : styles)
    {
      CAPTURE(style->Record.Name);
      const std::optional<std::size_t> opened = FindOpeningNoCase(twin, "STYLE", style->Record.Name);
      REQUIRE(opened);
      CHECK(style->Record.Base.Self == Self(twin, *opened));
      const int flags = std::stoi(GroupOf(twin, *opened, 70));
      CHECK(style->Shape == ((flags & 1) != 0));
      CHECK(style->Vertical == ((flags & 4) != 0));
      CHECK(style->Height == doctest::Approx(ReadReal(twin, *opened, 40)));
      CHECK(style->Width == doctest::Approx(ReadReal(twin, *opened, 41)));
      CHECK(style->Oblique == doctest::Approx(ReadReal(twin, *opened, 50) * std::numbers::pi / 180.0));
      CHECK(style->Generation == std::stoi(GroupOf(twin, *opened, 71)));
      CHECK(style->LastHeight == doctest::Approx(ReadReal(twin, *opened, 42)));
      CHECK(style->Font == GroupOf(twin, *opened, 3));
      CHECK(style->Bigfont == GroupOf(twin, *opened, 4));
      CHECK(style->Record.Source == Handle{});
    }
  }
}
} // namespace DwgIO::Tables
