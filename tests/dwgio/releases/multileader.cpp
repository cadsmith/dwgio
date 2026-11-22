/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/multileader.cpp
 *  @brief     A multileader's leader lines kept in its record below R2010 and taken back on upgrading.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/multileader.hpp"

#include "geometry/axes.hpp"
#include "releases/roundtrip.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"
#include "support/pass.hpp"
#include "support/vector.hpp"

#include "dwgio/color.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The multileader whose lines override what they draw with, in the leaders fixtures.
 */
constexpr Handle Overriding{.Value = 0x2F8};

auto LeaderOf(const Drawing &drawing) -> const Entities::MultiLeader &
{
  const auto *leader = GetIf<Entities::MultiLeader>(Find(drawing, Overriding));
  REQUIRE(leader != nullptr);
  return *leader;
}

/**
 *  The groups of the multileader's round-trip record, or nothing.
 */
auto RecordOf(const Drawing &drawing) -> std::optional<std::vector<Tag>>
{
  const auto *dictionary = GetIf<Objects::Dictionary>(Find(drawing, LeaderOf(drawing).Base.Dictionary));
  if (dictionary == nullptr)
  {
    return std::nullopt;
  }
  const auto entry = std::ranges::find(dictionary->Entries, "ACAD_XREC_ROUNDTRIP", &Objects::Entry::Name);
  REQUIRE(entry != dictionary->Entries.end());
  return Get<Objects::XRecord>(*Find(drawing, entry->Object)).Data;
}

auto Lines(const Entities::MultiLeader &leader) -> std::vector<Entities::MultiLeaders::LeaderLine>
{
  std::vector<Entities::MultiLeaders::LeaderLine> lines;
  for (const Entities::MultiLeaders::LeaderRoot &branch : leader.MLeader.Context.Leaders)
  {
    lines.insert(lines.end(), branch.Lines.begin(), branch.Lines.end());
  }
  return lines;
}

auto MakeMultiLeader(std::vector<Entities::MultiLeaders::LeaderLine> stems) -> Entities::MultiLeader
{
  Entities::MultiLeader leader;
  leader.MLeader.Context.Leaders = {Entities::MultiLeaders::LeaderRoot{.Lines = std::move(stems)}};
  return leader;
}
} // namespace

TEST_CASE("releases/multileader: leader lines move into the multileader's record below R2010")
{
  const std::optional<std::vector<Tag>> theirs = RecordOf(LoadFixture("leaders-r2007.dwg").Drawing);
  REQUIRE(theirs);
  for (const std::string_view name : {"leaders-r2010.dwg", "leaders-r2018.dwg"})
  {
    CAPTURE(name);
    const std::optional<Drawing> lowered = Pass(KeepRoundTrips, LoadFixture(name).Drawing, Version::R2007);
    REQUIRE(lowered);
    CHECK(RecordOf(*lowered) == theirs);
    CHECK(Lines(LeaderOf(*lowered)) == Lines(LeaderOf(LoadFixture("leaders-r2007.dwg").Drawing)));
  }
}

TEST_CASE("releases/multileader: a multileader's leader lines come back from its record on upgrading to R2010")
{
  const std::optional<Drawing> upgraded =
      Pass(KeepRoundTrips, LoadFixture("leaders-r2007.dwg").Drawing, Version::R2010);
  REQUIRE(upgraded);
  const Drawing theirs = LoadFixture("leaders-r2007-as-r2010.dwg").Drawing;
  CHECK(Lines(LeaderOf(*upgraded)) == Lines(LeaderOf(theirs)));
  CHECK(LeaderOf(*upgraded).Base.Dictionary == Handle{});
  CHECK(!RecordOf(*upgraded));
}

TEST_CASE("releases/multileader: leader lines saved below R2010 and back keep what they hold")
{
  const Drawing newer = LoadFixture("leaders-r2018.dwg").Drawing;
  const std::expected<Saved, Failure> down = Save(newer, SaveOptions{.Release = Version::R2007});
  REQUIRE(down);
  const std::expected<Loaded, Failure> older = Load(down->Bytes);
  REQUIRE(older);
  CHECK(older->Issues.empty());
  QueueAudit(down->Bytes, "dwgio-leaders-r2018-as-r2007.dwg");
  const std::expected<Saved, Failure> up = Save(older->Drawing, SaveOptions{.Release = Version::R2018});
  REQUIRE(up);
  const std::expected<Loaded, Failure> back = Load(up->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  CHECK(Lines(LeaderOf(back->Drawing)) == Lines(LeaderOf(newer)));
  QueueAudit(up->Bytes, "dwgio-leaders-r2018-as-r2007-as-r2018.dwg");
}

TEST_CASE("releases/multileader: every color kind converts through the record")
{
  const std::vector<Color> colors{Color{.Index = 256}, Color{.Index = 0}, Color{.Index = 5},
                                  Color{.Index = 0, .Rgb = static_cast<std::int32_t>(0xC2102030U)}};
  std::vector<Entities::MultiLeaders::LeaderLine> stems;
  for (const Color &color : colors)
  {
    stems.push_back(
        Entities::MultiLeaders::LeaderLine{.Index = static_cast<int>(stems.size()), .Color = color, .Overrides = 2});
  }
  stems.push_back(Entities::MultiLeaders::LeaderLine{.Index = 9, .LeaderType = 2});
  Entities::MultiLeader leader = MakeMultiLeader(stems);
  std::vector<Tag> record = MakeLinesRecord(leader);
  CHECK(record.size() == 16);
  CHECK(Lines(leader).back() ==
        Entities::MultiLeaders::LeaderLine{.Index = 9}); // Nothing of its own; what R2007 holds left.
  Entities::MultiLeader back = MakeMultiLeader(std::vector<Entities::MultiLeaders::LeaderLine>(4));
  for (int index = 0; index < 4; ++index)
  {
    back.MLeader.Context.Leaders.front().Lines[static_cast<std::size_t>(index)].Index = index;
  }
  RestoreLines(back, record);
  CHECK(record.empty());
  const std::vector<Entities::MultiLeaders::LeaderLine> lines = Lines(back);
  for (std::size_t at = 0; at < colors.size(); ++at)
  {
    CHECK(lines[at].Color == colors[at]);
  }
}

TEST_CASE("releases/multileader: record values that are not a line's stay and a line with no index is skipped")
{
  Entities::MultiLeader leader = MakeMultiLeader({Entities::MultiLeaders::LeaderLine{.Index = 1}});
  std::vector<Tag> record{Tag{.Code = 102, .Value = std::string{"OTHER"}},
                          Tag{.Code = 272, .Value = std::int64_t{1}},
                          Tag{.Code = 102, .Value = std::int64_t{3}},
                          Tag{.Code = 102, .Value = std::string{"Leaderline7"}},
                          Tag{.Code = 40, .Value = 3.0},
                          Tag{.Code = 102, .Value = std::string{"Leaderline5"}},
                          Tag{.Code = 91, .Value = std::int64_t{5}},
                          Tag{.Code = 40, .Value = 3.0},
                          Tag{.Code = 102, .Value = std::string{"Leaderline1"}},
                          Tag{.Code = 91, .Value = std::int64_t{1}},
                          Tag{.Code = 40, .Value = std::string{"odd"}},
                          Tag{.Code = 341, .Value = std::int64_t{2}},
                          Tag{.Code = 340, .Value = Handle{0x40}},
                          Tag{.Code = 171, .Value = std::string{"odd"}},
                          Tag{.Code = 171, .Value = std::int64_t{30}},
                          Tag{.Code = 170, .Value = std::int64_t{2}},
                          Tag{.Code = 7, .Value = std::int64_t{1}},
                          Tag{.Code = 93, .Value = std::int64_t{29}},
                          Tag{.Code = 102, .Value = std::string{"Leaderline8"}}};
  RestoreLines(leader, record);
  CHECK(record == std::vector<Tag>{Tag{.Code = 102, .Value = std::string{"OTHER"}},
                                   Tag{.Code = 272, .Value = std::int64_t{1}},
                                   Tag{.Code = 102, .Value = std::int64_t{3}}});
  const Entities::MultiLeaders::LeaderLine stem = Lines(leader).front();
  CHECK(stem.ArrowheadSize == 0);
  CHECK(stem.Arrowhead == Handle{});
  CHECK(stem.Linetype == Handle{0x40});
  CHECK(stem.Lineweight == 30);
  CHECK(stem.LeaderType == 2);
  CHECK(stem.Overrides == 29);
}
} // namespace DwgIO::Releases

namespace DwgIO::Releases
{
namespace
{
auto Leaders(const Drawing &drawing) -> std::vector<const Entities::MultiLeader *>
{
  std::vector<const Entities::MultiLeader *> leaders;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *leader = GetIf<Entities::MultiLeader>(&object))
    {
      leaders.push_back(leader);
    }
  }
  std::ranges::sort(leaders, {}, [](const Entities::MultiLeader *leader) { return leader->Base.Self.Value; });
  return leaders;
}

/**
 *  The groups of a multileader's round-trip record, empty where it has none.
 */
auto Kept(const Drawing &drawing, const Entities::MultiLeader &leader) -> std::vector<Tag>
{
  const auto *dictionary = GetIf<Objects::Dictionary>(Find(drawing, leader.Base.Dictionary));
  if (dictionary == nullptr)
  {
    return {};
  }
  const auto entry = std::ranges::find(dictionary->Entries, "ACAD_XREC_ROUNDTRIP", &Objects::Entry::Name);
  return entry == dictionary->Entries.end() ? std::vector<Tag>{}
                                            : Get<Objects::XRecord>(*Find(drawing, entry->Object)).Data;
}

/**
 *  What a multileader holds but its content's base point, which AutoCAD's saves across R2010 derive from the text's
 *  extents for a vertically attached text.
 */
auto Held(const Entities::MultiLeader &leader) -> Entities::MultiLeaders::MLeader
{
  Entities::MultiLeaders::MLeader callout = leader.MLeader;
  callout.Context.ContentBasePoint = {};
  return callout;
}
} // namespace

TEST_CASE("releases/multileader: a multileader's text attachments move into its record below R2010")
{
  const Drawing theirs = LoadFixture("attachments-r2007.dwg").Drawing;
  for (const std::string_view name : {"attachments-r2010.dwg", "attachments-r2018.dwg"})
  {
    CAPTURE(name);
    const std::optional<Drawing> lowered = Pass(KeepRoundTrips, LoadFixture(name).Drawing, Version::R2007);
    REQUIRE(lowered);
    const std::vector<const Entities::MultiLeader *> ours = Leaders(*lowered);
    const std::vector<const Entities::MultiLeader *> expected = Leaders(theirs);
    REQUIRE(ours.size() == expected.size());
    for (std::size_t at = 0; at < ours.size(); ++at)
    {
      CAPTURE(ours[at]->Base.Self.Value);
      CHECK(Kept(*lowered, *ours[at]) == Kept(theirs, *expected[at]));
      CHECK(Held(*ours[at]) == Held(*expected[at]));
    }
  }
}

TEST_CASE("releases/multileader: text attachments come back from the record on upgrading to R2010")
{
  const std::optional<Drawing> upgraded =
      Pass(KeepRoundTrips, LoadFixture("attachments-r2007.dwg").Drawing, Version::R2010);
  REQUIRE(upgraded);
  const Drawing theirs = LoadFixture("attachments-r2007-as-r2010.dwg").Drawing;
  const std::vector<const Entities::MultiLeader *> ours = Leaders(*upgraded);
  const std::vector<const Entities::MultiLeader *> expected = Leaders(theirs);
  REQUIRE(ours.size() == expected.size());
  for (std::size_t at = 0; at < ours.size(); ++at)
  {
    CAPTURE(ours[at]->Base.Self.Value);
    CHECK(Held(*ours[at]) == Held(*expected[at]));
    CHECK(Kept(*upgraded, *ours[at]).empty());
  }
}

TEST_CASE("releases/multileader: a bottom text attachment other than centered moves into the record")
{
  Drawing drawing = LoadFixture("attachments-r2010.dwg").Drawing;
  for (Object &object : drawing.Objects)
  {
    if (auto *leader = GetIf<Entities::MultiLeader>(&object); leader != nullptr && leader->Base.Self == Handle{0x2D9})
    {
      leader->MLeader.TextTopAttachment = 9;
    }
  }
  const std::optional<Drawing> lowered = Pass(KeepRoundTrips, drawing, Version::R2007);
  REQUIRE(lowered);
  const auto &leader = Get<Entities::MultiLeader>(*Find(*lowered, Handle{0x2D9}));
  CHECK(Kept(*lowered, leader) ==
        std::vector<Tag>{Tag{.Code = 102, .Value = std::string{"ACAD_ROUNDTRIP_2010_MLEADER"}},
                         Tag{.Code = 273, .Value = std::int64_t{10}}});
}

TEST_CASE("releases/multileader: text attachments saved below R2010 and back keep what AutoCAD's saves keep")
{
  const Drawing newer = LoadFixture("attachments-r2018.dwg").Drawing;
  const std::expected<Saved, Failure> down = Save(newer, SaveOptions{.Release = Version::R2007});
  REQUIRE(down);
  const std::expected<Loaded, Failure> older = Load(down->Bytes);
  REQUIRE(older);
  CHECK(older->Issues.empty());
  QueueAudit(down->Bytes, "dwgio-attachments-r2018-as-r2007.dwg");
  const std::expected<Saved, Failure> up = Save(older->Drawing, SaveOptions{.Release = Version::R2018});
  REQUIRE(up);
  const std::expected<Loaded, Failure> back = Load(up->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  QueueAudit(up->Bytes, "dwgio-attachments-r2018-as-r2007-as-r2018.dwg");
  // AutoCAD's own saves down and up keep all but the doglegs of vertically attached leaders, and mark the
  // attachments and the dogleg overridden.
  const Drawing theirs = LoadFixture("attachments-r2007-as-r2010.dwg").Drawing;
  const std::vector<const Entities::MultiLeader *> ours = Leaders(back->Drawing);
  const std::vector<const Entities::MultiLeader *> expected = Leaders(theirs);
  REQUIRE(ours.size() == expected.size());
  for (std::size_t at = 0; at < ours.size(); ++at)
  {
    CAPTURE(ours[at]->Base.Self.Value);
    CHECK(Close(ours[at]->MLeader.Context.ContentBasePoint, Leaders(newer)[at]->MLeader.Context.ContentBasePoint));
    CHECK(Held(*ours[at]) == Held(*expected[at]));
  }
}
} // namespace DwgIO::Releases

namespace DwgIO::Releases
{
namespace
{
/**
 *  The vertical fixtures' multileaders by the alignment of their text: centered, left and right; the last one holds a
 *  block.
 */
constexpr std::array<std::uint64_t, 3> Centered{0x2D4, 0x2D8, 0x2DD};
constexpr std::array<std::uint64_t, 3> Lefts{0x2D9, 0x2DA, 0x2DF};

auto Along(Vector from, Vector to, Vector direction) -> double
{
  return Geometry::Dot(Vector{.X = to.X - from.X, .Y = to.Y - from.Y, .Z = to.Z - from.Z}, direction);
}

auto UpOf(const Entities::MultiLeaders::ContextData &note) -> Vector
{
  return Geometry::Cross(note.TextNormal, note.TextDirection);
}
} // namespace

TEST_CASE("releases/multileader: a vertical text's base point moves below R2010 as AutoCAD's save moves it")
{
  const Drawing newer = LoadFixture("vertical-r2018.dwg").Drawing;
  const std::optional<Drawing> lowered = Pass(KeepRoundTrips, newer, Version::R2007);
  REQUIRE(lowered);
  const Drawing theirs = LoadFixture("vertical-r2007.dwg").Drawing;
  const std::vector<const Entities::MultiLeader *> ours = Leaders(*lowered);
  const std::vector<const Entities::MultiLeader *> expected = Leaders(theirs);
  const std::vector<const Entities::MultiLeader *> before = Leaders(newer);
  REQUIRE(ours.size() == 10);
  REQUIRE(ours.size() == expected.size());
  for (std::size_t at = 0; at < ours.size(); ++at)
  {
    CAPTURE(ours[at]->Base.Self.Value);
    const Entities::MultiLeaders::ContextData &note = ours[at]->MLeader.Context;
    const Vector theirBase = expected[at]->MLeader.Context.ContentBasePoint;
    CHECK(Held(*ours[at]) == Held(*expected[at]));
    if (std::ranges::contains(Centered, ours[at]->Base.Self.Value))
    {
      // The font gives AutoCAD half the text's width more; the location of a centered text does not show it.
      const Vector was = before[at]->MLeader.Context.ContentBasePoint;
      CHECK(Along(note.ContentBasePoint, was, note.TextDirection) == doctest::Approx(note.LandingGap));
      CHECK(Along(note.ContentBasePoint, theirBase, UpOf(note)) == doctest::Approx(0));
      CHECK(Along(note.ContentBasePoint, theirBase, note.TextDirection) < 0);
    }
    else
    {
      CHECK(Close(note.ContentBasePoint, theirBase));
    }
  }
}

TEST_CASE("releases/multileader: a vertical text's base point moves back above R2010 as AutoCAD's save moves it")
{
  const std::optional<Drawing> upgraded =
      Pass(KeepRoundTrips, LoadFixture("vertical-r2007.dwg").Drawing, Version::R2018);
  REQUIRE(upgraded);
  const Drawing theirs = LoadFixture("vertical-r2007-as-r2018.dwg").Drawing;
  const std::vector<const Entities::MultiLeader *> ours = Leaders(*upgraded);
  const std::vector<const Entities::MultiLeader *> expected = Leaders(theirs);
  REQUIRE(ours.size() == expected.size());
  for (std::size_t at = 0; at < ours.size(); ++at)
  {
    CAPTURE(ours[at]->Base.Self.Value);
    const Entities::MultiLeaders::ContextData &note = ours[at]->MLeader.Context;
    const Vector theirBase = expected[at]->MLeader.Context.ContentBasePoint;
    CHECK(Held(*ours[at]) == Held(*expected[at]));
    if (std::ranges::contains(Lefts, ours[at]->Base.Self.Value))
    {
      // A text aligned left has its location at its left edge, which leaves its width unknown.
      CHECK(Along(note.ContentBasePoint, note.TextLocation, note.TextDirection) == doctest::Approx(0));
      CHECK(Along(note.ContentBasePoint, theirBase, UpOf(note)) == doctest::Approx(0));
      CHECK(Along(note.ContentBasePoint, theirBase, note.TextDirection) > 0);
    }
    else
    {
      CHECK(Close(note.ContentBasePoint, theirBase));
    }
  }
}

TEST_CASE("releases/multileader: the base point of a text with no leader moves by the landing gap and back")
{
  Entities::MultiLeader leader;
  leader.MLeader.TextAttachmentDirection = 1;
  Entities::MultiLeaders::ContextData &note = leader.MLeader.Context;
  note.HasMText = true;
  note.TextAttachment = 2;
  note.TextHeight = 3;
  note.LandingGap = 2;
  note.ContentBasePoint = Vector{.X = 10, .Y = 5};
  note.TextLocation = Vector{.X = 10, .Y = 9};
  std::vector<Tag> record = MakeLinesRecord(leader);
  CHECK(note.ContentBasePoint == Vector{.X = 8, .Y = 5});
  RestoreLines(leader, record);
  CHECK(note.ContentBasePoint == Vector{.X = 10, .Y = 5});
}

TEST_CASE("releases/multileader: leader values with no leader or of the wrong kind are skipped")
{
  Entities::MultiLeader leader;
  leader.MLeader.Context.Leaders = {Entities::MultiLeaders::LeaderRoot{.Index = 2, .DoglegLength = 5}};
  std::vector<Tag> record{Tag{.Code = 7, .Value = std::int64_t{1}},
                          Tag{.Code = 102, .Value = std::string{"ROUNDTRIP_MLCONTEXT"}},
                          Tag{.Code = 7, .Value = std::int64_t{1}},
                          Tag{.Code = 91, .Value = std::int64_t{4}},
                          Tag{.Code = 40, .Value = 3.0},
                          Tag{.Code = 91, .Value = std::int64_t{2}},
                          Tag{.Code = 7, .Value = 3.0},
                          Tag{.Code = 91, .Value = std::int64_t{2}},
                          Tag{.Code = 40, .Value = std::string{"odd"}},
                          Tag{.Code = 102, .Value = std::string{"ACAD_ROUNDTRIP_2010_MLEADER"}},
                          Tag{.Code = 7, .Value = std::int64_t{1}},
                          Tag{.Code = 41, .Value = 2.0}};
  RestoreLines(leader, record);
  CHECK(record == std::vector<Tag>{Tag{.Code = 7, .Value = std::int64_t{1}}});
  CHECK(leader.MLeader.Context.Leaders.front().DoglegLength == 0);
  CHECK(leader.MLeader.DoglegLength == 2);
  for (const auto &[top, bottom] : {std::pair{10, 9}, std::pair{9, 10}})
  {
    Entities::MultiLeader flat;
    flat.MLeader.TextTopAttachment = top;
    flat.MLeader.TextBottomAttachment = bottom;
    const std::vector<Tag> kept = MakeLinesRecord(flat);
    REQUIRE(kept.size() == 2);
    CHECK(kept.back() == Tag{.Code = top == 10 ? 274 : 273, .Value = std::int64_t{10}});
    CHECK(flat.MLeader.TextTopAttachment == 9);
    CHECK(flat.MLeader.TextBottomAttachment == 9);
  }
}

TEST_CASE("releases/multileader: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
