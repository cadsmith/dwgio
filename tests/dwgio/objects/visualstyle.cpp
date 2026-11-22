/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/visualstyle.cpp
 *  @brief     The VISUALSTYLE against its DXF twin in each release's layout, and properties of the wrong kind.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/visualstyle.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/read.hpp"
#include "objects/shapes/visualstyle.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/class.hpp"
#include "dwgio/color.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
using VisualStyles::Property;

/**
 *  One property as the twin gives it: its group, the true color that may follow, the flags that may follow.
 */
struct Given
{
  int Code{};
  std::string Value;
  std::optional<int> Rgb;
  std::optional<int> Flags;
};

/**
 *  The color the twin gives: no method as 257, a true color by its 42x group, else the index.
 */
void CheckColor(const Color &color, const Given &given)
{
  constexpr unsigned NoMethod = 0xC8;
  if (color.Rgb && (static_cast<unsigned>(*color.Rgb) >> 24U) == NoMethod)
  {
    CHECK(std::stoi(given.Value) == 257);
  }
  else if (color.Rgb)
  {
    REQUIRE(given.Rgb);
    CHECK((*color.Rgb & 0xFFFFFF) == *given.Rgb);
  }
  else
  {
    CHECK(color.Index == std::stoi(given.Value));
  }
}

void CheckValue(const Property &property, const Given &given)
{
  CAPTURE(given.Code);
  CAPTURE(given.Value);
  if (const auto *whole = std::get_if<std::int32_t>(&property.Value))
  {
    CHECK(*whole == static_cast<std::int32_t>(std::stod(given.Value))); // R2000 gives a whole 44 as a real.
  }
  else if (const auto *real = std::get_if<double>(&property.Value))
  {
    CHECK(std::abs(*real) == doctest::Approx(std::abs(std::stod(given.Value))));
  }
  else if (const auto *flag = std::get_if<bool>(&property.Value))
  {
    CHECK(*flag == (std::stoi(given.Value) == 1));
  }
  else if (const auto *color = std::get_if<Color>(&property.Value))
  {
    CheckColor(*color, given);
  }
  else
  {
    CHECK(std::get<std::string>(property.Value) == given.Value);
  }
}

/**
 *  The twin's property groups after the style's own: from the first group of a property to the extended data.
 */
auto Properties(const std::vector<Pair> &twin, std::size_t at, Version release) -> std::vector<Given>
{
  std::size_t from = at + 1;
  while (twin[from].Code != 70)
  {
    ++from;
  }
  ++from; // Past the kind.
  if (release >= Version::R2013)
  {
    from += 3; // The version, the internal flag and the count.
  }
  else if (release >= Version::R2010)
  {
    from += 2;
  }
  std::vector<Given> given;
  for (std::size_t one = from; twin[one].Code != 0 && twin[one].Code < 1000; ++one)
  {
    const int code = twin[one].Code;
    if (code >= 420 && code <= 431 && !given.empty())
    {
      given.back().Rgb = std::stoi(twin[one].Value);
    }
    else if (code == 176 && !given.empty())
    {
      given.back().Flags = std::stoi(twin[one].Value);
    }
    else
    {
      given.push_back(Given{.Code = code, .Value = twin[one].Value, .Rgb = {}, .Flags = {}});
    }
  }
  return given;
}

/**
 *  R14 to R2007 keep their properties in another order than DXF's, which gives a color and a real more: each is matched
 *  by its code.
 */
void CheckOlder(const VisualStyle &style, const std::vector<Given> &given)
{
  for (const Property &property : style.Properties)
  {
    const auto found = std::ranges::find(given, property.Code, &Given::Code);
    REQUIRE(found != given.end());
    if (property.Code == 72 && found->Value == "3") // The file keeps 2 where DXF and R2010 say 3.
    {
      CHECK(property.Value == VisualStyles::Setting{std::int32_t{2}});
      continue;
    }
    CheckValue(property, *found);
    CHECK(property.Flags == 0);
  }
}

void CheckStyle(const VisualStyle &style, const std::vector<Pair> &twin, std::size_t at, Version release)
{
  CHECK(style.Name == GroupOf(twin, at, 2));
  CHECK(style.Kind == ReadWhole(twin, at, 70));
  CHECK(style.Internal == (ReadWhole(twin, at, 291) == 1));
  CHECK(style.Version == 2); // R2013's and R2018's DXF says 3.
  CHECK(style.Properties.size() == Shapes::LayoutOf(release).size());
  const std::vector<Given> given = Properties(twin, at, release);
  if (release < Version::R2010)
  {
    CheckOlder(style, given);
    return;
  }
  REQUIRE(given.size() == style.Properties.size());
  for (std::size_t one = 0; one < given.size(); ++one)
  {
    CHECK(style.Properties[one].Code == given[one].Code);
    CheckValue(style.Properties[one], given[one]);
    CHECK(style.Properties[one].Flags == given[one].Flags.value_or(-1));
  }
}

auto Written(const VisualStyle &style, Version release, std::vector<Issue> &issues) -> std::optional<Object>
{
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "VISUALSTYLE"}};
  const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = classes, .Issues = &issues};
  return ReadObject(WriteObject(Object{style}, with), 0, reading);
}
} // namespace

TEST_CASE("objects/visualstyle: every visual style matches the DXF twin in its release's layout")
{
  for (const auto &[fixture, styles] : {std::pair{std::string_view{"kinds"}, std::size_t{28}},
                                        std::pair{std::string_view{"visualstyles"}, std::size_t{26}}})
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
      std::size_t seen = 0;
      for (const Object &object : loaded->Drawing.Objects)
      {
        const auto *style = GetIf<VisualStyle>(&object);
        if (style == nullptr)
        {
          continue;
        }
        ++seen;
        CAPTURE(HexOf(style->Base.Self));
        CHECK(TypeOf(object) == "VISUALSTYLE");
        const std::optional<std::size_t> at = FindHandled(twin, style->Base.Self);
        REQUIRE(at);
        CheckStyle(*style, twin, *at, loaded->Drawing.Release);
      }
      CHECK(seen == styles);
    }
  }
}

TEST_CASE("objects/visualstyle: a style round-trips and one of the wrong size or kinds is written to its layout")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("kinds-r2018.dwg"));
  REQUIRE(loaded);
  VisualStyle style;
  for (const Object &object : loaded->Drawing.Objects)
  {
    if (const auto *one = GetIf<VisualStyle>(&object); one != nullptr && one->Name == "2dWireframe")
    {
      style = *one;
    }
  }
  REQUIRE(style.Properties.size() == 58);
  std::vector<Issue> issues;
  std::optional<Object> back = Written(style, Version::R2018, issues);
  REQUIRE(back);
  CHECK(*back == Object{style});
  CHECK(issues.empty());

  // A short where a text belongs, and a flag value past a short: the text written empty, the short clamped.
  VisualStyle odd = style;
  odd.Properties[54].Value = std::int32_t{3};
  odd.Properties[0].Flags = 70000;
  back = Written(odd, Version::R2018, issues);
  REQUIRE(back);
  CHECK(std::get<std::string>(Get<VisualStyle>(*back).Properties[54].Value).empty());
  CHECK(Get<VisualStyle>(*back).Properties[0].Flags == 32767);
  REQUIRE(issues.size() == 2);
  CHECK(issues[0].What == "short 70000 out of range, 32767 written");
  CHECK(issues[1].What == "visual style property 1 of another kind, written as zero");

  // Every kind of slot given a value of the wrong kind, and a style of no properties.
  issues.clear();
  VisualStyle wrong;
  wrong.Base.Self = Handle{0x40};
  for (const Version release : {Version::R2000, Version::R2010, Version::R2018})
  {
    for (const Shapes::Slot slot : Shapes::LayoutOf(release))
    {
      wrong.Properties.push_back(Property{.Code = slot.Code, .Value = std::string{"x"}, .Flags = 0});
    }
    wrong.Properties.front().Value = 1.5; // A real where a short belongs.
    back = Written(wrong, release, issues);
    REQUIRE(back);
    const std::vector<Property> &read = Get<VisualStyle>(*back).Properties;
    const auto layout = Shapes::LayoutOf(release);
    REQUIRE(read.size() == layout.size());
    for (std::size_t at = 0; at < read.size(); ++at)
    {
      const bool text = layout[at].As == Shapes::Stored::Text;
      CHECK(read[at].Value == (text ? VisualStyles::Setting{std::string{"x"}} : Shapes::MakeZero(layout[at])));
    }
    wrong.Properties.clear();
  }
  CHECK_FALSE(issues.empty());
  issues.clear();
  back = Written(VisualStyle{.Base = {.Self = Handle{0x41}}}, Version::R2007, issues);
  REQUIRE(back);
  CHECK(Get<VisualStyle>(*back).Properties.size() == 30);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "visual style of 0 properties written with the 30 of its layout");
  // R2013's layout ends with a text among its slots, written empty for a style that lacks it.
  back = Written(VisualStyle{.Base = {.Self = Handle{0x42}}}, Version::R2018, issues);
  REQUIRE(back);
  CHECK(Get<VisualStyle>(*back).Properties[54].Value == VisualStyles::Setting{std::string{}});
}

TEST_CASE("objects/visualstyle: truncated properties stop the read")
{
  std::vector<Issue> issues;
  Bits::Writer data;
  data.BS(0);    // An empty name,
  data.BS(4);    // the kind,
  data.BS(2);    // R2010's version
  data.B(false); // and internal bit, then one property of the 28 and its flags.
  data.BS(0);
  data.BS(1);
  const std::vector<std::byte> bytes = data.Take();
  Bits::Reader in{bytes};
  Fields::Input reading{.In = &in,
                        .With = Fields::Context{.Release = Version::R2010, .Text = &Latin, .Issues = &issues}};
  VisualStyle style;
  CHECK_FALSE(Fields::Read(Shape<VisualStyle>::Layout(), style, reading));
  CHECK(style.Properties.size() < 28);
}

} // namespace DwgIO::Objects
