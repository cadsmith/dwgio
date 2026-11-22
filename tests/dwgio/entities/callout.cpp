/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/callout.cpp
 *  @brief     The multileader's callout and its style written in every release, and their version across R2010.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "classes/catalog.hpp"
#include "entities/shapes/multileader.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/shapes/mleaderstyle.hpp"
#include "objects/write.hpp"
#include "releases/object.hpp"
#include "support/reader.hpp"

#include "dwgio/class.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/mleaderstyle.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  The two classes the kinds are stored under, as a save declares them.
 */
auto MakeClasses() -> std::vector<Class>
{
  std::vector<Class> classes{*Classes::FindDeclared("MULTILEADER"), *Classes::FindDeclared("MLEADERSTYLE")};
  classes[0].Number = 500;
  classes[1].Number = 501;
  return classes;
}

/**
 *  An object written in a release and read back in it, with no issue either way.
 */
auto RoundTrip(const Object &object, Version release) -> Object
{
  const std::vector<Class> classes = MakeClasses();
  std::vector<Issue> issues;
  const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{
      .Release = release, .Text = &Objects::Latin, .Classes = classes, .Issues = &issues};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(object, with), 0, reading);
  REQUIRE(back);
  CHECK(issues.empty());
  return *back;
}

/**
 *  A multileader of a block with an attribute value, a leader line with breaks on two segments, two on the first, and
 *  an arrowhead of its own.
 */
auto Made() -> MultiLeader
{
  MultiLeader leader;
  leader.Base.Self = Handle{0x40};
  leader.Common.Layer = Handle{0x10};
  MultiLeaders::LeaderLine stem{
      .Vertices = {Vector{.X = 1}, Vector{.X = 5, .Y = 5}, Vector{.X = 9, .Y = 5}},
      .Breaks = {MultiLeaders::LineBreak{.Index = 0, .Start = Vector{.X = 2}, .End = Vector{.X = 3}},
                 MultiLeaders::LineBreak{.Index = 0, .Start = Vector{.X = 3.5}, .End = Vector{.X = 4}},
                 MultiLeaders::LineBreak{.Index = 1, .Start = Vector{.X = 6, .Y = 5}, .End = Vector{.X = 7, .Y = 5}}},
      .Index = 0};
  MultiLeaders::LeaderRoot branch{.LastPoint = Vector{.X = 9, .Y = 5}, .DoglegLength = 2};
  branch.Breaks.push_back(MultiLeaders::DoglegBreak{.Start = Vector{.X = 9.5, .Y = 5}, .End = Vector{.X = 10, .Y = 5}});
  branch.Lines.push_back(stem);
  MultiLeaders::ContextData &note = leader.MLeader.Context;
  note.Leaders.push_back(branch);
  note.HasBlock = true;
  note.BlockContent = Handle{0x30};
  note.BlockTransform[3] = 4;
  leader.MLeader.ContentType = 1;
  leader.MLeader.Style = Handle{0x20};
  leader.MLeader.BlockContent = Handle{0x30};
  leader.MLeader.BlockAttributes.push_back(
      MultiLeaders::BlockAttribute{.Definition = Handle{0x31}, .Index = 1, .Width = 2, .Text = "7"});
  leader.MLeader.Arrowheads.push_back(MultiLeaders::Arrowhead{.Index = 1, .Block = Handle{0x32}});
  leader.MLeader.Arrowheads.push_back(MultiLeaders::Arrowhead{.Index = 0, .Block = Handle{0x33}});
  return leader;
}
} // namespace

TEST_CASE("entities/callout: a block multileader with breaks and arrowheads and a label round-trips")
{
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2007})
  {
    CAPTURE(NameOf(release));
    const MultiLeader leader = Made();
    const Object back = RoundTrip(leader, release);
    REQUIRE(Holds<MultiLeader>(back));
    CHECK(Get<MultiLeader>(back).MLeader == leader.MLeader);
  }
  for (const Version release : {Version::R2010, Version::R2013, Version::R2018})
  {
    CAPTURE(NameOf(release));
    MultiLeader leader = Made();
    leader.MLeader.Arrowheads.clear(); // Kept up to R2007 only.
    leader.MLeader.ExtendToText = release >= Version::R2013;
    leader.MLeader.Context.Leaders[0].AttachmentDirection = 1;
    leader.MLeader.Context.Leaders[0].Lines[0].Arrowhead = Handle{0x32};
    leader.MLeader.Context.Leaders[0].Lines[0].Overrides = 32;
    const Object back = RoundTrip(leader, release);
    REQUIRE(Holds<MultiLeader>(back));
    CHECK(Get<MultiLeader>(back).MLeader == leader.MLeader);
  }
}

TEST_CASE("entities/callout: a negative or oversized count of breaks or break pairs is refused")
{
  for (const std::vector<std::int32_t> &counts :
       {std::vector<std::int32_t>{-1}, std::vector<std::int32_t>{1, 0, -1}, std::vector<std::int32_t>{1, 0, 99}})
  {
    Bits::Writer data;
    for (const std::int32_t count : counts)
    {
      data.BL(count);
    }
    const std::vector<std::byte> bytes = data.Take();
    Bits::Reader in{bytes};
    const Fields::Input reading{.In = &in, .With = Fields::Context{.Release = Version::R2018}};
    MultiLeaders::LeaderLine stem;
    CHECK_FALSE(Shapes::ReadGaps(stem, reading));
    CHECK(stem.Breaks.empty());
  }
}

TEST_CASE("entities/callout: a style stores the text angle and flush bit only in the new format")
{
  Objects::MLeaderStyle style;
  style.Base.Self = Handle{0x41};
  style.Description = "Callouts";
  style.Text.AngleType = 2;
  style.Text.AlwaysLeft = true;
  // R2010 and later are of the new format, as an older release is with ACAD_MLEADERVER extended data.
  CHECK(Get<Objects::MLeaderStyle>(RoundTrip(style, Version::R2010)) == style);
  Objects::MLeaderStyle versioned = style;
  versioned.Base.Extended.push_back(
      Xdata{.Application = {}, .AppId = Handle{0x12}, .Tags = {Tag{.Code = 1070, .Value = std::int64_t{2}}}});
  CHECK(Get<Objects::MLeaderStyle>(RoundTrip(versioned, Version::R2007)) == versioned);
  // Without it, or with other extended data, an older release has no place for them.
  for (const std::vector<Tag> &groups :
       {std::vector<Tag>{}, std::vector<Tag>{Tag{.Code = 1000, .Value = std::string{"x"}}},
        std::vector<Tag>{Tag{.Code = 1070, .Value = std::int64_t{2}}, Tag{.Code = 1070, .Value = std::int64_t{3}}}})
  {
    Objects::MLeaderStyle old = style;
    if (!groups.empty())
    {
      old.Base.Extended.push_back(Xdata{.Application = {}, .AppId = Handle{0x12}, .Tags = groups});
    }
    Objects::MLeaderStyle expected = old;
    expected.Text.AngleType = 1;
    expected.Text.AlwaysLeft = false;
    CHECK(Get<Objects::MLeaderStyle>(RoundTrip(old, Version::R2004)) == expected);
  }
}

TEST_CASE("entities/callout: the version moves between the data and the ACAD_MLEADERVER xdata at R2010")
{
  // Saved below R2010 with no ACAD_MLEADERVER application to name: noted, and the style left as it is.
  const Drawing newer = Create(Version::R2018);
  Objects::MLeaderStyle style;
  style.Base.Self = Handle{0x41};
  const Releases::Translation down = Releases::ConvertObject(Object{style}, newer, Version::R2007);
  CHECK(Get<Objects::MLeaderStyle>(down.Object).Base.Extended.empty());
  REQUIRE(down.Issues.size() == 1);
  CHECK(down.Issues[0].What == "application ACAD_MLEADERVER missing for the version of MLEADERSTYLE 41");
  // Upgraded: the record dropped, its version taken, or kept when the record holds no whole number.
  const Drawing older = Create(Version::R2007);
  Objects::MLeaderStyle versioned = style;
  versioned.Base.Extended.push_back(
      Xdata{.Application = {}, .AppId = Handle{0x12}, .Tags = {Tag{.Code = 1070, .Value = std::int64_t{3}}}});
  const Releases::Translation up = Releases::ConvertObject(Object{versioned}, older, Version::R2010);
  CHECK(Get<Objects::MLeaderStyle>(up.Object).Revision == 3);
  CHECK(Get<Objects::MLeaderStyle>(up.Object).Base.Extended.empty());
  versioned.Base.Extended[0].Tags[0].Value = 3.5;
  CHECK(Get<Objects::MLeaderStyle>(Releases::ConvertObject(Object{versioned}, older, Version::R2010).Object).Revision ==
        Objects::MLeaderStyles::Revision);
  // Saved below R2010 with the record there already: kept, none added.
  const Releases::Translation kept = Releases::ConvertObject(Object{versioned}, newer, Version::R2004);
  CHECK(Get<Objects::MLeaderStyle>(kept.Object).Base.Extended == versioned.Base.Extended);
  CHECK(kept.Issues.empty());
  // A multileader's arrowheads, which R2010 has no place for, are dropped and noted.
  MultiLeader leader = Made();
  const Releases::Translation upgraded = Releases::ConvertObject(Object{leader}, older, Version::R2018);
  CHECK(Get<MultiLeader>(upgraded.Object).MLeader.Arrowheads.empty());
  REQUIRE(upgraded.Issues.size() == 1);
  CHECK(upgraded.Issues[0].What == "arrowheads of MULTILEADER 40 not kept by R2010");
  leader.MLeader.Arrowheads.clear();
  CHECK(Releases::ConvertObject(Object{leader}, older, Version::R2018).Issues.empty());
}
} // namespace DwgIO::Entities
