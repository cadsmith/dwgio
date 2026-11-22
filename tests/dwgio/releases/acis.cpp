/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/acis.cpp
 *  @brief     Solids, surfaces and their histories across R2007 and R2013.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/acis.hpp"

#include "entities/shapes/geometry.hpp"
#include "octets.hpp"
#include "releases/bridge.hpp"
#include "releases/object.hpp"
#include "releases/omission.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/common.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/symbol.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
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
 *  Whether an object carries the extended data AutoCAD gives an extruded or swept surface before R2007.
 */
auto CountVersionData(const Object &object) -> std::size_t
{
  return static_cast<std::size_t>(std::ranges::count_if(
      BaseOf(object).Extended,
      [](const Xdata &xdata) -> bool
      {
        const std::string *text = xdata.Tags.empty() ? nullptr : std::get_if<std::string>(&xdata.Tags[0].Value);
        return text != nullptr && *text == "AcDbSavedByObjectVersion";
      }));
}
} // namespace

TEST_CASE("releases/acis: a solid saved below R2007 drops its history and the geometry's later fields")
{
  Drawing drawing = Create(Version::R2013);
  Entities::Solid3d solid;
  solid.Base.Self = Handle{0x500};
  solid.History = Handle{0x501};
  solid.Geometry.Unknown1 = 1;
  solid.Geometry.Stored = true;
  solid.Geometry.Guid = std::string{Entities::Shapes::NoGuid};
  solid.Geometry.Unknown2 = 2;
  const Translation lowered = ConvertObject(Object{solid}, drawing, Version::R2004);
  const auto &low = Get<Entities::Solid3d>(lowered.Object);
  CHECK_FALSE(low.History);
  CHECK(low.Geometry.Unknown1 == 0);
  CHECK_FALSE(low.Geometry.Stored);
  CHECK(low.Geometry.Guid.empty());
  CHECK(low.Geometry.Unknown2 == 0);
  const Translation kept = ConvertObject(Object{solid}, drawing, Version::R2007);
  CHECK(Get<Entities::Solid3d>(kept.Object).History == solid.History);
  CHECK(Get<Entities::Solid3d>(kept.Object).Geometry.Unknown1 == 1);
}

TEST_CASE("releases/acis: a SURFACE becomes a NURBS surface from R2013 and turns back into a SURFACE below it")
{
  Drawing older = Create(Version::R2010);
  Entities::Surface plain;
  plain.Base.Self = Handle{0x500};
  plain.UIsolines = 12;
  const Translation upgraded = ConvertObject(Object{plain}, older, Version::R2018);
  const auto &nurbs = Get<Entities::NurbSurface>(upgraded.Object);
  CHECK(nurbs.Base.Self == plain.Base.Self);
  CHECK(nurbs.UIsolines == 12);
  CHECK(nurbs.Geometry.Guid == Entities::Shapes::NoGuid);
  Drawing newer = Create(Version::R2013);
  const Translation downgraded = ConvertObject(upgraded.Object, newer, Version::R2000);
  CHECK(Get<Entities::Surface>(downgraded.Object) == plain);
}

TEST_CASE("releases/acis: an extruded or swept surface below R2007 carries ACAD's saved-by-version xdata")
{
  Drawing newer = Create(Version::R2010);
  Entities::ExtrudedSurface extruded;
  extruded.Base.Self = Handle{0x500};
  const Translation lowered = ConvertObject(Object{extruded}, newer, Version::R2004);
  CHECK(lowered.Issues.empty());
  CHECK(CountVersionData(lowered.Object) == 1);
  Drawing older = Create(Version::R2004);
  Object other = lowered.Object;
  BaseOf(other).Extended.insert(
      BaseOf(other).Extended.begin(),
      {Xdata{.Application = {},
             .AppId = {},
             .Tags = {Tag{.Code = 1070, .Value = std::int64_t{1}}, Tag{.Code = 1070, .Value = std::int64_t{2}}}},
       Xdata{.Application = {},
             .AppId = {},
             .Tags = {Tag{.Code = 1000, .Value = std::string{"Other"}}, Tag{.Code = 1070, .Value = std::int64_t{0}}}}});
  CHECK(BaseOf(ConvertObject(other, older, Version::R2018).Object).Extended.size() == 2);
  const Translation again = ConvertObject(lowered.Object, older, Version::R2000);
  CHECK(CountVersionData(again.Object) == 1);
  const Translation upgraded = ConvertObject(lowered.Object, older, Version::R2018);
  CHECK(CountVersionData(upgraded.Object) == 0);
  CHECK(BaseOf(upgraded.Object).Extended.empty());
  std::erase_if(newer.Objects,
                [](const Object &object) -> bool { return Holds<Tables::AppId>(object) && TypeOf(object) == "APPID"; });
  REQUIRE(View::Record<Tables::AppId>(newer, "ACAD") == nullptr);
  const Translation bare = ConvertObject(Object{Entities::SweptSurface{.Base = extruded.Base}}, newer, Version::R2000);
  CHECK(CountVersionData(bare.Object) == 0);
  REQUIRE(bare.Issues.size() == 1);
  CHECK(bare.Issues.front().What == "application ACAD missing for the version of 500");
}

TEST_CASE("releases/acis: an extrusion or sweep history node below R2007 carries its surface's version")
{
  Drawing newer = Create(Version::R2010);
  for (const char *name : {"ACSH_EXTRUSION_CLASS", "ACSH_SWEEP_CLASS", "ACSH_BOX_CLASS"})
  {
    CAPTURE(name);
    const Object node{Unknown{.Type = 500, .Class = Symbol{name}, .Base = {.Self = Handle{0x500}}}};
    const Translation lowered = ConvertObject(node, newer, Version::R2004);
    CHECK(lowered.Issues.empty());
    CHECK(CountVersionData(lowered.Object) == (std::string_view{name} == "ACSH_BOX_CLASS" ? 0U : 1U));
    CHECK(CountVersionData(ConvertObject(lowered.Object, newer, Version::R2007).Object) == 0);
  }
  std::erase_if(newer.Objects,
                [](const Object &object) -> bool { return Holds<Tables::AppId>(object) && TypeOf(object) == "APPID"; });
  const Translation bare =
      ConvertObject(Object{Unknown{.Type = 500, .Class = Symbol{"ACSH_SWEEP_CLASS"}, .Base = {.Self = Handle{0x500}}}},
                    newer, Version::R2004);
  REQUIRE(bare.Issues.size() == 1);
  CHECK(bare.Issues.front().What == "application ACAD missing for the version of 500");
}

TEST_CASE("releases/acis: only a body the store lacks or that cannot be written is stranded")
{
  Entities::Region region;
  region.Base.Self = Handle{0x500};
  region.Base.Stored = true;
  CHECK(FindStranding(Object{region}, Version::R2013, Version::R2010) ==
        "REGION 500 left out, its body not found in the data store");
  CHECK_FALSE(FindStranding(Object{region}, Version::R2013, Version::R2018));
  region.Base.Stored = false;
  CHECK_FALSE(FindStranding(Object{region}, Version::R2013, Version::R2010));
  region.Geometry.Body = Entities::Acis{.Version = 2, .Textual = false, .Data = BytesOf("x")};
  CHECK(FindStranding(Object{region}, Version::R2010, Version::R2013) ==
        "REGION 500 left out, its body cannot be written as R2013: head not read");
  CHECK_FALSE(FindStranding(Object{Entities::Line{}}, Version::R2013, Version::R2010));
  CHECK_FALSE(FindStranding(Object{Entities::Region{}}, Version::R2010, Version::R2013));
  // A body of the release's own ACIS is not read: it crosses as it is.
  const Drawing kinds = LoadFixture(FileOf("kinds", Version::R2007)).Drawing;
  const Object &solid = *Find(kinds, Handle{0x2CE});
  CHECK_FALSE(FindStranding(solid, Version::R2007, Version::R14));
  CHECK_FALSE(FindStranding(solid, Version::R2007, Version::R2018));
}

TEST_CASE("releases/acis: a body converted to another release is written in its ACIS version with its GUID kept")
{
  Drawing older = LoadFixture(FileOf("kinds", Version::R2007)).Drawing;
  // Upgraded to R2013 the body goes to the store; the body made by entmake kept no GUID below it, so none is said to
  // be there, as AutoCAD writes it.
  const Translation upgraded = ConvertObject(*Find(older, Handle{0x35F}), older, Version::R2013);
  const auto &body = Get<Entities::Body>(upgraded.Object);
  CHECK(body.Base.Stored);
  CHECK_FALSE(body.Geometry.Stored);
  CHECK(body.Geometry.Guid == Entities::Shapes::NoGuid);
  // Saved to an older release, the body is written in the object in its ACIS version, SAT below R2004.
  const Translation downgraded = ConvertObject(*Find(older, Handle{0x2CE}), older, Version::R2000);
  const auto &low = Get<Entities::Solid3d>(downgraded.Object);
  CHECK(low.Geometry.Body.Version == 1);
  CHECK(TextOf(low.Geometry.Body.Data).starts_with("400 "));
  // A body that cannot be read stays as it is, and a second stream that cannot be is dropped.
  Entities::Region broken;
  broken.Geometry.Body = Entities::Acis{.Version = 2, .Textual = false, .Data = BytesOf("x")};
  broken.Geometry.Second = Entities::Acis{.Version = 2, .Textual = false, .Data = BytesOf("y")};
  const Translation unread = ConvertObject(Object{broken}, older, Version::R2000);
  const auto &kept = Get<Entities::Region>(unread.Object);
  CHECK(kept.Geometry.Body == broken.Geometry.Body);
  CHECK(kept.Geometry.Second.Version == 0);
  // Saved below R2013, the GUID moves into ACAD_OBJECT_CHANGE_GUID's extended data and back from it on upgrading.
  Drawing newer = Load(ReadFixture("kinds-r2013.dwg")).value().Drawing;
  Tables::AppId appid;
  appid.Record.Name = "ACAD_OBJECT_CHANGE_GUID";
  REQUIRE(Edit::Add(newer, appid));
  const Object &stored = *Find(newer, Handle{0x2D7});
  const Translation lowered = ConvertObject(stored, newer, Version::R2010);
  const auto &region = Get<Entities::Region>(lowered.Object);
  REQUIRE(region.Base.Extended.size() == 1);
  CHECK(region.Base.Extended.front().Tags.size() == 12);
  CHECK(region.Geometry.Guid.empty());
  Drawing lowest = LoadFixture(FileOf("kinds", Version::R2010)).Drawing;
  const Translation raised = ConvertObject(*Find(lowest, Handle{0x2D7}), lowest, Version::R2018);
  const auto &again = Get<Entities::Region>(raised.Object);
  CHECK(again.Geometry.Guid == Get<Entities::Region>(stored).Geometry.Guid);
  CHECK(again.Geometry.Stored);
  CHECK(again.Base.Extended.empty());
  // An entity with a GUID kept below R2013 but no body says none is in the store.
  Object empty = *Find(lowest, Handle{0x2D7});
  GeometryOf(empty)->Body = {};
  const Translation bare = ConvertObject(empty, lowest, Version::R2013);
  CHECK_FALSE(Get<Entities::Region>(bare.Object).Geometry.Stored);
  CHECK_FALSE(Get<Entities::Region>(bare.Object).Base.Stored);
  const Translation plainRegion = ConvertObject(Object{Entities::Region{}}, lowest, Version::R2013);
  CHECK(Get<Entities::Region>(plainRegion.Object).Geometry.Guid == Entities::Shapes::NoGuid);
  // A body that cannot be written as the release leaves its entity out, and says why.
  Drawing unwritable = lowest;
  GeometryOf(*Find(unwritable, Handle{0x2D7}))->Body.Data = BytesOf("broken");
  const std::vector<Handle> leftOut = CollectLeftOut(unwritable, Version::R2004);
  CHECK(std::ranges::binary_search(leftOut, std::uint64_t{0x2D7}, {}, &Handle::Value));
  CHECK(Refusal(*Find(unwritable, Handle{0x2D7}), unwritable, Version::R2004) ==
        "REGION 2D7 left out, its body cannot be written as R2004: head not read");
  // A NURBS surface never says its body is in the store, and loses its second stream across R2013.
  const Translation renamed = ConvertObject(*Find(lowest, Handle{0x2EA}), lowest, Version::R2013);
  const auto &nurbs = Get<Entities::NurbSurface>(renamed.Object);
  CHECK_FALSE(nurbs.Geometry.Stored);
  CHECK(nurbs.Geometry.Guid == Entities::Shapes::NoGuid);
  CHECK(nurbs.Base.Stored);
  const Translation plain = ConvertObject(*Find(newer, Handle{0x2EA}), newer, Version::R2010);
  const auto &surface = Get<Entities::Surface>(plain.Object);
  CHECK(surface.Geometry.Second.Version == 0);
}

TEST_CASE("releases/acis: GeometryOf gives an ACIS entity's geometry and IsProcedural whether AutoCAD remakes")
{
  Object solid{Entities::Solid3d{}};
  CHECK(GeometryOf(solid) == &Get<Entities::Solid3d>(solid).Geometry);
  const Object &same = solid;
  CHECK(GeometryOf(same) == &Get<Entities::Solid3d>(solid).Geometry);
  Object line{Entities::Line{}};
  CHECK(GeometryOf(line) == nullptr);
  CHECK(GeometryOf(static_cast<const Object &>(line)) == nullptr);
  CHECK(IsProcedural(Object{Entities::SweptSurface{}}));
  CHECK(IsProcedural(Object{Entities::ExtrudedSurface{}}));
  CHECK_FALSE(IsProcedural(Object{Entities::Surface{}}));
  CHECK_FALSE(IsProcedural(line));
}

TEST_CASE("releases/acis: R14 and R2000 chains skip what a save drops and link to the nearest written entity")
{
  Drawing drawing = LoadFixture(FileOf("kinds", Version::R2000)).Drawing;
  // The helix and the mesh between the region and the planar surface are left out by hand, as a save leaves out what it
  // cannot write.
  const std::vector<Handle> leftOut{Handle{0x2D8}, Handle{0x2D9}};
  const Lookup found{drawing};
  const Omission leaving{.To = Version::R14, .LeftOut = leftOut, .Lookup = &found};
  Object region = *Find(drawing, Handle{0x2D7});
  Bridge(region, drawing, leaving);
  CHECK(CommonOf(region)->Next == Handle{0x2DA});
  CHECK(CommonOf(region)->Previous == Handle{0x2D2});
  Object planar = *Find(drawing, Handle{0x2DA});
  Bridge(planar, drawing, leaving);
  CHECK(CommonOf(planar)->Previous == Handle{0x2D7});
  // A release with lists, or a save that leaves nothing out, leaves the links as they are.
  Object kept = *Find(drawing, Handle{0x2D7});
  Bridge(kept, drawing, Omission{.To = Version::R2004, .LeftOut = leftOut, .Lookup = &found});
  CHECK(CommonOf(kept)->Next == Handle{0x2D8});
  Bridge(kept, drawing, Omission{.To = Version::R14, .LeftOut = {}, .Lookup = &found});
  CHECK(CommonOf(kept)->Next == Handle{0x2D8});
  // A block's ends past what is left out, the ends of a chain, a link to nothing and a loop of what is left out.
  std::vector<Handle> ends = leftOut;
  ends.insert(ends.end(), {Handle{0x2CE}, Handle{0x35F}, Handle{0x9999}, drawing.Header.Clayer});
  std::ranges::sort(ends, {}, &Handle::Value);
  Object model = *Find(drawing, drawing.Header.BlockRecordModelSpace);
  Bridge(model, drawing, Omission{.To = Version::R14, .LeftOut = ends, .Lookup = &found});
  CHECK(Get<Tables::BlockRecord>(model).First == Handle{0x2D2});
  CHECK(Get<Tables::BlockRecord>(model).Last != Handle{0x35F});
  Object sphere = *Find(drawing, Handle{0x2D2});
  Bridge(sphere, drawing, Omission{.To = Version::R14, .LeftOut = ends, .Lookup = &found});
  CHECK_FALSE(CommonOf(sphere)->Previous);
  CommonOf(sphere)->Next = Handle{0x9999};
  Bridge(sphere, drawing, Omission{.To = Version::R14, .LeftOut = ends, .Lookup = &found});
  CHECK_FALSE(CommonOf(sphere)->Next);
  CommonOf(sphere)->Next = drawing.Header.Clayer; // A link to what is no entity.
  Bridge(sphere, drawing, Omission{.To = Version::R14, .LeftOut = ends, .Lookup = &found});
  CHECK_FALSE(CommonOf(sphere)->Next);
  CommonOf(*Find(drawing, Handle{0x2D8}))->Next = Handle{0x2D9};
  CommonOf(*Find(drawing, Handle{0x2D9}))->Next = Handle{0x2D8};
  Object looped = *Find(drawing, Handle{0x2D7});
  Bridge(looped, drawing, leaving);
  CHECK_FALSE(CommonOf(looped)->Next);
  Object layer = *Find(drawing, drawing.Header.Clayer);
  Bridge(layer, drawing, leaving);
  CHECK(layer == *Find(drawing, drawing.Header.Clayer));
}

TEST_CASE("releases/acis: a solid's history must name a history object or it is cleared")
{
  Drawing drawing = LoadFixture(FileOf("kinds", Version::R2007)).Drawing;
  for (const Audit::Finding &finding : Audit::Check(drawing).Findings)
  {
    CHECK(finding.What.find("history") == std::string::npos);
  }
  auto *solid = GetIf<Entities::Solid3d>(Find(drawing, Handle{0x2CE}));
  REQUIRE(solid != nullptr);
  REQUIRE(solid->History);
  auto *sphere = GetIf<Entities::Solid3d>(Find(drawing, Handle{0x2D2}));
  REQUIRE(sphere != nullptr);
  sphere->History = {}; // None, which names nothing to check.
  auto *third = GetIf<Entities::Solid3d>(Find(drawing, Handle{0x2FB}));
  REQUIRE(third != nullptr);
  REQUIRE(Holds<Unknown>(*Find(drawing, Handle{0x338})));
  third->History = Handle{0x338}; // An object of another class.
  solid->History = Handle{0x2CE};
  const Audit::Report report = Audit::Repair(drawing);
  CHECK(std::ranges::any_of(report.Findings, [](const Audit::Finding &finding) -> bool
                            { return finding.What == "history 2CE of 3DSOLID 2CE missing"; }));
  CHECK_FALSE(GetIf<Entities::Solid3d>(Find(drawing, Handle{0x2CE}))->History);
  CHECK_FALSE(GetIf<Entities::Solid3d>(Find(drawing, Handle{0x2FB}))->History);
}
} // namespace DwgIO::Releases
