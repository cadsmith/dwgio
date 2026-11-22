/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/older.cpp
 *  @brief     Every typed entity of an R14 fixture written as R2000, against AutoCAD's own fixture.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/shape.hpp"
#include "objects/write.hpp"
#include "releases/object.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/registered.hpp"

#include "dwgio/entities/block.hpp"
#include "dwgio/entities/dimension.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
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
/**
 *  What an R14 file cannot say is left aside before the comparison.
 *
 *  Left aside: AutoCAD's null plot style on a SEQEND; a block's name, which R14 keeps in upper case; extended data R14
 *  keeps on an entity, a leader's, where R2000 keeps none; a viewport's clip entity and UCS, which R14's view of it in
 *  extended data has not. The lineweight and the ByBlock and Continuous linetypes R14 keeps otherwise are translated
 *  first, as a save to R2000 translates them. A dimension's R2000 text fields and measurement are taken from the R2000
 *  twin before the rest is compared. An object read on its own has no header to take a space's owner from, so the owner
 *  is set as Load sets it.
 */
namespace
{
struct Counts
{
  std::size_t Compared{};
  std::size_t Aside{};
  std::size_t Regenerated{};
};

/**
 *  Writes every typed entity of the R14 fixture of that name as R2000 and compares it with the R2000 fixture's.
 */
void Check(std::string_view name, const Counts &expected)
{
  const std::expected<Loaded, Failure> older = Load(ReadFixture(std::string{name} + "-r14.dwg"));
  const std::expected<Loaded, Failure> newer = Load(ReadFixture(std::string{name} + "-r2000.dwg"));
  REQUIRE(older);
  REQUIRE(newer);
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Counts counts;
  const std::vector<Handle> regenerated = CollectDimensionBlocks(older->Drawing);
  for (const Object &object : older->Drawing.Objects)
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
    const auto same =
        std::ranges::find(newer->Drawing.Objects, self, [](const Object &one) -> Handle { return BaseOf(one).Self; });
    REQUIRE(same != newer->Drawing.Objects.end());
    const Common &modern = *CommonOf(*same);
    const Object translated = Releases::ConvertObject(object, older->Drawing, Version::R2000).Object;
    if (modern.PlotstyleOrigin != Origin::ByLayer || Holds<Block>(object) ||
        BaseOf(translated).Extended.size() != BaseOf(*same).Extended.size())
    {
      ++counts.Aside;
      continue;
    }
    std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(translated, with), 0, reading);
    REQUIRE(back);
    Visit([&object](auto &kind) -> void { Objects::BaseOfKind(kind).Owner = BaseOf(object).Owner; }, *back);
    Reregister(*back, older->Drawing, *same, newer->Drawing);
    if (auto *dimension = GetIf<Dimension>(&*back))
    {
      const Measure &measure = Get<Dimension>(*same).Measure;
      dimension->Measure.Attachment = measure.Attachment;
      dimension->Measure.LineSpacingStyle = measure.LineSpacingStyle;
      dimension->Measure.LineSpacingFactor = measure.LineSpacingFactor;
      dimension->Measure.Measured = measure.Measured;
    }
    if (auto *viewport = GetIf<Viewport>(&*back))
    {
      // R14's view of a viewport, read from its extended data, has no clip entity and no UCS of its own.
      constexpr int Clipped = 65536;
      const Viewport &theirs = Get<Viewport>(*same);
      viewport->Clip = theirs.Clip;
      viewport->Axes = theirs.Axes;
      viewport->Flags |= theirs.Flags & Clipped;
    }
    CHECK(*back == *same);
    ++counts.Compared;
  }
  CHECK(counts.Compared == expected.Compared);
  CHECK(counts.Aside == expected.Aside);
  CHECK(counts.Regenerated == expected.Regenerated);
  CHECK(issues.empty());
}
} // namespace

TEST_CASE("entities/older: every typed entity of the R14 fixtures written as R2000 matches the R2000 fixture")
{
  for (const auto &[name, counts] :
       {std::pair{std::string_view{"entities"}, Counts{.Compared = 33, .Aside = 6}},
        std::pair{std::string_view{"curves"}, Counts{.Compared = 172, .Aside = 19, .Regenerated = 92}},
        std::pair{std::string_view{"objects"}, Counts{.Compared = 13, .Aside = 4}},
        std::pair{std::string_view{"empty"}, Counts{.Compared = 3, .Aside = 3}}})
  {
    CAPTURE(name);
    Check(name, counts);
  }
}
} // namespace DwgIO::Entities
