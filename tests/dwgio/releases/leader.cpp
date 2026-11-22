/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/leader.cpp
 *  @brief     A leader's fields AutoCAD derives when it saves across R14, R2000 and R2010.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/leader.hpp"

#include "releases/lookup.hpp"
#include "releases/object.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/base.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/leader.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The groups of each extended data record in order: the handles of the applications differ between files.
 */
auto Groups(const Base &base) -> std::vector<std::vector<Tag>>
{
  std::vector<std::vector<Tag>> groups;
  for (const Xdata &xdata : base.Extended)
  {
    groups.push_back(xdata.Tags);
  }
  return groups;
}

/**
 *  Each leader of a drawing translated to a release, with its twin in AutoCAD's save to that release; the count.
 */
template <typename CheckT>
auto CompareTwins(const Drawing &from, const Drawing &to, CheckT check) -> std::size_t
{
  std::size_t compared = 0;
  for (const Object &object : from.Objects)
  {
    if (!Holds<Entities::Leader>(object))
    {
      continue;
    }
    const Entities::Leader &leader = Get<Entities::Leader>(object);
    CAPTURE(leader.Base.Self.Value);
    const Translation translated = ConvertObject(object, from, to.Release);
    CHECK(translated.Issues.empty());
    const auto *twin = GetIf<Entities::Leader>(Find(to, leader.Base.Self));
    REQUIRE(twin != nullptr);
    check(Get<Entities::Leader>(translated.Object), *twin);
    ++compared;
  }
  return compared;
}

void SameFourteen(const Entities::Leader &ours, const Entities::Leader &theirs)
{
  CHECK(ours.Gap == theirs.Gap);
  CHECK(ours.Dimasz == theirs.Dimasz);
  CHECK(ours.Color == theirs.Color);
  CHECK(ours.Arrowhead == theirs.Arrowhead);
  CHECK(ours.Unknown2 == theirs.Unknown2);
  CHECK(ours.Unknown3 == theirs.Unknown3);
  CHECK(ours.RepeatedColor == theirs.RepeatedColor);
  CHECK(ours.Unknown5 == theirs.Unknown5);
  CHECK(ours.Unknown6 == theirs.Unknown6);
  CHECK(ours.Height == theirs.Height);
  CHECK(ours.Width == theirs.Width);
  CHECK(Groups(ours.Base) == Groups(theirs.Base));
}

/**
 *  A drawing with one leader of no annotation in model space, and its handle.
 */
struct Made
{
  DwgIO::Drawing Drawing;
  Handle Leader;
};

auto Make(const Entities::Leader &leader) -> Made
{
  Drawing drawing = Create();
  const std::expected<Handle, Failure> added = Edit::Add(drawing, Object{leader});
  REQUIRE(added);
  return Made{.Drawing = std::move(drawing), .Leader = *added};
}

auto LowerToR14(const Made &made) -> Translation
{
  return ConvertObject(*Find(made.Drawing, made.Leader), made.Drawing, Version::R14);
}
} // namespace

TEST_CASE("releases/leader: a leader saved as R14 gets the fields AutoCAD's R14 save derives")
{
  const Drawing older = LoadDrawing("leaders-r14.dwg");
  for (const std::string_view name : {"leaders-r2000.dwg", "leaders-r2018.dwg"})
  {
    CAPTURE(name);
    CHECK(CompareTwins(LoadDrawing(name), older, SameFourteen) == 10);
  }
  CHECK(CompareTwins(LoadDrawing("curves-r2000.dwg"), LoadDrawing("curves-r14.dwg"), SameFourteen) == 4);
}

TEST_CASE("releases/leader: a leader upgraded from R14 matches AutoCAD's save")
{
  const Drawing older = LoadDrawing("leaders-r14.dwg");
  const std::size_t compared = CompareTwins(older, LoadDrawing("leaders-r14-as-r2000.dwg"),
                                            [](const Entities::Leader &ours, const Entities::Leader &theirs) -> void
                                            {
                                              CHECK(ours.ArrowheadType == theirs.ArrowheadType);
                                              CHECK(ours.Gap == 0);
                                              CHECK(ours.Dimasz == 0);
                                              CHECK(ours.Color == 0);
                                              CHECK(ours.Arrowhead == 0);
                                              CHECK(!ours.Unknown3);
                                              CHECK(ours.RepeatedColor == 0);
                                              CHECK(ours.Height == theirs.Height);
                                              CHECK(Groups(ours.Base) == Groups(theirs.Base));
                                            });
  CHECK(compared == 10);
}

TEST_CASE("releases/leader: saving below R2010 derives the annotation box and upgrading drops it")
{
  const Drawing older = LoadDrawing("leaders-r2007.dwg");
  const auto boxed = [](const Entities::Leader &ours, const Entities::Leader &theirs) -> void
  {
    CHECK(ours.Height == theirs.Height);
    CHECK(ours.Width == theirs.Width);
  };
  CHECK(CompareTwins(LoadDrawing("leaders-r2010.dwg"), older, boxed) == 10);
  CHECK(CompareTwins(LoadDrawing("leaders-r2018.dwg"), older, boxed) == 10);
  CHECK(CompareTwins(older, LoadDrawing("leaders-r2010.dwg"), boxed) == 10);
}

TEST_CASE("releases/leader: leaders saved across R2000 and R2010 load back clean")
{
  for (const auto &[from, release] :
       {std::pair{"leaders-r2018", Version::R14}, std::pair{"leaders-r2018", Version::R2007},
        std::pair{"leaders-r14", Version::R2018}})
  {
    const std::string name{from};
    CAPTURE(name);
    CAPTURE(NameOf(release));
    const std::expected<Saved, Failure> saved = Save(LoadDrawing(name + ".dwg"), SaveOptions{.Release = release});
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    QueueAudit(saved->Bytes, "dwgio-" + name + "-as-" + std::string{NameOf(release)} + ".dwg");
  }
}

TEST_CASE("releases/leader: a leader without a style takes the header's and one with its own xdata gets DSTYLE")
{
  Entities::Leader leader;
  leader.Points = {Vector{}, Vector{.X = 1}};
  leader.Common.Color.Index = 0;
  Made made = Make(leader);
  Header &header = made.Drawing.Header;
  header.Dimscale = 0;
  header.Dimasz = 3;
  header.Dimgap = 0.5;
  header.Dimclrd.Index = 0;
  const Handle acad = View::Record<Tables::AppId>(made.Drawing, "ACAD")->Record.Base.Self;
  auto &own = Get<Entities::Leader>(*Find(made.Drawing, made.Leader));
  own.Style = {};
  // An ACAD record without DSTYLE, after another application's.
  own.Base.Extended = {Xdata{.Application = {}, .AppId = Handle{0x99}, .Tags = {Tag{.Code = 1000, .Value = "x"}}},
                       Xdata{.Application = {}, .AppId = acad, .Tags = {Tag{.Code = 1000, .Value = "kept"}}}};
  const Translation translated = LowerToR14(made);
  CHECK(translated.Issues.empty());
  const auto &lower = Get<Entities::Leader>(translated.Object);
  CHECK(lower.Gap == 0.5);
  CHECK(lower.Dimasz == 3);
  CHECK(lower.Color == 7);
  CHECK(lower.RepeatedColor == 7);
  CHECK(lower.Unknown3);
  CHECK(lower.Arrowhead == 0);
  CHECK(lower.Height == 0); // No annotation.
  REQUIRE(lower.Base.Extended.size() == 2);
  CHECK(lower.Base.Extended.back().AppId == acad);
  CHECK(lower.Base.Extended.back().Tags ==
        std::vector<Tag>{Tag{.Code = 1000, .Value = "kept"}, Tag{.Code = 1000, .Value = "DSTYLE"},
                         Tag{.Code = 1002, .Value = "{"}, Tag{.Code = 1070, .Value = std::int64_t{5}},
                         Tag{.Code = 1000, .Value = ""}, Tag{.Code = 1002, .Value = "}"}});
}

TEST_CASE("releases/leader: a leader's overrides are read up to the first non-override")
{
  Made made = Make(Entities::Leader{.Points = {Vector{}, Vector{.X = 1}}});
  const std::expected<Handle, Failure> block = Edit::AddBlock(made.Drawing, "_Tick");
  REQUIRE(block);
  const Handle acad = View::Record<Tables::AppId>(made.Drawing, "ACAD")->Record.Base.Self;
  auto &own = Get<Entities::Leader>(*Find(made.Drawing, made.Leader));
  own.Base.Extended = {
      Xdata{.Application = {},
            .AppId = acad,
            .Tags = {Tag{.Code = 1000, .Value = "DSTYLE"}, Tag{.Code = 1002, .Value = "{"},
                     Tag{.Code = 1070, .Value = std::int64_t{341}}, Tag{.Code = 1005, .Value = *block},
                     Tag{.Code = 1070, .Value = std::int64_t{342}}, Tag{.Code = 1005, .Value = Handle{0x99}},
                     Tag{.Code = 1070, .Value = std::int64_t{176}}, Tag{.Code = 1070, .Value = std::int64_t{3}},
                     Tag{.Code = 1070, .Value = std::int64_t{147}}, Tag{.Code = 1070, .Value = std::int64_t{2}},
                     Tag{.Code = 1000, .Value = "odd"}, Tag{.Code = 1070, .Value = std::int64_t{41}},
                     Tag{.Code = 1040, .Value = 9.0}, Tag{.Code = 1002, .Value = "}"}}}};
  const Translation translated = LowerToR14(made);
  const auto &lower = Get<Entities::Leader>(translated.Object);
  CHECK(lower.Arrowhead == 8);
  CHECK(lower.Color == 3);
  CHECK(lower.Dimasz == 2.5); // The 41 after the text is no override.
  const std::vector<Tag> &groups = lower.Base.Extended.front().Tags;
  REQUIRE(groups.size() == 16);
  CHECK(groups[2] == Tag{.Code = 1070, .Value = std::int64_t{5}});
  CHECK(groups[3] == Tag{.Code = 1000, .Value = "TICK"});
}

TEST_CASE("releases/leader: a DSTYLE text that opens no brace gives no overrides")
{
  for (const std::vector<Tag> &kept :
       {std::vector<Tag>{Tag{.Code = 1000, .Value = "DSTYLE"}},
        std::vector<Tag>{Tag{.Code = 1000, .Value = "DSTYLE"}, Tag{.Code = 1000, .Value = "x"}},
        std::vector<Tag>{Tag{.Code = 1070, .Value = std::int64_t{7}}, Tag{.Code = 1000, .Value = "DSTYLE"}}})
  {
    Made made = Make(Entities::Leader{.Points = {Vector{}, Vector{.X = 1}}});
    const Handle acad = View::Record<Tables::AppId>(made.Drawing, "ACAD")->Record.Base.Self;
    Get<Entities::Leader>(*Find(made.Drawing, made.Leader)).Base.Extended = {
        Xdata{.Application = {}, .AppId = acad, .Tags = kept}};
    const Translation translated = LowerToR14(made);
    const std::vector<Tag> &groups = Get<Entities::Leader>(translated.Object).Base.Extended.front().Tags;
    REQUIRE(groups.size() == kept.size() + 5);
    CHECK(groups[kept.size()] == Tag{.Code = 1000, .Value = "DSTYLE"});
  }
}

TEST_CASE("releases/leader: a leader in a drawing without the ACAD application keeps its xdata with an issue")
{
  Made made = Make(Entities::Leader{.Points = {Vector{}, Vector{.X = 1}}});
  std::erase_if(made.Drawing.Objects,
                [](const Object &object) -> bool
                {
                  const auto *appid = GetIf<Tables::AppId>(&object);
                  return appid != nullptr && appid->Record.Name == "ACAD";
                });
  const Translation translated = LowerToR14(made);
  REQUIRE(translated.Issues.size() == 1);
  CHECK(translated.Issues.front().What.contains("application ACAD missing for the arrow of LEADER"));
  CHECK(Get<Entities::Leader>(translated.Object).Base.Extended.empty());
  CHECK(Get<Entities::Leader>(translated.Object).Gap == 0.625);
}

TEST_CASE("releases/leader: only an MTEXT annotation gives the box")
{
  Drawing drawing = Create();
  Entities::MText text;
  text.ExtentHeight = 2;
  text.ExtentWidth = 5;
  const std::expected<Handle, Failure> note = Edit::Add(drawing, Object{text});
  REQUIRE(note);
  Entities::Leader leader{.Annotation = 0, .Points = {Vector{}, Vector{.X = 1}}, .Note = *note};
  const Lookup found{drawing};
  BoxLeader(leader, found);
  CHECK(leader.Height == 2);
  CHECK(leader.Width == 5);
  leader.Annotation = 1;
  BoxLeader(leader, found);
  CHECK(leader.Height == 0);
  leader.Annotation = 0;
  leader.Note = Handle{0x7FFF};
  leader.Height = 4;
  BoxLeader(leader, found);
  CHECK(leader.Height == 0);
}

TEST_CASE("releases/leader: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
