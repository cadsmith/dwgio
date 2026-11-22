/***********************************************************************************************************************
 *  @file      tests/dwgio/save/viewport.cpp
 *  @brief     A viewport saved as R14: its view in the MVIEW extended data, and back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/viewport.hpp"

#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO
{
TEST_CASE("save/viewport: a viewport saved as R14 keeps its view in the ACAD MVIEW xdata")
{
  for (const std::string_view name : {"curves", "objects"})
  {
    CAPTURE(name);
    const Drawing drawing = Load(ReadFixture(std::format("{}-r2000.dwg", name))).value().Drawing;
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R14});
    REQUIRE(saved);
    const Drawing ours = Load(saved->Bytes).value().Drawing;
    const Drawing theirs = Load(ReadFixture(std::format("{}-r14.dwg", name))).value().Drawing;
    std::size_t viewports = 0;
    for (const Object &object : ours.Objects)
    {
      if (const auto *viewport = GetIf<Entities::Viewport>(&object))
      {
        CAPTURE(HexOf(viewport->Base.Self));
        CHECK(*viewport == Get<Entities::Viewport>(*Find(theirs, viewport->Base.Self)));
        ++viewports;
      }
    }
    CHECK(viewports >= 4);
    CHECK(std::ranges::any_of(saved->Issues,
                              [](const Issue &issue) -> bool { return issue.What.starts_with("clip of VIEWPORT"); }));
    QueueAudit(saved->Bytes, std::format("dwgio-{}-r2000-as-r14.dwg", name));
  }
}

TEST_CASE("save/viewport: a viewport's isopair and frozen layers and other xdata convert to R14 and back")
{
  Drawing drawing = Load(ReadFixture("curves-r2000.dwg")).value().Drawing;
  const Handle acad = View::Record<Tables::AppId>(drawing, "ACAD")->Record.Base.Self;
  std::vector<Handle> handles;
  int pair = 0;
  for (Object &object : drawing.Objects)
  {
    if (auto *viewport = GetIf<Entities::Viewport>(&object); viewport != nullptr && handles.size() < 2)
    {
      viewport->Flags |= pair++ == 0 ? 4096 : 8192; // The top isopair, then the right one.
      // Extended data of another shape, which no translation takes for R14's view: a short first, and nothing.
      viewport->Base.Extended.push_back(
          Xdata{.Application = {}, .AppId = acad, .Tags = {Tag{.Code = 1000, .Value = std::string{"Note"}}}});
      viewport->Base.Extended.push_back(Xdata{.Application = {}, .AppId = acad, .Tags = {}});
      handles.push_back(viewport->Base.Self);
    }
  }
  REQUIRE(handles.size() == 2);
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R14});
  REQUIRE(saved);
  CHECK(std::ranges::any_of(saved->Issues, [](const Issue &issue) -> bool
                            { return issue.What.ends_with("with no groups left out"); }));
  const Drawing lowered = Load(saved->Bytes).value().Drawing;
  const Drawing back = Load(Save(lowered, SaveOptions{.Release = Version::R2000})->Bytes).value().Drawing;
  const auto &first = Get<Entities::Viewport>(*Find(back, handles[0]));
  CHECK((first.Flags & 4096) != 0);
  CHECK((Get<Entities::Viewport>(*Find(back, handles[1])).Flags & 8192) != 0);
  REQUIRE(first.Base.Extended.size() == 1);
  CHECK(first.Base.Extended.front().Tags == std::vector<Tag>{Tag{.Code = 1000, .Value = std::string{"Note"}}});
}

TEST_CASE("save/viewport: an R14 viewport's isopair is 1 for top and 2 for right with no zoom locking")
{
  // The isopair short of R14's MVIEW extended data, after the name, the brace, the version, six coordinates, seven
  // reals and seven shorts.
  constexpr std::size_t Isopair = 23;
  for (const auto &[flag, isopair] : {std::pair{0, std::int64_t{0}}, std::pair{4096, std::int64_t{1}},
                                      std::pair{8192, std::int64_t{2}}, std::pair{16384, std::int64_t{0}}})
  {
    CAPTURE(flag);
    Drawing drawing = Load(ReadFixture("curves-r2000.dwg")).value().Drawing;
    Handle self{};
    for (Object &object : drawing.Objects)
    {
      if (auto *viewport = GetIf<Entities::Viewport>(&object); viewport != nullptr && self == Handle{})
      {
        viewport->Flags = (viewport->Flags & ~(4096 | 8192 | 16384)) | flag;
        self = viewport->Base.Self;
      }
    }
    REQUIRE(self != Handle{});
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R14});
    REQUIRE(saved);
    QueueAudit(saved->Bytes, std::format("dwgio-isopair-{}-as-r14.dwg", flag));
    const Drawing lowered = Load(saved->Bytes).value().Drawing;
    const auto &viewport = Get<Entities::Viewport>(*Find(lowered, self));
    REQUIRE(!viewport.Base.Extended.empty());
    const std::vector<Tag> &groups = viewport.Base.Extended.back().Tags;
    REQUIRE(groups.size() > Isopair);
    CHECK(groups.front().Value == Content{std::string{"MVIEW"}});
    CHECK(groups[Isopair].Value == Content{isopair});
    const Drawing back = Load(Save(lowered, SaveOptions{.Release = Version::R2000})->Bytes).value().Drawing;
    CHECK((Get<Entities::Viewport>(*Find(back, self)).Flags & (4096 | 8192 | 16384)) == (flag & (4096 | 8192)));
  }
}

TEST_CASE("save/viewport: an R14 viewport's MVIEW xdata of another shape stays and one with no ACAD is noted")
{
  Drawing drawing = Load(ReadFixture("objects-r14.dwg")).value().Drawing;
  std::vector<Entities::Viewport *> viewports;
  for (Object &object : drawing.Objects)
  {
    if (auto *viewport = GetIf<Entities::Viewport>(&object))
    {
      viewports.push_back(viewport);
    }
  }
  REQUIRE(viewports.size() >= 3);
  // Cut short, with text where a real or a short belongs: none is read, each stays as it was.
  REQUIRE(viewports.size() >= 4);
  viewports[0]->Base.Extended.front().Tags.resize(20);
  viewports[1]->Base.Extended.front().Tags[5].Value = std::string{"x"};
  viewports[3]->Base.Extended.front().Tags[16].Value = std::string{"x"};
  // A frozen layer given by name, where R14 gives a handle, is passed over and the rest read.
  std::vector<Tag> &read = viewports[2]->Base.Extended.front().Tags;
  read.insert(read.begin() + 33, Tag{.Code = 1003, .Value = std::string{"Walls"}});
  const std::vector<Xdata> cut = viewports[0]->Base.Extended;
  const std::vector<Xdata> odd = viewports[1]->Base.Extended;
  const Drawing raised = Load(Save(drawing, SaveOptions{.Release = Version::R2000})->Bytes).value().Drawing;
  CHECK(Get<Entities::Viewport>(*Find(raised, viewports[0]->Base.Self)).Base.Extended.size() == cut.size());
  CHECK(Get<Entities::Viewport>(*Find(raised, viewports[1]->Base.Self)).Base.Extended.size() == odd.size());
  CHECK(Get<Entities::Viewport>(*Find(raised, viewports[2]->Base.Self)).Base.Extended.empty());
  CHECK(Get<Entities::Viewport>(*Find(raised, viewports[3]->Base.Self)).Base.Extended.size() == 1);
  // An R14 viewport with no view in its extended data upgrades with the fields' defaults.
  Drawing plain = Load(ReadFixture("objects-r14.dwg")).value().Drawing;
  Entities::Viewport *unviewed = nullptr;
  for (Object &object : plain.Objects)
  {
    unviewed = unviewed == nullptr ? GetIf<Entities::Viewport>(&object) : unviewed;
  }
  REQUIRE(unviewed != nullptr);
  unviewed->Base.Extended.clear();
  const Drawing upgraded = Load(Save(plain, SaveOptions{.Release = Version::R2000})->Bytes).value().Drawing;
  CHECK(Get<Entities::Viewport>(*Find(upgraded, unviewed->Base.Self)).Zoom == 0);
  // Lowered without an ACAD application to hang the view on, the viewport is noted.
  Drawing bare = Load(ReadFixture("objects-r2000.dwg")).value().Drawing;
  std::erase_if(bare.Objects,
                [](const Object &object) -> bool
                {
                  const auto *appid = GetIf<Tables::AppId>(&object);
                  return appid != nullptr && appid->Record.Name == "ACAD";
                });
  const std::expected<Saved, Failure> lowered = Save(bare, SaveOptions{.Release = Version::R14});
  REQUIRE(lowered);
  CHECK(std::ranges::any_of(lowered->Issues, [](const Issue &issue) -> bool
                            { return issue.What.starts_with("application ACAD missing for the view of VIEWPORT"); }));
}

TEST_CASE("save/viewport: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
