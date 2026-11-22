/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/newer.cpp
 *  @brief     Every typed object translated between R2000 and R2004, against AutoCAD's own fixture.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/shape.hpp"
#include "objects/write.hpp"
#include "releases/object.hpp"
#include "support/comparable.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
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
struct Counts
{
  std::size_t Compared{};
  std::size_t Aside{};
};

auto FixtureOf(std::string_view name, Version release) -> std::string
{
  return std::string{name} + (release == Version::R2000 ? "-r2000.dwg" : "-r2004.dwg");
}

/**
 *  A block record's entities as a set: the oracle asks that the same entities are owned, not AutoCAD's order.
 */
void SortEntities(Object &object)
{
  if (auto *record = GetIf<Tables::BlockRecord>(&object))
  {
    std::ranges::sort(record->Entities, {}, &Handle::Value);
  }
}

/**
 *  Every typed object of the fixture in one release that is no entity, translated to the other, written as it and read
 *  back, is what the fixture of the other release holds under the same handle, but for what AutoCAD remakes at each
 *  save; the owner is set as Load sets it.
 */
auto Check(std::string_view name, Version from, Version to, const Counts &expected) -> std::size_t
{
  const std::expected<Loaded, Failure> source = Load(ReadFixture(FixtureOf(name, from)));
  const std::expected<Loaded, Failure> target = Load(ReadFixture(FixtureOf(name, to)));
  REQUIRE(source);
  REQUIRE(target);
  std::vector<Issue> issues;
  const Fields::Context with{.Release = to, .Text = &Latin, .Issues = &issues, .Classes = &target->Drawing.Classes};
  const Objects::FileContext reading{
      .Release = to, .Text = &Latin, .Classes = target->Drawing.Classes, .Issues = &issues};
  Counts counts;
  for (const Object &object : source->Drawing.Objects)
  {
    if (CommonOf(object) != nullptr || Holds<Unknown>(object) || Holds<Tables::Control>(object))
    {
      continue;
    }
    const Handle self = BaseOf(object).Self;
    CAPTURE(HexOf(self));
    CAPTURE(TypeOf(object));
    const Object *same = Find(target->Drawing, self);
    if (same == nullptr || same->index() != object.index() || !Comparable(object, *same))
    {
      ++counts.Aside;
      continue;
    }
    const Object translated = Releases::ConvertObject(object, source->Drawing, to).Object;
    std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(translated, with), 0, reading);
    REQUIRE(back);
    Visit([&object](auto &kind) -> void { Objects::BaseOfKind(kind).Owner = BaseOf(object).Owner; }, *back);
    SortEntities(*back);
    Object twin = *same;
    SortEntities(twin);
    CHECK(*back == twin);
    ++counts.Compared;
  }
  CHECK(counts.Compared == expected.Compared);
  CHECK(counts.Aside == expected.Aside);
  CHECK(issues.empty());
  return counts.Compared;
}
} // namespace

TEST_CASE("objects/newer: every typed object of the R2000 fixtures converted to R2004 matches the R2004 fixture")
{
  std::size_t compared = 0;
  for (const auto &[name, counts] : {std::pair{std::string_view{"objects"}, Counts{.Compared = 102, .Aside = 130}},
                                     std::pair{std::string_view{"empty"}, Counts{.Compared = 83, .Aside = 114}},
                                     std::pair{std::string_view{"tables"}, Counts{.Compared = 104, .Aside = 116}}})
  {
    CAPTURE(name);
    compared += Check(name, Version::R2000, Version::R2004, counts);
  }
  CHECK(compared >= 60);
}

TEST_CASE("objects/newer: every typed object of the R2004 fixtures converted to R2000 matches the R2000 fixture")
{
  std::size_t compared = 0;
  for (const auto &[name, counts] : {std::pair{std::string_view{"objects"}, Counts{.Compared = 102, .Aside = 116}},
                                     std::pair{std::string_view{"empty"}, Counts{.Compared = 83, .Aside = 107}},
                                     std::pair{std::string_view{"tables"}, Counts{.Compared = 104, .Aside = 109}}})
  {
    CAPTURE(name);
    compared += Check(name, Version::R2004, Version::R2000, counts);
  }
  CHECK(compared >= 60);
}
} // namespace DwgIO::Objects
