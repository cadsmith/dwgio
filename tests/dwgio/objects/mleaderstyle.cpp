/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/mleaderstyle.cpp
 *  @brief     The MLEADERSTYLE against its DXF twin in every release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/mleaderstyle.hpp"

#include "support/emitted.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/twin.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
/**
 *  The AcDbMLeaderStyle groups AutoCAD writes for a style.
 */
auto Emit(const MLeaderStyle &style, Version release) -> std::vector<Pair>
{
  Emitted out;
  if (release >= Version::R2010)
  {
    out.Add(179, style.Revision);
  }
  out.Add(170, style.ContentType);
  out.Add(171, style.DrawOrder);
  out.Add(172, style.Leader.DrawOrder);
  out.Add(90, style.Leader.MaxPoints);
  out.Add(40, style.Leader.FirstSegmentAngle);
  out.Add(41, style.Leader.SecondSegmentAngle);
  out.Add(173, style.Leader.LeaderType);
  out.Add(91, style.Leader.Color);
  out.Add(340, style.Leader.Linetype);
  out.Add(92, style.Leader.Lineweight);
  out.Add(290, style.Leader.EnableLanding);
  out.Add(42, style.Leader.LandingGap);
  out.Add(291, style.Leader.EnableDogleg);
  out.Add(43, style.Leader.DoglegLength);
  out.Add(3, style.Description);
  out.Add(341, style.Leader.Arrowhead);
  out.Add(44, style.Leader.ArrowheadSize);
  out.Add(300, style.Text.DefaultText);
  out.Add(342, style.Text.Style);
  out.Add(174, style.Text.LeftAttachment);
  out.Add(178, style.Text.RightAttachment);
  out.Add(175, style.Text.AngleType);
  out.Add(176, style.Text.Alignment);
  out.Add(93, style.Text.Color);
  out.Add(45, style.Text.Height);
  out.Add(292, style.Text.Frame);
  out.Add(297, style.Text.AlwaysLeft);
  out.Add(46, style.Text.AlignSpace);
  out.Add(343, style.Block.Content);
  out.Add(94, style.Block.Color);
  out.Add(47, style.Block.Scale.X);
  out.Add(49, style.Block.Scale.Y);
  out.Add(140, style.Block.Scale.Z);
  out.Add(293, style.Block.EnableScale);
  out.Add(141, style.Block.Rotation);
  out.Add(294, style.Block.EnableRotation);
  out.Add(177, style.Block.Connection);
  out.Add(142, style.Scale);
  out.Add(295, style.OverwriteProperty);
  out.Add(296, style.Annotative);
  out.Add(143, style.BreakGapSize);
  if (release >= Version::R2010)
  {
    out.Add(271, style.Text.AttachmentDirection);
    out.Add(272, style.Text.BottomAttachment);
    out.Add(273, style.Text.TopAttachment);
  }
  if (release >= Version::R2013)
  {
    out.Add(298, style.Unknown1);
  }
  return out.Groups;
}

/**
 *  The styles of a fixture, each checked against its twin's groups, in the order of their handles.
 */
auto CheckFixture(std::string_view name, std::string_view twinName) -> std::vector<MLeaderStyle>
{
  CAPTURE(name);
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  const std::vector<Pair> twin = ReadTwin(twinName);
  std::vector<MLeaderStyle> found;
  for (const Object &object : loaded->Drawing.Objects)
  {
    if (Holds<Unknown>(object))
    {
      CHECK(TypeOf(object) != "MLEADERSTYLE"); // Every one typed.
    }
    const auto *style = GetIf<MLeaderStyle>(&object);
    if (style == nullptr)
    {
      continue;
    }
    CAPTURE(HexOf(style->Base.Self));
    CHECK(TypeOf(object) == "MLEADERSTYLE");
    const std::optional<std::size_t> opened = FindHandled(twin, style->Base.Self);
    REQUIRE(opened);
    CheckGroups(Emit(*style, loaded->Drawing.Release), Subclass(twin, *opened, "AcDbMLeaderStyle"));
    CHECK(style->Base.Extended.size() == (loaded->Drawing.Release < Version::R2010 ? 1 : 0));
    found.push_back(*style);
  }
  std::ranges::sort(found, {}, [](const MLeaderStyle &one) { return one.Base.Self.Value; });
  return found;
}
} // namespace

TEST_CASE("objects/mleaderstyle: every style of the fixtures matches its DXF twin in every release")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    const std::string twin = release == "r14" ? "r2000" : std::string{release};
    const std::vector<MLeaderStyle> kinds =
        CheckFixture(std::format("kinds-{}.dwg", release), std::format("kinds-{}.dxf", twin));
    REQUIRE(kinds.size() == 2);
    CHECK(kinds[0].Annotative == false);
    CHECK(kinds[1].Annotative);
    const std::vector<MLeaderStyle> varied =
        CheckFixture(std::format("mleaders-{}.dwg", release), std::format("mleaders-{}.dxf", twin));
    REQUIRE(varied.size() == 4);
    CHECK(varied[2].ContentType == 1);
    CHECK(varied[2].Block.Content);
    const bool fresh = release == "r2010" || release == "r2013" || release == "r2018";
    CHECK(varied[3].Text.TopAttachment ==
          (fresh ? 10 : 9)); // Before R2010 the default, which the file has no place for.
  }
}
} // namespace DwgIO::Objects
