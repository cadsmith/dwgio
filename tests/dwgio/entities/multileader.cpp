/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/multileader.cpp
 *  @brief     The MULTILEADER against its DXF twin in every release, and saved across releases.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/multileader.hpp"

#include "releases/multileader.hpp"
#include "support/acad.hpp"
#include "support/emitted.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/saves.hpp"
#include "support/twin.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/mleaderstyle.hpp"
#include "dwgio/save.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
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
/**
 *  The groups of a leader line.
 */
void EmitStem(Emitted &out, const MultiLeaders::LeaderLine &stem)
{
  out.Add(304, std::string{"LEADER_LINE{"});
  for (const Vector &point : stem.Vertices)
  {
    out.Add(10, point);
  }
  for (const MultiLeaders::LineBreak &gap : stem.Breaks)
  {
    out.Add(90, gap.Index);
    out.Add(11, gap.Start);
    out.Add(12, gap.End);
  }
  out.Add(91, stem.Index);
  out.Add(305, std::string{"}"});
}

/**
 *  The groups of a leader.
 */
void EmitBranch(Emitted &out, const MultiLeaders::LeaderRoot &branch, Version release)
{
  out.Add(302, std::string{"LEADER{"});
  out.Add(290, branch.HasLastPoint);
  out.Add(291, branch.HasDoglegVector);
  out.Add(10, branch.LastPoint);
  out.Add(11, branch.DoglegVector);
  for (const MultiLeaders::DoglegBreak &span : branch.Breaks)
  {
    out.Add(12, span.Start);
    out.Add(13, span.End);
  }
  out.Add(90, branch.Index);
  out.Add(40, branch.DoglegLength);
  for (const MultiLeaders::LeaderLine &stem : branch.Lines)
  {
    EmitStem(out, stem);
  }
  if (release >= Version::R2010)
  {
    out.Add(271, branch.AttachmentDirection);
  }
  out.Add(303, std::string{"}"});
}

void EmitText(Emitted &out, const MultiLeaders::ContextData &note)
{
  out.Add(304, note.Text);
  out.Add(11, note.TextNormal);
  out.Add(340, note.TextStyle);
  out.Add(12, note.TextLocation);
  out.Add(13, note.TextDirection);
  out.Add(42, note.TextRotation);
  out.Add(43, note.BoundaryWidth);
  out.Add(44, note.BoundaryHeight);
  out.Add(45, note.LineSpacingFactor);
  out.Add(170, note.LineSpacingStyle);
  out.Add(90, note.TextColor);
  out.Add(171, note.TextAttachment);
  out.Add(172, note.FlowDirection);
  out.Add(91, note.BackgroundColor);
  out.Add(141, note.BackgroundScale);
  out.Add(92, note.BackgroundTransparency);
  out.Add(291, note.BackgroundFill);
  out.Add(292, note.BackgroundMask);
  out.Add(173, note.ColumnType);
  out.Add(293, note.AutoHeight);
  out.Add(142, note.ColumnWidth);
  out.Add(143, note.ColumnGutter);
  out.Add(294, note.ColumnFlowReversed);
  for (const double height : note.ColumnHeights)
  {
    out.Add(144, height);
  }
  out.Add(295, note.WordBreak);
}

void EmitBlock(Emitted &out, const MultiLeaders::ContextData &note)
{
  out.Add(341, note.BlockContent);
  out.Add(14, note.BlockNormal);
  out.Add(15, note.BlockPosition);
  out.Add(16, note.BlockScale);
  out.Add(46, note.BlockRotation);
  out.Add(93, note.BlockColor);
  for (const double cell : note.BlockTransform)
  {
    out.Add(47, cell);
  }
}

/**
 *  The context data's groups.
 */
void EmitNote(Emitted &out, const MultiLeaders::ContextData &note, Version release)
{
  out.Add(300, std::string{"CONTEXT_DATA{"});
  out.Add(40, note.ContentScale);
  out.Add(10, note.ContentBasePoint);
  out.Add(41, note.TextHeight);
  out.Add(140, note.ArrowheadSize);
  out.Add(145, note.LandingGap);
  out.Add(174, note.TextLeftAttachment);
  out.Add(175, note.TextRightAttachment);
  out.Add(176, note.TextAlignment);
  out.Add(177, note.AttachmentType);
  out.Add(290, note.HasMText);
  if (note.HasMText)
  {
    EmitText(out, note);
  }
  out.Add(296, note.HasBlock);
  if (note.HasBlock)
  {
    EmitBlock(out, note);
  }
  out.Add(110, note.PlaneOrigin);
  out.Add(111, note.PlaneXAxis);
  out.Add(112, note.PlaneYAxis);
  out.Add(297, note.PlaneNormalReversed);
  for (const MultiLeaders::LeaderRoot &branch : note.Leaders)
  {
    EmitBranch(out, branch, release);
  }
  if (release >= Version::R2010)
  {
    out.Add(272, note.TextBottomAttachment);
    out.Add(273, note.TextTopAttachment);
  }
  out.Add(301, std::string{"}"});
}

/**
 *  The AcDbMLeader groups AutoCAD writes for a multileader.
 */
auto Emit(const MultiLeaders::MLeader &callout, Version release) -> std::vector<Pair>
{
  Emitted out;
  if (release >= Version::R2010)
  {
    out.Add(270, callout.Version);
  }
  EmitNote(out, callout.Context, release);
  out.Add(340, callout.Style);
  out.Add(90, callout.Overrides);
  out.Add(170, callout.LeaderType);
  out.Add(91, callout.LineColor);
  out.Add(341, callout.Linetype);
  out.Add(171, callout.Lineweight);
  out.Add(290, callout.EnableLanding);
  out.Add(291, callout.EnableDogleg);
  out.Add(41, callout.DoglegLength);
  out.Add(342, callout.Arrowhead);
  out.Add(42, callout.ArrowheadSize);
  out.Add(172, callout.ContentType);
  out.Add(343, callout.TextStyle);
  out.Add(173, callout.TextLeftAttachment);
  out.Add(95, callout.TextRightAttachment);
  out.Add(174, callout.TextAngleType);
  out.Add(175, callout.TextAlignment);
  out.Add(92, callout.TextColor);
  out.Add(292, callout.TextFrame);
  out.Add(344, callout.BlockContent);
  out.Add(93, callout.BlockColor);
  out.Add(10, callout.BlockScale);
  out.Add(43, callout.BlockRotation);
  out.Add(176, callout.BlockConnection);
  out.Add(293, callout.Annotative);
  for (const MultiLeaders::Arrowhead &tip : callout.Arrowheads)
  {
    out.Add(94, tip.Index);
    out.Add(345, tip.Block);
  }
  for (const MultiLeaders::BlockAttribute &label : callout.BlockAttributes)
  {
    out.Add(330, label.Definition);
    out.Add(177, label.Index);
    out.Add(44, label.Width);
    out.Add(302, label.Text);
  }
  out.Add(294, callout.TextDirectionNegative);
  out.Add(178, callout.IpeAlignment);
  out.Add(179, callout.TextAttachmentPoint);
  out.Add(45, callout.OverallScale);
  if (release >= Version::R2010)
  {
    out.Add(271, callout.TextAttachmentDirection);
    out.Add(272, callout.TextBottomAttachment);
    out.Add(273, callout.TextTopAttachment);
  }
  if (release >= Version::R2013)
  {
    out.Add(295, callout.ExtendToText);
  }
  return out.Groups;
}

/**
 *  The multileaders of a fixture, each checked against its twin's groups; how many there are.
 */
auto CheckFixture(std::string_view name, std::string_view twinName) -> std::vector<MultiLeader>
{
  CAPTURE(name);
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  const std::vector<Pair> twin = ReadTwin(twinName);
  std::vector<MultiLeader> found;
  for (const Object &object : loaded->Drawing.Objects)
  {
    if (Holds<Unknown>(object))
    {
      CHECK(TypeOf(object) != "MULTILEADER"); // Every one typed.
    }
    const auto *leader = GetIf<MultiLeader>(&object);
    if (leader == nullptr)
    {
      continue;
    }
    CAPTURE(HexOf(leader->Base.Self));
    const std::optional<std::size_t> opened = Entity(twin, leader->Base.Self);
    REQUIRE(opened);
    CheckGroups(Emit(leader->MLeader, loaded->Drawing.Release), Subclass(twin, *opened, "AcDbMLeader"));
    CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, leader->Common, leader->Base);
    found.push_back(*leader);
  }
  std::ranges::sort(found, {}, [](const MultiLeader &one) { return one.Base.Self.Value; });
  return found;
}

/**
 *  A callout as a release keeps it: what the releases after it add at its defaults.
 */
auto Kept(MultiLeaders::MLeader callout, Version release) -> MultiLeaders::MLeader
{
  if (release < Version::R2010)
  {
    // As AutoCAD's save below R2010 leaves it, what it holds of R2010 in its record.
    MultiLeader leader{.MLeader = std::move(callout)};
    static_cast<void>(Releases::MakeLinesRecord(leader));
    callout = std::move(leader.MLeader);
    for (MultiLeaders::LeaderRoot &branch : callout.Context.Leaders)
    {
      branch.AttachmentDirection = 0;
    }
  }
  if (release < Version::R2013)
  {
    callout.ExtendToText = false;
  }
  return callout;
}

/**
 *  A style as a release keeps it.
 */
auto Kept(Objects::MLeaderStyle style, Version release) -> Objects::MLeaderStyle
{
  const Objects::MLeaderStyles::TextFormat fresh{};
  if (release < Version::R2010)
  {
    style.Text.AttachmentDirection = fresh.AttachmentDirection;
    style.Text.TopAttachment = fresh.TopAttachment;
    style.Text.BottomAttachment = fresh.BottomAttachment;
  }
  if (release < Version::R2013)
  {
    style.Unknown1 = false;
  }
  style.Base = {};
  return style;
}

/**
 *  The objects of a kind in a drawing, in the order of their handles.
 */
template <typename KindT>
auto All(const Drawing &drawing) -> std::vector<KindT>
{
  std::vector<KindT> found;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *kind = GetIf<KindT>(&object))
    {
      found.push_back(*kind);
    }
  }
  std::ranges::sort(found, {}, [](const KindT &one) { return one.Base.Self.Value; });
  return found;
}
} // namespace

TEST_CASE("entities/multileader: every multileader of the fixtures matches its DXF twin in every release")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    const std::string twin = release == "r14" ? "r2000" : std::string{release};
    const std::vector<MultiLeader> kinds =
        CheckFixture(std::format("kinds-{}.dwg", release), std::format("kinds-{}.dxf", twin));
    REQUIRE(kinds.size() == 1);
    CHECK(kinds.front().MLeader.Context.Text == "note");
    const std::vector<MultiLeader> varied =
        CheckFixture(std::format("mleaders-{}.dwg", release), std::format("mleaders-{}.dxf", twin));
    REQUIRE(varied.size() == 4);
    CHECK(varied[0].MLeader.Context.Leaders.size() == 2);
    CHECK(varied[1].MLeader.Context.HasBlock);
    CHECK(varied[1].MLeader.BlockAttributes.size() == 1);
    CHECK(varied[2].MLeader.Context.Leaders.front().Lines.front().Vertices.size() == 2);
    CHECK(varied[3].MLeader.Context.Leaders.front().Breaks.size() == 1);
    CHECK(varied[3].MLeader.Context.Leaders.front().Lines.front().Breaks.size() == 1);
  }
}

TEST_CASE("entities/multileader: the kinds and multileader fixtures save to their own release byte for byte")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CheckSameSave(std::format("kinds-{}.dwg", release));
    CheckSameSave(std::format("mleaders-{}.dwg", release));
  }
}

TEST_CASE("entities/multileader: the fixtures saved to other releases load back clean")
{
  struct Crossing
  {
    std::string_view From;
    Version To;
  };

  for (const Crossing crossing :
       {Crossing{.From = "r2018", .To = Version::R2000}, Crossing{.From = "r2000", .To = Version::R2018},
        Crossing{.From = "r2007", .To = Version::R2013}, Crossing{.From = "r2004", .To = Version::R14},
        Crossing{.From = "r2018", .To = Version::R2007}, Crossing{.From = "r14", .To = Version::R2010}})
  {
    for (const std::string_view name : {"kinds", "mleaders"})
    {
      CAPTURE(name);
      CAPTURE(crossing.From);
      CAPTURE(NameOf(crossing.To));
      const Drawing drawing = Load(ReadFixture(std::format("{}-{}.dwg", name, crossing.From))).value().Drawing;
      const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = crossing.To});
      REQUIRE(saved);
      for (const Issue &issue : saved->Issues)
      {
        CHECK(issue.What.find("MULTILEADER") == std::string::npos);
        CHECK(issue.What.find("MLEADERSTYLE") == std::string::npos);
      }
      const std::expected<Loaded, Failure> back = Load(saved->Bytes);
      REQUIRE(back);
      CHECK(back->Issues.empty());
      const Version lower = std::min(drawing.Release, crossing.To);
      const std::vector<MultiLeader> before = All<MultiLeader>(drawing);
      const std::vector<MultiLeader> after = All<MultiLeader>(back->Drawing);
      REQUIRE(after.size() == before.size());
      for (std::size_t at = 0; at < before.size(); ++at)
      {
        CHECK(Kept(after[at].MLeader, lower) == Kept(before[at].MLeader, lower));
        CHECK(after[at].MLeader.Version == 2);
      }
      const auto styles = All<Objects::MLeaderStyle>(back->Drawing);
      const auto were = All<Objects::MLeaderStyle>(drawing);
      REQUIRE(styles.size() == were.size());
      for (std::size_t at = 0; at < styles.size(); ++at)
      {
        CHECK(Kept(styles[at], lower) == Kept(were[at], lower));
        CHECK(styles[at].Revision == 2);
        CHECK(styles[at].Base.Extended.size() == (crossing.To < Version::R2010 ? 1 : 0));
      }
      QueueAudit(saved->Bytes, std::format("dwgio-{}-{}-as-{}.dwg", name, crossing.From, NameOf(crossing.To)));
    }
  }
}

TEST_CASE("entities/multileader: a reference to a missing style falls back and to a missing block is dropped")
{
  constexpr Handle Nowhere{.Value = 0x999};
  Drawing drawing = Load(ReadFixture("mleaders-r2018.dwg")).value().Drawing;
  const std::vector<MultiLeader> leaders = All<MultiLeader>(drawing);
  const Handle texted = leaders[0].Base.Self;
  const Handle blocked = leaders[1].Base.Self;
  const Handle upright = leaders[2].Base.Self;
  auto &text = Get<MultiLeader>(*Find(drawing, texted)).MLeader;
  text.Style = Nowhere;
  text.Arrowhead = Nowhere;
  text.Linetype = Nowhere;
  text.Context.TextStyle = Nowhere;
  text.Context.Leaders[0].Lines[0].Arrowhead = Nowhere;
  text.Arrowheads.push_back(MultiLeaders::Arrowhead{.Index = 1, .Block = Nowhere});
  Get<MultiLeader>(*Find(drawing, blocked)).MLeader.BlockContent = Nowhere;
  const Handle style = Get<MultiLeader>(*Find(drawing, upright)).MLeader.Style;
  auto &styled = Get<Objects::MLeaderStyle>(*Find(drawing, style));
  styled.Leader.Arrowhead = Nowhere;
  styled.Text.Style = Nowhere;
  styled.Block.Content = Nowhere;
  const Audit::Report checked = Audit::Check(drawing);
  std::vector<std::string> whats;
  for (const Audit::Finding &finding : checked.Findings)
  {
    whats.push_back(finding.What);
  }
  const std::string at = HexOf(texted);
  CHECK(std::ranges::contains(whats, std::format("multileader style 999 of MULTILEADER {} missing", at)));
  CHECK(std::ranges::contains(whats, std::format("arrowhead 999 of MULTILEADER {} missing", at)));
  CHECK(std::ranges::contains(whats, std::format("linetype 999 of MULTILEADER {} missing", at)));
  CHECK(std::ranges::contains(whats, std::format("text style 999 of MULTILEADER {} missing", at)));
  CHECK(std::ranges::contains(whats, std::format("block 999 of MULTILEADER {} missing, left out", HexOf(blocked))));
  CHECK(std::ranges::contains(whats, std::format("arrowhead 999 of MLEADERSTYLE {} missing", HexOf(style))));
  CHECK(std::ranges::contains(whats, std::format("text style 999 of MLEADERSTYLE {} missing", HexOf(style))));
  Audit::Repair(drawing);
  CHECK(Find(drawing, blocked) == nullptr);
  const MultiLeaders::MLeader &fixed = Get<MultiLeader>(*Find(drawing, texted)).MLeader;
  const Object *standard = View::Entry(drawing, *View::Dictionary(drawing, "ACAD_MLEADERSTYLE"), "Standard");
  REQUIRE(standard != nullptr);
  CHECK(fixed.Style == BaseOf(*standard).Self);
  CHECK(fixed.Arrowhead == Handle{});
  CHECK(fixed.Context.Leaders[0].Lines[0].Arrowhead == Handle{});
  CHECK(fixed.Arrowheads[0].Block == Handle{});
  CHECK(fixed.Context.TextStyle == View::Record<Tables::Style>(drawing, "Standard")->Record.Base.Self);
  const auto &restyled = Get<Objects::MLeaderStyle>(*Find(drawing, style));
  CHECK(restyled.Leader.Arrowhead == Handle{});
  CHECK(restyled.Block.Content == Handle{});
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("entities/multileader: without a Standard multileader style a missing style is left null")
{
  constexpr Handle Nowhere{.Value = 0x999};
  for (const bool dictionary : {false, true})
  {
    CAPTURE(dictionary);
    Drawing drawing = Load(ReadFixture("mleaders-r2018.dwg")).value().Drawing;
    const Handle styles = View::Dictionary(drawing, "ACAD_MLEADERSTYLE")->Base.Self;
    if (dictionary) // Its Standard entry naming no style but the dictionary itself.
    {
      for (Objects::Entry &entry : Get<Objects::Dictionary>(*Find(drawing, styles)).Entries)
      {
        entry.Object = entry.Name == "Standard" ? styles : entry.Object;
      }
    }
    else
    {
      std::erase_if(drawing.Objects, [styles](const Object &object) { return BaseOf(object).Self == styles; });
    }
    for (Object &object : drawing.Objects)
    {
      if (auto *leader = GetIf<MultiLeader>(&object))
      {
        leader->MLeader.Style = Nowhere;
      }
    }
    Audit::Repair(drawing);
    for (const MultiLeader &leader : All<MultiLeader>(drawing))
    {
      CHECK(leader.MLeader.Style == Handle{});
    }
  }
}

TEST_CASE("entities/multileader: AutoCAD's own saves across R2010 keep every multileader as dwgio's do" *
          doctest::skip(!IsVerifying()))
{
  for (const auto &[from, to] :
       {std::pair{std::string_view{"r2018"}, Version::R2007}, std::pair{std::string_view{"r2007"}, Version::R2018}})
  {
    CAPTURE(from);
    const Drawing drawing = Load(ReadFixture(std::format("mleaders-{}.dwg", from))).value().Drawing;
    const std::string name = std::format("dwgio-mleaders-{}-as-{}", from, NameOf(to));
    const std::expected<Loaded, Failure> theirs =
        Load(Resave(Save(drawing, SaveOptions{.Release = to}).value().Bytes, name));
    REQUIRE(theirs);
    const std::vector<MultiLeader> ours = All<MultiLeader>(drawing);
    const std::vector<MultiLeader> kept = All<MultiLeader>(theirs->Drawing);
    REQUIRE(kept.size() == ours.size());
    for (std::size_t at = 0; at < ours.size(); ++at)
    {
      CHECK(Kept(kept[at].MLeader, Version::R2000) == Kept(ours[at].MLeader, Version::R2000));
    }
  }
}

TEST_CASE("entities/multileader: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Entities
