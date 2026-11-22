/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/conversion.cpp
 *  @brief     Entities, linetypes, dimension styles and names across R14 and R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "octets.hpp"
#include "releases/header.hpp"
#include "releases/object.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  The entity of a drawing under a handle, or null.
 */
auto ReadTwin(const Drawing &drawing, Handle self) -> const Object *
{
  const Object *object = Find(drawing, self);
  return object != nullptr && CommonOf(*object) != nullptr && !Holds<Unknown>(*object) ? object : nullptr;
}

/**
 *  Whether an entity belongs to a dimension's anonymous block, which AutoCAD redraws at each save.
 */
auto IsInDimensionBlock(const Drawing &drawing, const Object &object) -> bool
{
  const Object *owner = Find(drawing, BaseOf(object).Owner);
  const auto *record = owner == nullptr ? nullptr : GetIf<Tables::BlockRecord>(owner);
  return record != nullptr && record->Record.Name.starts_with("*D");
}

/**
 *  The groups of every extended data record: AutoCAD renumbers the applications at each save, so the handles of two
 *  saves differ while the data does not.
 */
auto Groups(const Object &object) -> std::vector<std::vector<DwgIO::Tag>>
{
  std::vector<std::vector<DwgIO::Tag>> groups;
  for (const Xdata &xdata : BaseOf(object).Extended)
  {
    groups.push_back(xdata.Tags);
  }
  return groups;
}

/**
 *  Whether an extended data record is R14's lineweight.
 */
auto Width(const Xdata &xdata) -> bool
{
  return !xdata.Tags.empty() && xdata.Tags.front().Code == 1000 &&
         std::get<std::string>(xdata.Tags.front().Value) == "RTLwPs";
}
} // namespace

TEST_CASE("releases/conversion: an R14 entity converted to R2000 matches AutoCAD's R2000 save")
{
  for (const std::string name : {"entities", "curves"})
  {
    CAPTURE(name);
    const Drawing older = LoadFixture(name + "-r14.dwg").Drawing;
    const Drawing newer = LoadFixture(name + "-r2000.dwg").Drawing;
    std::size_t compared = 0;
    for (const Object &object : older.Objects)
    {
      const Object *twin = CommonOf(object) == nullptr ? nullptr : ReadTwin(newer, BaseOf(object).Self);
      if (twin == nullptr || IsInDimensionBlock(older, object) || Holds<Entities::Block>(object))
      {
        continue;
      }
      CAPTURE(HexOf(BaseOf(object).Self));
      const Releases::Translation translated = Releases::ConvertObject(object, older, Version::R2000);
      CHECK(translated.Issues.empty());
      const Common &ours = *CommonOf(translated.Object);
      const Common &theirs = *CommonOf(*twin);
      CHECK(ours.Lineweight == theirs.Lineweight);
      CHECK(ours.LinetypeOrigin == theirs.LinetypeOrigin);
      CHECK(ours.Linetype == theirs.Linetype);
      CHECK(std::ranges::none_of(BaseOf(translated.Object).Extended, Width));
      if (BaseOf(*twin).Extended.size() == BaseOf(translated.Object).Extended.size())
      {
        CHECK(Groups(translated.Object) == Groups(*twin));
      }
      ++compared;
    }
    CHECK(compared > 30);
  }
}

TEST_CASE("releases/conversion: an R2000 entity converted to R14 matches AutoCAD's R14 save")
{
  for (const std::string name : {"entities", "curves"})
  {
    CAPTURE(name);
    const Drawing older = LoadFixture(name + "-r14.dwg").Drawing;
    const Drawing newer = LoadFixture(name + "-r2000.dwg").Drawing;
    std::size_t compared = 0;
    std::size_t widths = 0;
    for (const Object &object : newer.Objects)
    {
      const Object *twin = CommonOf(object) == nullptr ? nullptr : ReadTwin(older, BaseOf(object).Self);
      if (twin == nullptr || IsInDimensionBlock(newer, object) || Holds<Entities::Block>(object))
      {
        continue;
      }
      CAPTURE(HexOf(BaseOf(object).Self));
      const Releases::Translation translated = Releases::ConvertObject(object, newer, Version::R14);
      const Common &ours = *CommonOf(translated.Object);
      const Common &theirs = *CommonOf(*twin);
      CHECK(ours.Lineweight == theirs.Lineweight);
      CHECK(ours.LinetypeOrigin == theirs.LinetypeOrigin);
      CHECK(ours.Linetype == theirs.Linetype);
      CHECK(ours.PlotstyleOrigin == Origin::ByLayer);
      CHECK(ours.Plotstyle == Handle{});
      if (BaseOf(*twin).Extended.size() == BaseOf(translated.Object).Extended.size())
      {
        CHECK(Groups(translated.Object) == Groups(*twin));
      }
      widths += std::ranges::any_of(BaseOf(translated.Object).Extended, Width) ? 1U : 0U;
      ++compared;
    }
    CHECK(compared > 30);
    CHECK((widths > 0) == (name == "entities")); // The curves fixture draws every entity ByLayer.
  }
}

TEST_CASE("releases/conversion: a named linetype keeps its record and what R14 lacks is noted")
{
  Drawing drawing = Create();
  Tables::LType dashed;
  dashed.Record.Name = "Dashed";
  const Handle pattern = *Edit::Add(drawing, dashed);
  Entities::Line line;
  line.Common.LinetypeOrigin = Origin::Record;
  line.Common.Linetype = pattern;
  line.Common.PlotstyleOrigin = Origin::Record;
  line.Common.Plotstyle = Handle{.Value = 0xF};
  line.Common.Lineweight = Lineweight::Mm018;
  const Handle drawn = *Edit::Add(drawing, line);
  const Releases::Translation older = Releases::ConvertObject(*Find(drawing, drawn), drawing, Version::R14);
  CHECK(CommonOf(older.Object)->LinetypeOrigin == Origin::Record);
  CHECK(CommonOf(older.Object)->Linetype == pattern);
  CHECK(CommonOf(older.Object)->Lineweight == std::nullopt);
  REQUIRE(BaseOf(older.Object).Extended.size() == 1);
  CHECK(BaseOf(older.Object).Extended[0].AppId == View::Record<Tables::AppId>(drawing, "ACAD")->Record.Base.Self);
  CHECK(BaseOf(older.Object).Extended[0].Tags ==
        std::vector<DwgIO::Tag>{DwgIO::Tag{.Code = 1000, .Value = std::string{"RTLwPs"}},
                                DwgIO::Tag{.Code = 1070, .Value = std::int64_t{133}}});
  REQUIRE(older.Issues.size() == 1);
  CHECK(older.Issues[0].What == "plot style of LINE 2C not kept by R14");
  CHECK(older.Issues[0].Subject == drawn);
  Drawing back = drawing;
  back.Release = Version::R14;
  const Releases::Translation newer = Releases::ConvertObject(older.Object, back, Version::R2000);
  CHECK(newer.Issues.empty());
  CHECK(CommonOf(newer.Object)->Linetype == pattern);
  CHECK(CommonOf(newer.Object)->LinetypeOrigin == Origin::Record);
  // A handle equal to ByLayer's, ByBlock's or Continuous's record is what R2000 says in the flags.
  Entities::Line solid;
  solid.Common.LinetypeOrigin = Origin::Record;
  solid.Common.Linetype = drawing.Header.LTypeContinuous;
  const Handle drawnSolid = *Edit::Add(drawing, solid);
  CHECK(CommonOf(Releases::ConvertObject(*Find(drawing, drawnSolid), back, Version::R2000).Object)->LinetypeOrigin ==
        Origin::Continuous);
  Entities::Line bylayer;
  bylayer.Common.LinetypeOrigin = Origin::Record;
  bylayer.Common.Linetype = drawing.Header.LTypeByLayer;
  const Handle drawnBylayer = *Edit::Add(drawing, bylayer);
  const Releases::Translation upgraded = Releases::ConvertObject(*Find(drawing, drawnBylayer), back, Version::R2000);
  CHECK(CommonOf(upgraded.Object)->LinetypeOrigin == Origin::ByLayer);
  CHECK(CommonOf(upgraded.Object)->Linetype == Handle{});
  CHECK(CommonOf(newer.Object)->Lineweight == Lineweight::Mm018);
  CHECK(BaseOf(newer.Object).Extended.empty());
  CHECK(Releases::ConvertObject(*Find(drawing, drawn), drawing, Version::R2000).Object == *Find(drawing, drawn));

  Tables::Layer walls;
  walls.Record.Name = "Walls";
  walls.Lineweight = Lineweight::Mm018;
  walls.Linetype = drawing.Header.LTypeContinuous;
  const Handle layered = *Edit::Add(drawing, walls);
  const Releases::Translation layer = Releases::ConvertObject(*Find(drawing, layered), drawing, Version::R14);
  REQUIRE(layer.Issues.size() == 1);
  CHECK(layer.Issues[0].What == "lineweight of layer Walls not kept by R14");
  CHECK(Get<Tables::Layer>(layer.Object).Style == Handle{});

  // Without the ACAD application there is nowhere to keep the lineweight: it is lost and said.
  const Handle acad = View::Record<Tables::AppId>(drawing, "ACAD")->Record.Base.Self;
  std::erase_if(drawing.Objects, [acad](const Object &object) -> bool { return BaseOf(object).Self == acad; });
  const Releases::Translation unkept = Releases::ConvertObject(*Find(drawing, drawn), drawing, Version::R14);
  CHECK(BaseOf(unkept.Object).Extended.empty());
  CHECK(CommonOf(unkept.Object)->Lineweight == std::nullopt);
  REQUIRE(unkept.Issues.size() == 2);
  CHECK(unkept.Issues[1].What == "application ACAD missing for the lineweight of LINE 2C");
}

TEST_CASE("releases/conversion: references at the edges of a conversion are kept or named or left as they are")
{
  // Extended data of ACAD's that is not the lineweight record stays, and the lineweight is ByLayer.
  Drawing older = Create(Version::R14);
  const Handle acad = View::Record<Tables::AppId>(older, "ACAD")->Record.Base.Self;
  Entities::Line noted;
  noted.Base.Extended = {Xdata{.AppId = acad,
                               .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::int64_t{1}},
                                        DwgIO::Tag{.Code = 1070, .Value = std::int64_t{133}}}},
                         Xdata{.AppId = acad,
                               .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string{"RTLwPs"}},
                                        DwgIO::Tag{.Code = 1070, .Value = std::string{"5"}}}}};
  const Handle drawn = *Edit::Add(older, noted);
  const Releases::Translation upgraded = Releases::ConvertObject(*Find(older, drawn), older, Version::R2000);
  CHECK(upgraded.Issues.empty());
  CHECK(CommonOf(upgraded.Object)->Lineweight == Lineweight::ByLayer);
  CHECK(BaseOf(upgraded.Object).Extended.size() == 2);

  // A lineweight below zero has no record to keep in R14.
  Drawing newer = Create();
  Entities::Line unweighted;
  unweighted.Common.Lineweight = std::nullopt;
  Handle self = Edit::MakeHandle(newer);
  unweighted.Base.Self = self;
  unweighted.Base.Owner = newer.Header.BlockRecordModelSpace;
  unweighted.Common.Layer = newer.Header.Clayer;
  newer.Objects.emplace_back(unweighted);
  const Releases::Translation lowered = Releases::ConvertObject(*Find(newer, self), newer, Version::R14);
  CHECK(lowered.Issues.empty());
  CHECK(CommonOf(lowered.Object)->Lineweight == std::nullopt);
  CHECK(BaseOf(lowered.Object).Extended.empty());

  // An arrow block of the drawing's own goes by its name as it is, either way.
  const Handle tick = *Edit::AddBlock(newer, "Tick");
  Tables::DimStyle ticked;
  ticked.Record.Name = "Ticked";
  ticked.Dimblk = tick;
  const Handle styled = *Edit::Add(newer, ticked);
  const Releases::Translation named = Releases::ConvertObject(*Find(newer, styled), newer, Version::R14);
  CHECK(named.Issues.empty());
  CHECK(Get<Tables::DimStyle>(named.Object).Arrows.Dimblk == "Tick");
  CHECK(Get<Tables::DimStyle>(named.Object).Dimblk == Handle{});
  Drawing asOlder = newer;
  asOlder.Release = Version::R14;
  const Releases::Translation found = Releases::ConvertObject(named.Object, asOlder, Version::R2000);
  CHECK(found.Issues.empty());
  CHECK(Get<Tables::DimStyle>(found.Object).Dimblk == tick);

  // A root without one of R2000's dictionaries gives the upgraded header a null handle for it.
  auto &root = Get<Objects::Dictionary>(*Find(older, older.Header.DictionaryNamedObjects));
  std::erase_if(root.Entries, [](const Objects::Entry &entry) -> bool { return entry.Name == "ACAD_PLOTSETTINGS"; });
  const Releases::HeaderTranslation header = Releases::ConvertHeader(older.Header, older, Version::R2000);
  CHECK(header.Header.DictionaryPlotSettings == Handle{});
  CHECK(header.Header.DictionaryLayouts == View::Dictionary(older, "ACAD_LAYOUT")->Base.Self);
}

TEST_CASE("releases/conversion: the units and fit that R2000 split in two convert whole between releases")
{
  // DIMUNIT 6 is architectural, not stacked; DIMFIT 4 is the best fit with the text on a leader.
  Drawing older = LoadFixture("tables-r14.dwg").Drawing;
  older.Header.Dimunit = 6;
  older.Header.Dimfit = 4;
  const Releases::HeaderTranslation up = Releases::ConvertHeader(older.Header, older, Version::R2000);
  CHECK(up.Header.Dimlunit == 4);
  CHECK(up.Header.Dimfrac == 2);
  CHECK(up.Header.Dimatfit == 3);
  CHECK(up.Header.Dimtmove == 1);
  Drawing newer = LoadFixture("tables-r2000.dwg").Drawing;
  newer.Header.Dimlunit = 5;
  newer.Header.Dimfrac = 2;
  newer.Header.Dimatfit = 1;
  newer.Header.Dimtmove = 2;
  const Releases::HeaderTranslation down = Releases::ConvertHeader(newer.Header, newer, Version::R14);
  CHECK(down.Header.Dimunit == 7);
  CHECK(down.Header.Dimfit == 5);
  // Every R14 value comes back from its R2000 pair.
  Drawing asNewer = older;
  asNewer.Release = Version::R2000;
  for (int unit = 1; unit <= 8; ++unit)
  {
    for (int fit = 0; fit <= 5; ++fit)
    {
      CAPTURE(unit);
      CAPTURE(fit);
      Header header = older.Header;
      header.Dimunit = unit;
      header.Dimfit = fit;
      const Releases::HeaderTranslation upgraded = Releases::ConvertHeader(header, older, Version::R2000);
      const Releases::HeaderTranslation back = Releases::ConvertHeader(upgraded.Header, asNewer, Version::R14);
      CHECK(back.Header.Dimunit == unit);
      CHECK(back.Header.Dimfit == fit);
    }
  }
  // A dimension style the same way.
  Tables::DimStyle windows;
  windows.Dimunit = 8;
  windows.Dimfit = 2;
  const Object styled = Releases::ConvertObject(Object{windows}, older, Version::R2000).Object;
  CHECK(Get<Tables::DimStyle>(styled).Dimlunit == 6);
  CHECK(Get<Tables::DimStyle>(styled).Dimfrac == 0);
  CHECK(Get<Tables::DimStyle>(styled).Dimatfit == 2);
  CHECK(Get<Tables::DimStyle>(styled).Dimtmove == 0);
  Tables::DimStyle stacked;
  stacked.Dimlunit = 4;
  stacked.Dimfrac = 1;
  stacked.Dimatfit = 0;
  stacked.Dimtmove = 0;
  const Object lowered = Releases::ConvertObject(Object{stacked}, newer, Version::R14).Object;
  CHECK(Get<Tables::DimStyle>(lowered).Dimunit == 4);
  CHECK(Get<Tables::DimStyle>(lowered).Dimfit == 0);
  // A value neither release names is decimal and the best fit.
  Tables::DimStyle odd;
  odd.Dimunit = 0;
  odd.Dimfit = 9;
  const Object oddRisen = Releases::ConvertObject(Object{odd}, older, Version::R2000).Object;
  CHECK(Get<Tables::DimStyle>(oddRisen).Dimlunit == 2);
  CHECK(Get<Tables::DimStyle>(oddRisen).Dimfrac == 0);
  CHECK(Get<Tables::DimStyle>(oddRisen).Dimatfit == 3);
  CHECK(Get<Tables::DimStyle>(oddRisen).Dimtmove == 0);
  Tables::DimStyle oddModern;
  oddModern.Dimlunit = 9;
  oddModern.Dimatfit = 7;
  oddModern.Dimtmove = 0;
  const Object oddLowered = Releases::ConvertObject(Object{oddModern}, newer, Version::R14).Object;
  CHECK(Get<Tables::DimStyle>(oddLowered).Dimunit == 2);
  CHECK(Get<Tables::DimStyle>(oddLowered).Dimfit == 3);
  oddModern.Dimatfit = -1;
  CHECK(Get<Tables::DimStyle>(Releases::ConvertObject(Object{oddModern}, newer, Version::R14).Object).Dimfit == 3);
}

TEST_CASE("releases/conversion: an R14 entity's lineweight record comes first when saved as R2000")
{
  Drawing older = Create(Version::R14);
  const Handle acad = View::Record<Tables::AppId>(older, "ACAD")->Record.Base.Self;
  const Xdata other{.AppId = acad, .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string{"kept"}}}};
  Entities::Line line;
  line.Base.Extended = {Xdata{.AppId = acad,
                              .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string{"RTLwPs"}},
                                       DwgIO::Tag{.Code = 1070, .Value = std::int64_t{133}}}},
                        other};
  const Handle drawn = *Edit::Add(older, line);
  const std::expected<Saved, Failure> saved = Save(older, SaveOptions{.Release = Version::R2000});
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  const Object *back = Find(loaded->Drawing, drawn);
  REQUIRE(back);
  CHECK(CommonOf(*back)->Lineweight == Lineweight::Mm018);
  REQUIRE(BaseOf(*back).Extended.size() == 1);
  CHECK(BaseOf(*back).Extended[0].Tags == other.Tags);
}

TEST_CASE("releases/conversion: the entities fixture saved to the other release loads back converted")
{
  const Drawing newer = LoadFixture("entities-r2000.dwg").Drawing;
  const std::expected<Saved, Failure> asOlder = Save(newer, SaveOptions{.Release = Version::R14});
  REQUIRE(asOlder);
  const std::expected<Loaded, Failure> older = Load(asOlder->Bytes);
  REQUIRE(older);
  const Common &line = *CommonOf(*Find(older->Drawing, Handle{.Value = 0x2F8}));
  CHECK(line.Lineweight == std::nullopt);
  CHECK(std::ranges::any_of(BaseOf(*Find(older->Drawing, Handle{.Value = 0x2F8})).Extended, Width));
  QueueAudit(asOlder->Bytes, "dwgio-entities-as-r14.dwg");
  const Drawing theirs = LoadFixture("entities-r14.dwg").Drawing;
  const std::expected<Saved, Failure> asNewer = Save(theirs, SaveOptions{.Release = Version::R2000});
  REQUIRE(asNewer);
  const std::expected<Loaded, Failure> back = Load(asNewer->Bytes);
  REQUIRE(back);
  CHECK(CommonOf(*Find(back->Drawing, Handle{.Value = 0x2F8}))->Lineweight == Lineweight::Mm050);
  CHECK(std::ranges::none_of(BaseOf(*Find(back->Drawing, Handle{.Value = 0x2F8})).Extended, Width));
  QueueAudit(asNewer->Bytes, "dwgio-entities-as-r2000.dwg");
}

TEST_CASE("releases/conversion: the header and the dimension styles convert like AutoCAD's saves")
{
  const Drawing older = LoadFixture("tables-r14.dwg").Drawing;
  const Drawing newer = LoadFixture("tables-r2000.dwg").Drawing;
  const Releases::HeaderTranslation up = Releases::ConvertHeader(older.Header, older, Version::R2000);
  CHECK(up.Issues.empty());
  CHECK(up.Header.Dimblk == newer.Header.Dimblk);
  CHECK(up.Header.Dimblk == Handle{.Value = 0x2E2});
  CHECK(up.Header.Dimblk1 == Handle{});
  CHECK(up.Header.Dimldrblk == Handle{});
  CHECK(up.Header.Dimlwd == -2);
  CHECK(up.Header.Tstacksize == 70);
  CHECK(!up.Header.Fingerprintguid.empty());
  CHECK(up.Header.DictionaryLayouts == View::Dictionary(older, "ACAD_LAYOUT")->Base.Self);
  CHECK(up.Header.DictionaryPlotStyles == View::Dictionary(older, "ACAD_PLOTSTYLENAME")->Base.Self);
  CHECK(up.Header.Clayer == older.Header.Clayer);
  const Releases::HeaderTranslation down = Releases::ConvertHeader(newer.Header, newer, Version::R14);
  CHECK(down.Issues.empty());
  CHECK(View::Matches(down.Header.DimblkName, older.Header.DimblkName));
  CHECK(down.Header.DimblkName == "Dot");
  CHECK(down.Header.Dimblk1Name.empty());
  CHECK(down.Header.Attreq);
  CHECK(down.Header.Unknown11 == 256);
  CHECK(down.Header.Dimfit == 3);
  CHECK(Releases::ConvertHeader(newer.Header, newer, Version::R2000).Header == newer.Header);

  const Tables::DimStyle *metric = View::Record<Tables::DimStyle>(older, "Metric");
  REQUIRE(metric);
  const Object upgraded = Releases::ConvertObject(*Find(older, metric->Record.Base.Self), older, Version::R2000).Object;
  CHECK(Get<Tables::DimStyle>(upgraded).Dimblk == Handle{.Value = 0x2E2});
  CHECK(Get<Tables::DimStyle>(upgraded).Arrows.Dimblk == "DOT");
  const Tables::DimStyle *modern = View::Record<Tables::DimStyle>(newer, "Metric");
  REQUIRE(modern);
  const Object lowered = Releases::ConvertObject(*Find(newer, modern->Record.Base.Self), newer, Version::R14).Object;
  CHECK(Get<Tables::DimStyle>(lowered).Arrows.Dimblk == "Dot");

  Drawing missing = older;
  missing.Header.DimblkName = "ARROW";
  const Releases::HeaderTranslation arrowless = Releases::ConvertHeader(missing.Header, missing, Version::R2000);
  CHECK(arrowless.Header.Dimblk == Handle{});
  REQUIRE(arrowless.Issues.size() == 1);
  CHECK(arrowless.Issues[0].What == "dimension block ARROW missing");
}

TEST_CASE("releases/conversion: the header variables R2010 and R2013 add take AutoCAD's values when upgrading")
{
  const Drawing older = LoadFixture("attachments-r2007.dwg").Drawing;
  const Header theirs = LoadFixture("attachments-r2007-as-r2010.dwg").Drawing.Header;
  const Header up = Releases::ConvertHeader(older.Header, older, Version::R2010).Header;
  CHECK(up.Dimmzf == theirs.Dimmzf);
  CHECK(up.Dimaltmzf == theirs.Dimaltmzf);
  CHECK(up.Dimmzf == 100);
  const Drawing newer = LoadFixture("empty-r2013.dwg").Drawing;
  Drawing marked = newer;
  marked.Header.Requiredversions = 1;
  marked.Header.Dimmzf = 3;
  const Header down = Releases::ConvertHeader(marked.Header, marked, Version::R2010).Header;
  CHECK(down.Requiredversions == 0);
  CHECK(down.Dimmzf == 3);
  CHECK(Releases::ConvertHeader(down, marked, Version::R2007).Header.Dimmzf == 0);
}

TEST_CASE("releases/conversion: the tables fixture saved to the other release keeps its dimension blocks")
{
  const Drawing older = LoadFixture("tables-r14.dwg").Drawing;
  const std::expected<Saved, Failure> asNewer = Save(older, SaveOptions{.Release = Version::R2000});
  REQUIRE(asNewer);
  const std::expected<Loaded, Failure> back = Load(asNewer->Bytes);
  REQUIRE(back);
  CHECK(back->Drawing.Header.Dimblk == Handle{.Value = 0x2E2});
  CHECK(back->Drawing.Header.DictionaryLayouts == View::Dictionary(older, "ACAD_LAYOUT")->Base.Self);
  QueueAudit(asNewer->Bytes, "dwgio-tables-as-r2000.dwg");
  const Drawing newer = LoadFixture("tables-r2000.dwg").Drawing;
  const std::expected<Saved, Failure> asOlder = Save(newer, SaveOptions{.Release = Version::R14});
  REQUIRE(asOlder);
  const std::expected<Loaded, Failure> down = Load(asOlder->Bytes);
  REQUIRE(down);
  CHECK(View::Matches(down->Drawing.Header.DimblkName, "DOT"));
  CHECK(down->Drawing.Header.Attreq);
  QueueAudit(asOlder->Bytes, "dwgio-tables-as-r14.dwg");
}

namespace
{
/**
 *  The drawing's first multiline style.
 */
auto Style(Drawing &drawing) -> Objects::MLineStyle &
{
  const auto found = std::ranges::find_if(drawing.Objects, [](const Object &object) -> bool
                                          { return Holds<Objects::MLineStyle>(object); });
  REQUIRE(found != drawing.Objects.end());
  return Get<Objects::MLineStyle>(*found);
}

auto IssueTexts(const std::vector<Issue> &issues) -> std::vector<std::string>
{
  std::vector<std::string> whats;
  for (const Issue &issue : issues)
  {
    whats.push_back(issue.What);
  }
  return whats;
}
} // namespace

TEST_CASE("releases/conversion: a multiline style's linetypes go by index before R2018 and by record from it")
{
  // AutoCAD's own R2018 save of the style names ByLayer where R2013 gives 32767.
  Drawing older = LoadFixture("tables-r2013.dwg").Drawing;
  Drawing newer = LoadFixture("tables-r2018.dwg").Drawing;
  const Objects::MLineStyle standard = Style(older);
  CHECK(Get<Objects::MLineStyle>(Releases::ConvertObject(Object{standard}, older, Version::R2018).Object) ==
        Style(newer));
  CHECK(Get<Objects::MLineStyle>(Releases::ConvertObject(Object{Style(newer)}, newer, Version::R2013).Object) ==
        standard);
  // An index counts among the linetype table's entries, as AutoCAD's R2018 save of an R2013 file reads it.
  const std::vector<Handle> table = View::Control(older, Tables::Table::LType)->Entries;
  REQUIRE(table.size() > 2);
  Objects::MLineStyle indexed = standard;
  indexed.Elements.resize(5);
  const std::vector<int> indices{2, 32766, 32767, -1, 99};
  for (std::size_t at = 0; at < indices.size(); ++at)
  {
    indexed.Elements[at].Linetype = indices[at];
  }
  const Releases::Translation up = Releases::ConvertObject(Object{indexed}, older, Version::R2018);
  std::vector<Handle> records;
  for (const Objects::MLineStyles::Element &element : Get<Objects::MLineStyle>(up.Object).Elements)
  {
    records.push_back(element.Record);
    CHECK(element.Linetype == 32767);
  }
  const Handle bylayer = older.Header.LTypeByLayer;
  CHECK(records == std::vector<Handle>{table[2], older.Header.LTypeByBlock, bylayer, bylayer, bylayer});
  const std::string style = std::format("MLINESTYLE {}", HexOf(standard.Base.Self));
  CHECK(IssueTexts(up.Issues) ==
        std::vector<std::string>{std::format("linetype -1 of {} not in its table, ByLayer", style),
                                 std::format("linetype 99 of {} not in its table, ByLayer", style)});
  // Saved as R14, a record goes by its place in the table; one the table lacks is ByLayer and noted.
  Objects::MLineStyle named = Style(newer);
  named.Elements.resize(5);
  const std::vector<Handle> pointed{table[2], newer.Header.LTypeByBlock, newer.Header.LTypeByLayer, Handle{},
                                    Handle{.Value = 0x999}};
  for (std::size_t at = 0; at < pointed.size(); ++at)
  {
    named.Elements[at].Record = pointed[at];
  }
  const Releases::Translation down = Releases::ConvertObject(Object{named}, newer, Version::R2013);
  std::vector<int> back;
  for (const Objects::MLineStyles::Element &element : Get<Objects::MLineStyle>(down.Object).Elements)
  {
    back.push_back(element.Linetype);
    CHECK(element.Record == Handle{});
  }
  CHECK(back == std::vector<int>{2, 32766, 32767, 32767, 32767});
  CHECK(IssueTexts(down.Issues) ==
        std::vector<std::string>{std::format("linetype 999 of {} not in its table, ByLayer", style)});
  // A drawing without a linetype table has no index to give.
  std::erase_if(older.Objects, [](const Object &object) -> bool
                { return Holds<Tables::Control>(object) && TypeOf(object) == "LTYPE_CONTROL"; });
  Objects::MLineStyle first = standard;
  first.Elements.front().Linetype = 0;
  CHECK(Releases::ConvertObject(Object{first}, older, Version::R2018).Issues.size() == 1);
}

TEST_CASE("releases/conversion: a multiline style saved across R2018 keeps its linetypes")
{
  for (const auto &[from, to] :
       {std::pair{"tables-r2013.dwg", Version::R2018}, std::pair{"tables-r2018.dwg", Version::R2013}})
  {
    CAPTURE(from);
    Drawing drawing = LoadFixture(from).Drawing;
    const std::vector<Handle> table = View::Control(drawing, Tables::Table::LType)->Entries;
    REQUIRE(table.size() > 2);
    Objects::MLineStyle &style = Style(drawing);
    style.Elements.front().Linetype = 2;
    style.Elements.front().Record = table[2];
    style.Elements.back().Linetype = 32766;
    style.Elements.back().Record = drawing.Header.LTypeByBlock;
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = to});
    REQUIRE(saved);
    Drawing back = Load(saved->Bytes).value().Drawing;
    const Objects::MLineStyle &kept = Style(back);
    if (to == Version::R2018)
    {
      CHECK(kept.Elements.front().Record == table[2]);
      CHECK(kept.Elements.back().Record == drawing.Header.LTypeByBlock);
    }
    else
    {
      CHECK(kept.Elements.front().Linetype == 2);
      CHECK(kept.Elements.back().Linetype == 32766);
    }
    QueueAudit(saved->Bytes, std::format("dwgio-mlinestyle-{}-as-{}.dwg", from, NameOf(to)));
  }
}

TEST_CASE("releases/conversion: text a code page lacks is written below R2007 as Unicode escapes")
{
  // AutoCAD's Japanese samples name their layers in Japanese under ANSI_1252; written as question marks, AutoCAD's
  // audit renames them.
  Drawing drawing = Load(ReadFixture("tables-r2018.dwg")).value().Drawing;
  Tables::Layer title;
  title.Record.Name = "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\x88\xe3\x83\xab caf\xc3\xa9";
  const std::expected<Handle, Failure> added = Edit::Add(drawing, title);
  REQUIRE(added);
  for (const Version release : {Version::R14, Version::R2000, Version::R2004})
  {
    CAPTURE(NameOf(release));
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = release});
    REQUIRE(saved);
    const Drawing down = Load(saved->Bytes).value().Drawing;
    const auto *layer = GetIf<Tables::Layer>(Find(down, *added));
    REQUIRE(layer != nullptr);
    // Written as AutoCAD writes it, and read back as AutoCAD reads it.
    if (release < Version::R2004)
    {
      CHECK(TextOf(saved->Bytes).contains("\\U+30BF\\U+30A4\\U+30C8\\U+30EB caf\xe9"));
    }
    CHECK(layer->Record.Name == title.Record.Name);
    QueueAudit(saved->Bytes, std::format("dwgio-tables-wide-as-{}.dwg", NameOf(release)));
    // Upgraded again, the characters themselves, as AutoCAD names the layers when it saves such an R2004 file as R2018.
    const std::expected<Saved, Failure> upgraded = Save(down, SaveOptions{.Release = Version::R2018});
    REQUIRE(upgraded);
    const Drawing up = Load(upgraded->Bytes).value().Drawing;
    const auto *back = GetIf<Tables::Layer>(Find(up, *added));
    REQUIRE(back != nullptr);
    CHECK(back->Record.Name == title.Record.Name);
  }
}

TEST_CASE("releases/conversion: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
