/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/older.cpp
 *  @brief     Every typed object of an R14 fixture written as R2000, against AutoCAD's own fixture.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/shape.hpp"
#include "objects/write.hpp"
#include "support/comparable.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/dictionaryvar.hpp"
#include "dwgio/objects/layout.hpp"

#include <doctest/doctest.h>

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
  std::size_t Regenerated{};
};

/**
 *  Every typed object of the R14 fixture that is no entity, written as R2000 and read back, is what the R2000 fixture
 *  holds under the same handle, but for the objects AutoCAD remakes at each save and the dictionaries it fills then; an
 *  object read on its own has no header to take its owner from, so the owner is set as Load sets it.
 */
void Check(std::string_view name, const Counts &expected)
{
  const std::expected<Loaded, Failure> older = Load(ReadFixture(std::string{name} + "-r14.dwg"));
  const std::expected<Loaded, Failure> newer = Load(ReadFixture(std::string{name} + "-r2000.dwg"));
  REQUIRE(older);
  REQUIRE(newer);
  std::vector<Issue> issues;
  const Fields::Context with{
      .Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Classes = &newer->Drawing.Classes};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Latin, .Classes = newer->Drawing.Classes, .Issues = &issues};
  Counts counts;
  for (const Object &object : older->Drawing.Objects)
  {
    if (CommonOf(object) != nullptr || Holds<Unknown>(object) || Holds<Tables::Control>(object))
    {
      continue;
    }
    if (TypeOf(object).starts_with("APPID") || TypeOf(object).ends_with("STYLE") || TypeOf(object) == "LAYER" ||
        TypeOf(object) == "LTYPE" || TypeOf(object) == "VIEW" || TypeOf(object) == "UCS" || TypeOf(object) == "VPORT" ||
        TypeOf(object) == "BLOCK_HEADER")
    {
      continue;
    }
    const Handle self = BaseOf(object).Self;
    CAPTURE(HexOf(self));
    CAPTURE(TypeOf(object));
    const Object *same = FindObject(newer->Drawing, self);
    if (same == nullptr)
    {
      ++counts.Regenerated;
      continue;
    }
    if (!Comparable(object, *same))
    {
      ++counts.Aside;
      continue;
    }
    std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(object, with), 0, reading);
    REQUIRE(back);
    Visit([&object](auto &kind) -> void { Objects::BaseOfKind(kind).Owner = BaseOf(object).Owner; }, *back);
    CHECK(*back == *same);
    ++counts.Compared;
  }
  CHECK(counts.Compared == expected.Compared);
  CHECK(counts.Aside == expected.Aside);
  CHECK(counts.Regenerated == expected.Regenerated);
  CHECK(issues.empty());
}
} // namespace

TEST_CASE("objects/older: every typed object of the R14 fixtures written as R2000 matches the R2000 fixture")
{
  for (const auto &[name, counts] :
       {std::pair{std::string_view{"objects"}, Counts{.Compared = 68, .Aside = 25, .Regenerated = 178}},
        std::pair{std::string_view{"empty"}, Counts{.Compared = 47, .Aside = 20, .Regenerated = 167}}})
  {
    CAPTURE(name);
    Check(name, counts);
  }
}
} // namespace DwgIO::Objects
