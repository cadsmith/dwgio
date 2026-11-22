/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/roundtrip.cpp
 *  @brief     Visual and view styles across releases, through the round-trip record AutoCAD keeps.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/roundtrip.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "releases/untyped.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"
#include "support/pass.hpp"
#include "support/styles.hpp"

#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The first object of a type in a drawing.
 */
auto First(Drawing &drawing, std::string_view type) -> Object &
{
  const auto found =
      std::ranges::find_if(drawing.Objects, [type](const Object &object) -> bool { return TypeOf(object) == type; });
  REQUIRE(found != drawing.Objects.end());
  return *found;
}
} // namespace

TEST_CASE("releases/roundtrip: every visual and view style saved to another release matches AutoCAD's")
{
  for (const std::string_view name : {"visualstyles", "kinds", "material"})
  {
    for (const Version from : FixtureReleases)
    {
      const Drawing source = LoadFixture(FileOf(name, from)).Drawing;
      for (const Version to : FixtureReleases)
      {
        if (from == to)
        {
          continue;
        }
        CAPTURE(std::format("{} {} as {}", name, NameOf(from), NameOf(to)));
        const std::expected<Saved, Failure> saved = Save(source, SaveOptions{.Release = to});
        REQUIRE(saved);
        CHECK(KeepsStyles(source, saved->Issues));
        const std::expected<Loaded, Failure> back = Load(saved->Bytes);
        REQUIRE(back);
        CHECK(back->Issues.empty());
        CheckStyles(back->Drawing, LoadFixture(FileOf(name, to)).Drawing, source);
      }
    }
  }
}

TEST_CASE("releases/roundtrip: the visualstyles fixture saves across the layouts and R2018")
{
  struct Crossing
  {
    Version From;
    Version To;
  };

  for (const Crossing crossing :
       {Crossing{.From = Version::R2018, .To = Version::R14}, Crossing{.From = Version::R2018, .To = Version::R2007},
        Crossing{.From = Version::R2018, .To = Version::R2010}, Crossing{.From = Version::R2018, .To = Version::R2013},
        Crossing{.From = Version::R2013, .To = Version::R2018}, Crossing{.From = Version::R2010, .To = Version::R2004},
        Crossing{.From = Version::R2007, .To = Version::R2018}, Crossing{.From = Version::R14, .To = Version::R2018},
        Crossing{.From = Version::R2000, .To = Version::R2010}})
  {
    CAPTURE(std::format("{} as {}", NameOf(crossing.From), NameOf(crossing.To)));
    const std::expected<Saved, Failure> saved =
        Save(LoadFixture(FileOf("visualstyles", crossing.From)).Drawing, SaveOptions{.Release = crossing.To});
    REQUIRE(saved);
    QueueAudit(saved->Bytes,
               std::format("dwgio-visualstyles-{}-as-{}.dwg", NameOf(crossing.From), NameOf(crossing.To)));
  }
}

TEST_CASE("releases/roundtrip: nothing is rebuilt within a release or where no style keeps anything elsewhere")
{
  const Drawing newer = LoadFixture(FileOf("visualstyles", Version::R2018)).Drawing;
  CHECK_FALSE(Pass(KeepRoundTrips, newer, Version::R2018));
  CHECK_FALSE(Pass(KeepRoundTrips, Create(Version::R2018), Version::R2004));
  Drawing visual = LoadFixture(FileOf("visualstyles", Version::R2013)).Drawing;
  std::erase_if(visual.Objects, [](const Object &object) { return TypeOf(object).ends_with("VIEWSTYLE"); });
  CHECK_FALSE(Pass(KeepRoundTrips, visual, Version::R2018));
  CHECK(Pass(KeepRoundTrips, visual, Version::R2010));
}

TEST_CASE("releases/roundtrip: a record joins and leaves an extension dictionary holding other entries")
{
  Drawing drawing = LoadFixture(FileOf("visualstyles", Version::R2018)).Drawing;
  auto &style = Get<Objects::VisualStyle>(First(drawing, "VISUALSTYLE"));
  const Handle self = style.Base.Self;
  const Handle owned{.Value = 0x7000};
  const Handle mine{.Value = 0x7001};
  style.Base.Dictionary = owned;
  drawing.Objects.emplace_back(Objects::Dictionary{.Base = {.Self = owned, .Owner = self},
                                                   .HardOwner = true,
                                                   .Cloning = 1,
                                                   .Entries = {{.Name = "MINE", .Object = mine}},
                                                   .Default = {}});
  drawing.Objects.emplace_back(Objects::XRecord{.Base = {.Self = mine, .Owner = owned, .Reactors = {owned}},
                                                .Cloning = 1,
                                                .Data = {Tag{.Code = 1, .Value = std::string{"kept"}}},
                                                .Objects = {},
                                                .Rest = {}});
  const std::optional<Drawing> lowered = Pass(KeepRoundTrips, drawing, Version::R2010);
  REQUIRE(lowered);
  const auto &downgraded = Get<Objects::VisualStyle>(*Find(*lowered, self));
  CHECK(downgraded.Base.Dictionary == owned);
  const auto &entries = Get<Objects::Dictionary>(*Find(*lowered, owned)).Entries;
  REQUIRE(entries.size() == 2);
  CHECK(entries.back().Name == "ACAD_XREC_ROUNDTRIP");
  CHECK(Roundtrip(*lowered, downgraded.Base));
  // Back to R2018 the record is dropped, the dictionary stays with its own.
  Drawing older = *lowered;
  older.Release = Version::R2010;
  const std::optional<Drawing> raised = Pass(KeepRoundTrips, older, Version::R2018);
  REQUIRE(raised);
  CHECK(Get<Objects::VisualStyle>(*Find(*raised, self)).Base.Dictionary == owned);
  CHECK(Get<Objects::Dictionary>(*Find(*raised, owned)).Entries.size() == 1);
  CHECK(Find(*raised, entries.back().Object) == nullptr);
  // An entry of the record's name that names no record is given the one made; no dictionary at all is no record.
  Drawing odd = drawing;
  Get<Objects::Dictionary>(*Find(odd, owned)).Entries.front() = {.Name = "ACAD_XREC_ROUNDTRIP", .Object = self};
  const std::optional<Drawing> repointed = Pass(KeepRoundTrips, odd, Version::R2010);
  REQUIRE(repointed);
  const auto &one = Get<Objects::Dictionary>(*Find(*repointed, owned)).Entries;
  REQUIRE(one.size() == 1);
  CHECK(one.front().Object != self);
  CHECK(Roundtrip(*repointed, Get<Objects::VisualStyle>(*Find(*repointed, self)).Base));
  Drawing unlisted = drawing;
  Get<Objects::VisualStyle>(*Find(unlisted, self)).Base.Dictionary = mine; // Not a dictionary.
  const std::optional<Drawing> made = Pass(KeepRoundTrips, unlisted, Version::R2004);
  REQUIRE(made);
  CHECK(Get<Objects::VisualStyle>(*Find(*made, self)).Base.Dictionary != mine);
  unlisted.Release = Version::R2004;
  const std::optional<Drawing> bare = Pass(KeepRoundTrips, unlisted, Version::R2018);
  REQUIRE(bare);
  CHECK(Get<Objects::VisualStyle>(*Find(*bare, self)).Base.Dictionary == mine);
  older.Release = Version::R2010;
  Get<Objects::Dictionary>(*Find(older, owned)).Entries.pop_back();
  const std::optional<Drawing> without = Pass(KeepRoundTrips, older, Version::R2018);
  REQUIRE(without);
  CHECK(Get<Objects::Dictionary>(*Find(*without, owned)).Entries.size() == 1);
}

TEST_CASE("releases/roundtrip: a view style upgraded to R2018 without its record takes its name and no flags")
{
  Drawing drawing = LoadFixture(FileOf("visualstyles", Version::R2013)).Drawing;
  Object &detail = First(drawing, "ACDBDETAILVIEWSTYLE");
  auto &unknown = Get<Unknown>(detail);
  const Handle self = unknown.Base.Self;
  const Handle owner = unknown.Base.Owner;
  unknown.Base.Dictionary = {};
  const std::optional<Drawing> raised = Pass(KeepRoundTrips, drawing, Version::R2018);
  REQUIRE(raised);
  const auto &upgraded = Get<Unknown>(*Find(*raised, self));
  REQUIRE(upgraded.Body);
  CHECK(upgraded.Body->Release == Version::R2018);
  std::vector<Content> taken;
  const Encoding::Scheme scheme = Encoding::MakeScheme(Encoding::Standard());
  REQUIRE(
      Carry(upgraded,
            CarryContext{.From = Version::R2018, .To = Version::R2013, .Text = &scheme, .Given = {}, .Taken = &taken}));
  const auto &listed = Get<Objects::Dictionary>(*Find(drawing, owner)).Entries;
  const auto entry = std::ranges::find(listed, self, &Objects::Entry::Object);
  REQUIRE(entry != listed.end());
  CHECK(taken == std::vector<Content>{entry->Name, std::int64_t{0}});
  // A record whose markers are no text gives nothing.
  Drawing marked = drawing;
  const Handle owned{.Value = 0x7000};
  const Handle kept{.Value = 0x7001};
  Get<Unknown>(*Find(marked, self)).Base.Dictionary = owned;
  marked.Objects.emplace_back(Objects::Dictionary{.Base = {.Self = owned, .Owner = self},
                                                  .HardOwner = true,
                                                  .Cloning = 1,
                                                  .Entries = {{.Name = "ACAD_XREC_ROUNDTRIP", .Object = kept}},
                                                  .Default = {}});
  marked.Objects.emplace_back(Objects::XRecord{
      .Base = {.Self = kept, .Owner = owned, .Reactors = {owned}},
      .Cloning = 1,
      .Data = {Tag{.Code = 102, .Value = std::int64_t{1}}, Tag{.Code = 1, .Value = std::string{"DISPLAYNAME"}}},
      .Objects = {},
      .Rest = {}});
  const std::optional<Drawing> unmarked = Pass(KeepRoundTrips, marked, Version::R2018);
  REQUIRE(unmarked);
  taken.clear();
  REQUIRE(
      Carry(Get<Unknown>(*Find(*unmarked, self)),
            CarryContext{.From = Version::R2018, .To = Version::R2013, .Text = &scheme, .Given = {}, .Taken = &taken}));
  CHECK(taken.front() == Content{entry->Name});
  // Owned by no dictionary, or by one that does not list it, it has no name.
  for (const Handle other : {self, drawing.Header.DictionaryNamedObjects})
  {
    Get<Unknown>(*Find(drawing, self)).Base.Owner = other;
    const std::optional<Drawing> nameless = Pass(KeepRoundTrips, drawing, Version::R2018);
    REQUIRE(nameless);
    taken.clear();
    REQUIRE(Carry(
        Get<Unknown>(*Find(*nameless, self)),
        CarryContext{.From = Version::R2018, .To = Version::R2013, .Text = &scheme, .Given = {}, .Taken = &taken}));
    CHECK(taken.front() == Content{std::string{}});
  }
}

TEST_CASE("releases/roundtrip: a view style whose bits do not read is left for the save to drop")
{
  for (const Version from : {Version::R2018, Version::R2013})
  {
    CAPTURE(NameOf(from));
    Drawing drawing = LoadFixture(FileOf("visualstyles", from)).Drawing;
    auto &unknown = Get<Unknown>(First(drawing, "ACDBSECTIONVIEWSTYLE"));
    const Handle self = unknown.Base.Self;
    unknown.Body->Data.Size -= 2;
    const Unknown before = unknown;
    const std::optional<Drawing> made =
        Pass(KeepRoundTrips, drawing, from == Version::R2018 ? Version::R2010 : Version::R2018);
    REQUIRE(made);
    CHECK(Get<Unknown>(*Find(*made, self)) == before);
  }
  // Bits already of the release written give nothing to take into a record.
  Drawing drawing = LoadFixture(FileOf("visualstyles", Version::R2018)).Drawing;
  auto &unknown = Get<Unknown>(First(drawing, "ACDBDETAILVIEWSTYLE"));
  unknown.Body->Release = Version::R2013;
  const Unknown before = unknown;
  const std::optional<Drawing> made = Pass(KeepRoundTrips, drawing, Version::R2013);
  REQUIRE(made);
  CHECK(Get<Unknown>(*Find(*made, before.Base.Self)) == before);
}

TEST_CASE("releases/roundtrip: a style without a handle or with a repeated handle stays")
{
  Drawing drawing = LoadFixture(FileOf("visualstyles", Version::R2013)).Drawing;
  const Object visual = First(drawing, "VISUALSTYLE");
  Object view = First(drawing, "ACDBDETAILVIEWSTYLE");
  Object unhandled = visual;
  BaseOf(unhandled).Self = {};
  BaseOf(view).Self = drawing.Header.DictionaryNamedObjects; // A dictionary bears it first.
  drawing.Objects.push_back(unhandled);
  drawing.Objects.push_back(view);
  // A surface of the older name in a drawing of R2013 has no record of a release below it to read away.
  Object surface = Entities::Surface{};
  BaseOf(surface).Self = Edit::MakeHandle(drawing);
  drawing.Objects.push_back(surface);
  for (const Version to : {Version::R2010, Version::R2018})
  {
    CAPTURE(NameOf(to));
    const std::optional<Drawing> made = Pass(KeepRoundTrips, drawing, to);
    REQUIRE(made);
    CHECK(std::ranges::contains(made->Objects, unhandled));
    CHECK(std::ranges::contains(made->Objects, view));
  }
}

TEST_CASE("releases/roundtrip: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
