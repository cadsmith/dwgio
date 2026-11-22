/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/visualstyle.cpp
 *  @brief     Visual styles lifted from and fitted to each release's layout of properties.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/visualstyle.hpp"

#include "objects/shapes/visualstyle.hpp"
#include "support/fixtures.hpp"
#include "support/styles.hpp"

#include "dwgio/color.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
using Objects::VisualStyles::Property;
using Objects::VisualStyles::Setting;

auto StylesOf(const Drawing &drawing) -> std::vector<const Objects::VisualStyle *>
{
  std::vector<const Objects::VisualStyle *> styles;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *style = GetIf<Objects::VisualStyle>(&object))
    {
      styles.push_back(style);
    }
  }
  return styles;
}

/**
 *  The style of a name, which the drawing must have.
 */
auto FindStyle(const Drawing &drawing, std::string_view name) -> const Objects::VisualStyle &
{
  const std::vector<const Objects::VisualStyle *> styles = StylesOf(drawing);
  const auto found = std::ranges::find(styles, name, &Objects::VisualStyle::Name);
  REQUIRE(found != styles.end());
  return **found;
}

/**
 *  The 30 properties beyond R2010's.
 */
auto Beyond(const std::vector<Property> &full) -> std::vector<Property>
{
  return {full.begin() + 28, full.end()};
}
} // namespace

TEST_CASE("releases/visualstyle: every fixture's visual style fitted to its release matches AutoCAD's")
{
  for (const std::string_view fixture : {"visualstyles", "kinds", "material"})
  {
    for (const std::string_view suffix : FixtureSuffixes)
    {
      const std::string name = std::format("{}-{}.dwg", fixture, suffix);
      CAPTURE(name);
      const Drawing drawing = Load(ReadFixture(name)).value().Drawing;
      for (const Objects::VisualStyle *style : StylesOf(drawing))
      {
        CAPTURE(style->Name);
        const std::vector<Tag> kept = Roundtrip(drawing, style->Base).value_or(std::vector<Tag>{});
        CHECK(kept.empty() == (drawing.Release >= Version::R2013));
        const std::vector<Property> full = RaiseProperties(*style, drawing.Release, kept);
        REQUIRE(full.size() == 58);
        const StyleLayout fitted = FitProperties(full, kept, drawing.Release);
        CHECK(fitted.Properties == style->Properties);
        CHECK(fitted.Kept == kept);
      }
    }
  }
}

TEST_CASE("releases/visualstyle: a style missing its record takes AutoCAD's defaults and 2D wireframe its own")
{
  const Drawing older = Load(ReadFixture("kinds-r2007.dwg")).value().Drawing;
  const Drawing newer = Load(ReadFixture("kinds-r2018.dwg")).value().Drawing;
  for (const std::string_view name : {"2dWireframe", "Realistic", "Conceptual"})
  {
    CAPTURE(name);
    const std::vector<Property> full = RaiseProperties(FindStyle(older, name), Version::R2007, {});
    CHECK(Beyond(full) == Beyond(FindStyle(newer, name).Properties));
    for (std::size_t at = 0; at < 28; ++at)
    {
      CHECK(full[at].Flags == 1);
    }
  }
  // AutoCAD's own save of such a file, the break line symbol of an older AutoCAD, to R2010 makes the record so.
  const Drawing r2010 = Load(ReadFixture("kinds-r2010.dwg")).value().Drawing;
  const StyleLayout fitted =
      FitProperties(RaiseProperties(FindStyle(older, "Realistic"), Version::R2007, {}), {}, Version::R2010);
  CHECK(fitted.Kept == Roundtrip(r2010, FindStyle(r2010, "Realistic").Base));
}

TEST_CASE("releases/visualstyle: a record value of the wrong kind for its property stays at the default")
{
  const Drawing newer = Load(ReadFixture("kinds-r2018.dwg")).value().Drawing;
  const Objects::VisualStyle &style = FindStyle(newer, "Realistic");
  const std::vector<Property> defaults = RaiseProperties(style, Version::R2010, {});
  const auto marked = [](std::string name, Content value) -> std::vector<Tag>
  {
    return {Tag{.Code = 102, .Value = std::move(name)}, Tag{.Code = 90, .Value = std::move(value)}};
  };

  struct Odd
  {
    std::size_t At;
    std::vector<Tag> Kept;
  };

  for (const Odd &odd :
       {Odd{.At = 28, .Kept = marked("RTVSPost2010Prop28", std::string{"x"})},
        Odd{.At = 37, .Kept = marked("RTVSPost2010Prop37", 1.5)},
        Odd{.At = 38, .Kept = marked("RTVSPost2010Prop38", std::int64_t{1})},
        Odd{.At = 54, .Kept = marked("RTVSPost2010Prop54", std::int64_t{1})},
        Odd{.At = 41, .Kept = marked("RTVSPost2010Prop41ColorIndex", std::int64_t{3})},
        Odd{.At = 44, .Kept = marked("RTVSPost2010Prop44ColorRGB", std::int64_t{3})},
        Odd{.At = 51, .Kept = {Tag{.Code = 102, .Value = std::string{"RTVSPost2010Prop51"}}}},
        Odd{.At = 0, .Kept = marked("RTVSPropertyOp0", std::string{"x"})},
        Odd{.At = 37,
            .Kept = {Tag{.Code = 102, .Value = std::int64_t{37}}, Tag{.Code = 90, .Value = std::int64_t{7}}}}})
  {
    CAPTURE(odd.At);
    const std::vector<Property> full = RaiseProperties(style, Version::R2007, odd.Kept);
    CHECK(full[odd.At] == RaiseProperties(style, Version::R2007, {})[odd.At]);
  }
  CHECK(defaults.size() == 58);
}

TEST_CASE("releases/visualstyle: a style of the wrong size or kinds lifts and fits without failing")
{
  // No properties at all: zeros of their slots, then what R2007's layout makes of them.
  const Objects::VisualStyle empty{
      .Base = {}, .Name = "Empty", .Kind = 0, .Version = 2, .Internal = false, .Properties = {}};
  for (const Version release : {Version::R2000, Version::R2010, Version::R2018})
  {
    const std::vector<Property> full = RaiseProperties(empty, release, {});
    REQUIRE(full.size() == 58);
    CHECK(full.front().Value == Objects::Shapes::MakeZero(Objects::Shapes::Typed.front()));
    CHECK(FitProperties(full, {}, Version::R2007).Properties.size() == 30);
  }
  // Values of other kinds than their slots': a real face modifier, a whole opacity, a real line width below R2010;
  // a real lighting quality and a whole line width fitted to it.
  Objects::VisualStyle odd = empty;
  for (const Objects::Shapes::Slot slot : Objects::Shapes::LayoutOf(Version::R2007))
  {
    odd.Properties.push_back(Property{.Code = slot.Code, .Value = Objects::Shapes::MakeZero(slot), .Flags = 0});
  }
  const auto set = [&odd](int code, Setting value) -> void
  {
    for (Property &property : odd.Properties)
    {
      property.Value = property.Code == code ? value : property.Value;
    }
  };
  set(90, 1.5);
  set(40, std::int32_t{3});
  set(44, 2.5);
  const std::vector<Property> full = RaiseProperties(odd, Version::R2007, {});
  CHECK(full[4].Value == Setting{std::int32_t{3}});
  CHECK(full[26].Value == Setting{2.5});
  std::vector<Property> other = full;
  other[1].Value = 3.5;
  other[26].Value = std::int32_t{4};
  const StyleLayout fitted = FitProperties(other, {}, Version::R2004);
  CHECK(fitted.Properties[1].Value == Setting{3.5});
  CHECK(fitted.Properties[27].Value == Setting{std::int32_t{4}});
  // An R2013 style of fewer properties keeps the defaults of the rest.
  Objects::VisualStyle shorter = empty;
  shorter.Properties.push_back(Property{.Code = 90, .Value = std::int32_t{7}, .Flags = 2});
  const std::vector<Property> lifted = RaiseProperties(shorter, Version::R2013, {});
  CHECK(lifted.front() == Property{.Code = 90, .Value = std::int32_t{7}, .Flags = 2});
  CHECK(Beyond(lifted) == Beyond(RaiseProperties(empty, Version::R2013, {})));
}
} // namespace DwgIO::Releases
