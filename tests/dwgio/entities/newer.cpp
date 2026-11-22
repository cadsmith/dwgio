/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/newer.cpp
 *  @brief     Every typed entity translated between R2000 and R2004, against AutoCAD's own fixture.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/shape.hpp"
#include "objects/write.hpp"
#include "releases/object.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/registered.hpp"

#include "dwgio/entities/viewport.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/version.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <cstddef>
#include <expected>
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
struct Counts
{
  std::size_t Compared{};
  std::size_t Aside{};
  std::size_t Regenerated{};
};

auto FixtureOf(std::string_view name, Version release) -> std::string
{
  return std::string{name} + (release == Version::R2000 ? "-r2000.dwg" : "-r2004.dwg");
}

/**
 *  Whether two objects carry extended data of the same shape: as many applications, each with as many groups.
 */
auto SameGroups(const Object &one, const Object &other) -> bool
{
  const std::vector<Xdata> &first = BaseOf(one).Extended;
  const std::vector<Xdata> &second = BaseOf(other).Extended;
  return first.size() == second.size() &&
         std::ranges::equal(first, second, [](const Xdata &left, const Xdata &right) -> bool
                            { return left.Tags.size() == right.Tags.size(); });
}

/**
 *  Whether an entity's twin says what no translation can: a viewport lowered to R2000 names no viewport entity header,
 *  as R2004 keeps none, while AutoCAD's R2000 file names the one it made at its save.
 */
auto Unsayable(const Object &object, const Object &same, Version to) -> bool
{
  const auto *viewport = GetIf<Viewport>(&same);
  return to == Version::R2000 && viewport != nullptr && viewport->Header && Holds<Viewport>(object);
}

/**
 *  Every typed entity of the fixture in one release, translated to the other, written as it and read back, is what the
 *  fixture of the other release holds under the same handle; an object read on its own has no header to take its owner
 *  from, so the owner is set as Load sets it.
 */
auto Check(std::string_view name, Version from, Version to, const Counts &expected) -> std::size_t
{
  const std::expected<Loaded, Failure> source = Load(ReadFixture(FixtureOf(name, from)));
  const std::expected<Loaded, Failure> target = Load(ReadFixture(FixtureOf(name, to)));
  REQUIRE(source);
  REQUIRE(target);
  std::vector<Issue> issues;
  const Fields::Context with{
      .Release = to, .Text = &Objects::Latin, .Issues = &issues, .Classes = &target->Drawing.Classes};
  const Objects::FileContext reading{
      .Release = to, .Text = &Objects::Latin, .Classes = target->Drawing.Classes, .Issues = &issues};
  Counts counts;
  const std::vector<Handle> regenerated = CollectDimensionBlocks(source->Drawing);
  for (const Object &object : source->Drawing.Objects)
  {
    if (CommonOf(object) == nullptr || Holds<Unknown>(object))
    {
      continue;
    }
    const Handle self = BaseOf(object).Self;
    CAPTURE(HexOf(self));
    CAPTURE(TypeOf(object));
    if (std::ranges::contains(regenerated, BaseOf(object).Owner))
    {
      ++counts.Regenerated;
      continue;
    }
    const Object *same = Find(target->Drawing, self);
    // AutoCAD adds or drops extended data of its own at each save, so an entity whose groups differ is set aside.
    if (same == nullptr || same->index() != object.index() || !SameGroups(object, *same) ||
        Unsayable(object, *same, to))
    {
      ++counts.Aside;
      continue;
    }
    const Object translated = Releases::ConvertObject(object, source->Drawing, to).Object;
    std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(translated, with), 0, reading);
    REQUIRE(back);
    Visit([&object](auto &kind) -> void { Objects::BaseOfKind(kind).Owner = BaseOf(object).Owner; }, *back);
    Reregister(*back, source->Drawing, *same, target->Drawing);
    CHECK(*back == *same);
    ++counts.Compared;
  }
  CHECK(counts.Compared == expected.Compared);
  CHECK(counts.Aside == expected.Aside);
  CHECK(counts.Regenerated == expected.Regenerated);
  CHECK(issues.empty());
  return counts.Compared;
}
} // namespace

TEST_CASE("entities/newer: every R2000 fixture entity converted to R2004 matches the R2004 fixture")
{
  std::size_t compared = 0;
  for (const auto &[name, counts] :
       {std::pair{std::string_view{"entities"}, Counts{.Compared = 38, .Aside = 1}},
        std::pair{std::string_view{"curves"}, Counts{.Compared = 190, .Aside = 1, .Regenerated = 92}},
        std::pair{std::string_view{"objects"}, Counts{.Compared = 17}},
        std::pair{std::string_view{"empty"}, Counts{.Compared = 6}}})
  {
    CAPTURE(name);
    compared += Check(name, Version::R2000, Version::R2004, counts);
  }
  CHECK(compared >= 30);
}

TEST_CASE("entities/newer: every R2004 fixture entity converted to R2000 matches the R2000 fixture")
{
  std::size_t compared = 0;
  for (const auto &[name, counts] :
       {std::pair{std::string_view{"entities"}, Counts{.Compared = 38, .Aside = 1}},
        std::pair{std::string_view{"curves"}, Counts{.Compared = 185, .Aside = 6, .Regenerated = 92}},
        std::pair{std::string_view{"objects"}, Counts{.Compared = 13, .Aside = 4}},
        std::pair{std::string_view{"empty"}, Counts{.Compared = 6}}})
  {
    CAPTURE(name);
    compared += Check(name, Version::R2004, Version::R2000, counts);
  }
  CHECK(compared >= 30);
}
} // namespace DwgIO::Entities
