/***********************************************************************************************************************
 *  @file      tests/dwgio/save/cross.cpp
 *  @brief     Drawings saved to another release: what they keep, what they lose and what is noted.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/coded/layout.hpp"
#include "file/locator/layout.hpp"
#include "file/paged/layout.hpp"
#include "objects/map.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/class.hpp"
#include "dwgio/common.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/hatch.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/lwpolyline.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/scale.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/save.hpp"
#include "dwgio/section.hpp"
#include "dwgio/symbol.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  Renames the drawing's first untyped object to a class AutoCAD's files never hold, which no save to another release
 *  carries; returns its handle.
 */
auto Unseen(Drawing &drawing) -> Handle
{
  const auto found =
      std::ranges::find_if(drawing.Objects, [](const Object &object) -> bool { return Holds<Unknown>(object); });
  REQUIRE(found != drawing.Objects.end());
  Get<Unknown>(*found).Class = Symbol{"UNSEEN"};
  return BaseOf(*found).Self;
}
} // namespace

TEST_CASE("save/cross: an R2000 drawing saved as R14 has five locator records and no auxiliary block")
{
  const Drawing newer = LoadDrawing("foundation-r2000.dwg");
  const std::expected<Saved, Failure> saved = Save(newer, SaveOptions{.Release = Version::R14});
  REQUIRE(saved);
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Drawing.Release == Version::R14);
  CHECK(loaded->Drawing.Maintenance == 10);
  CHECK(!std::ranges::contains(loaded->Drawing.Order, "auxiliary"));
  CHECK(loaded->Drawing.Order.size() == 8);
  CHECK(std::ranges::none_of(loaded->Drawing.Kept,
                             [](const Section &block) -> bool { return block.Name == "auxiliary"; }));
  const Drawing older = LoadDrawing("foundation-r14.dwg");
  CHECK(Load(Save(older, SaveOptions{.Release = Version::R2000})->Bytes).value().Drawing.Maintenance == 15);
  CHECK(Load(Save(older)->Bytes).value().Drawing.Maintenance == older.Maintenance);
}

TEST_CASE("save/cross: a drawing saved to another release carries the unknown objects whose class converts")
{
  const Drawing older = LoadDrawing("foundation-r14.dwg");
  const auto unknown = [](const Object &one) -> bool
  {
    return Holds<Unknown>(one);
  };
  const auto unknowns = static_cast<std::size_t>(std::ranges::count_if(older.Objects, unknown));
  CHECK(unknowns == 2); // The two view styles.
  // AutoCAD writes both the same way in R14 and R2000: each crosses, its header rewritten.
  const std::expected<Saved, Failure> same = Save(older, SaveOptions{.Release = Version::R2000});
  REQUIRE(same);
  CHECK(same->Issues.empty());
  const std::expected<Loaded, Failure> carried = Load(same->Bytes);
  REQUIRE(carried);
  CHECK(carried->Issues.empty());
  CHECK(static_cast<std::size_t>(std::ranges::count_if(carried->Drawing.Objects, unknown)) == unknowns);
  for (const Object &object : older.Objects)
  {
    if (const auto *before = GetIf<Unknown>(&object))
    {
      const auto *after = GetIf<Unknown>(Find(carried->Drawing, before->Base.Self));
      REQUIRE(after != nullptr);
      CHECK(after->Class == before->Class);
      CHECK(after->Base == before->Base);
      REQUIRE(after->Body);
      CHECK(after->Body->Data == before->Body->Data);
      CHECK(after->Body->Handles == before->Body->Handles);
    }
  }
  // The view styles cross R2007, recast; the cell style map, typed, crosses in the layout of the release.
  const std::expected<Saved, Failure> saved = Save(older, SaveOptions{.Release = Version::R2007});
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Drawing.Release == Version::R2007);
  CHECK(std::ranges::count_if(loaded->Drawing.Objects,
                              [](const Object &one) -> bool { return TypeOf(one) == "CELLSTYLEMAP"; }) == 1);
  CHECK(std::ranges::count_if(loaded->Drawing.Objects,
                              [](const Object &one) -> bool { return TypeOf(one).ends_with("VIEWSTYLE"); }) == 2);
  CHECK(loaded->Drawing.Classes.size() == older.Classes.size());
  QueueAudit(same->Bytes, "dwgio-foundation-r14-carried-r2000.dwg");
}

TEST_CASE("save/cross: a drawing saved before R2000 declares the classes its kinds need")
{
  const Drawing newer = LoadDrawing("curves-r2000.dwg");
  const std::expected<Saved, Failure> saved = Save(newer, SaveOptions{.Release = Version::R14});
  REQUIRE(saved);
  CHECK(
      std::ranges::none_of(saved->Issues, [](const Issue &issue) -> bool { return issue.What.starts_with("class"); }));
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  const auto counted = []<class KindT>(const Drawing &drawing, std::type_identity<KindT>) -> std::size_t
  {
    return static_cast<std::size_t>(
        std::ranges::count_if(drawing.Objects, [](const Object &object) -> bool { return Holds<KindT>(object); }));
  };
  CHECK(counted(loaded->Drawing, std::type_identity<Entities::LwPolyline>{}) ==
        counted(newer, std::type_identity<Entities::LwPolyline>{}));
  CHECK(counted(loaded->Drawing, std::type_identity<Entities::Hatch>{}) ==
        counted(newer, std::type_identity<Entities::Hatch>{}));
  // The records R14 keeps the names it cut in come with it.
  const auto extnames = static_cast<std::size_t>(
      std::ranges::count_if(loaded->Drawing.Objects,
                            [](const Object &object) -> bool
                            {
                              const auto *record = GetIf<Objects::XRecord>(&object);
                              return record != nullptr && !record->Data.empty() &&
                                     record->Data.front() == Tag{.Code = 102, .Value = std::string{"EXTNAMES"}};
                            }));
  CHECK(extnames > 0);
  CHECK(counted(loaded->Drawing, std::type_identity<Objects::XRecord>{}) ==
        counted(newer, std::type_identity<Objects::XRecord>{}) + extnames);
  CHECK(counted(loaded->Drawing, std::type_identity<Entities::LwPolyline>{}) > 0);
  const auto declared = [&loaded](std::string_view dxf) -> const Class *
  {
    const Class *found = nullptr;
    std::size_t times = 0;
    for (const Class &one : loaded->Drawing.Classes)
    {
      if (one.DxfName == dxf)
      {
        found = &one;
        ++times;
      }
    }
    return times == 1 ? found : nullptr;
  };
  REQUIRE(declared("LWPOLYLINE"));
  CHECK(declared("LWPOLYLINE")->CppName == "AcDbPolyline");
  CHECK(declared("LWPOLYLINE")->ItemClass == 0x1F2);
  REQUIRE(declared("HATCH"));
  CHECK(declared("HATCH")->CppName == "AcDbHatch");
  REQUIRE(declared("XRECORD"));
  CHECK(declared("XRECORD")->CppName == "AcDbXrecord");
  CHECK(declared("XRECORD")->ItemClass == 0x1F3);
  CHECK(loaded->Drawing.Classes.size() == newer.Classes.size() + 3);
  CHECK(newer.Classes.size() == LoadDrawing("curves-r2000.dwg").Classes.size());
}

TEST_CASE("save/cross: a typed record of an R14 drawing is written as R2000 with the unknowns carried")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("tables-r14.dwg"));
  REQUIRE(loaded);
  const std::expected<Saved, Failure> saved = Save(loaded->Drawing, SaveOptions{.Release = Version::R2000});
  REQUIRE(saved);
  const auto leftOut = [](const Issue &one) -> bool
  {
    return one.What.contains("left out");
  };
  CHECK(std::ranges::none_of(saved->Issues, leftOut));
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  // Every object carried but the records of the names R14 cut, and the extension dictionaries that held them alone,
  // which AutoCAD reads and keeps none of.
  for (const Object &object : loaded->Drawing.Objects)
  {
    if (Find(back->Drawing, BaseOf(object).Self) != nullptr)
    {
      continue;
    }
    CAPTURE(HexOf(BaseOf(object).Self));
    const auto *record = GetIf<Objects::XRecord>(&object);
    const auto *dictionary = GetIf<Objects::Dictionary>(&object);
    CHECK(((record != nullptr && record->Data.front() == Tag{.Code = 102, .Value = std::string{"EXTNAMES"}}) ||
           (dictionary != nullptr && dictionary->Entries.size() == 1 &&
            dictionary->Entries.front().Name == "ACAD_XREC_ROUNDTRIP")));
  }
  CHECK(back->Drawing.Objects.size() < loaded->Drawing.Objects.size());
  std::vector<std::string> names;
  for (const Object &object : back->Drawing.Objects)
  {
    if (const auto *appid = GetIf<Tables::AppId>(&object))
    {
      names.push_back(appid->Record.Name);
    }
  }
  CHECK(names.size() == 9);
  CHECK(std::ranges::contains(names, "ACAD"));
  CHECK(std::ranges::contains(names, "DWGIO"));
  // The names R14 held in upper case come back as they were, as AutoCAD reads them.
  CHECK(std::ranges::contains(names, "AcadAnnotative"));
}

TEST_CASE("save/cross: an issue raised while writing a typed record names the record")
{
  Drawing drawing = LoadDrawing("tables-r2000.dwg");
  Handle walls;
  for (Object &object : drawing.Objects)
  {
    if (auto *layer = GetIf<Tables::Layer>(&object); layer != nullptr && layer->Record.Name == "Walls")
    {
      layer->Record.Name = std::string(40000, 'w');
      walls = layer->Record.Base.Self;
    }
  }
  REQUIRE(walls != Handle{});
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  REQUIRE(saved->Issues.size() == 1);
  CHECK(saved->Issues.front().Subject == walls);
  CHECK(saved->Issues.front().What == "text cut to 32766 code units");
}

TEST_CASE("save/cross: an R14 dimension style's arrow names and R14-only variables are noted as R2000")
{
  const std::expected<Loaded, Failure> older = Load(ReadFixture("tables-r14.dwg"));
  REQUIRE(older);
  Handle metric;
  for (const Object &object : older->Drawing.Objects)
  {
    if (const auto *style = GetIf<Tables::DimStyle>(&object); style != nullptr && style->Record.Name == "METRIC")
    {
      metric = style->Record.Base.Self;
    }
  }
  REQUIRE(metric != Handle{});
  const std::expected<Saved, Failure> saved = Save(older->Drawing, SaveOptions{.Release = Version::R2000});
  REQUIRE(saved);
  const auto about = std::ranges::find(saved->Issues, metric, &Issue::Subject);
  REQUIRE(about != saved->Issues.end());
  CHECK(about->What == "DIMBLK DOT of R14 not written as R2000");
  CHECK(std::ranges::count(saved->Issues, metric, &Issue::Subject) == 1);
  Drawing edited = older->Drawing;
  for (Object &object : edited.Objects)
  {
    if (auto *style = GetIf<Tables::DimStyle>(&object); style != nullptr && style->Record.Name == "STANDARD")
    {
      style->Dimfit = 5;
      style->Dimunit = 4;
      style->Arrows.Dimblk1 = "OPEN";
      style->Arrows.Dimblk2 = "CLOSED";
    }
  }
  const std::expected<Saved, Failure> again = Save(edited, SaveOptions{.Release = Version::R2000});
  REQUIRE(again);
  const auto noted = [&again](std::string_view what) -> bool
  {
    return std::ranges::any_of(again->Issues, [what](const Issue &one) { return one.What == what; });
  };
  CHECK(noted("DIMFIT 5 of R14 not written as R2000"));
  CHECK(noted("DIMUNIT 4 of R14 not written as R2000"));
  CHECK(noted("DIMBLK1 OPEN of R14 not written as R2000"));
  CHECK(noted("DIMBLK2 CLOSED of R14 not written as R2000"));
}

TEST_CASE("save/cross: a class the drawing lacks is declared without changing the drawing")
{
  // A drawing with no classes at all: the fixtures declare every class AutoCAD knows.
  Drawing drawing;
  drawing.Release = Version::R2000;
  drawing.Order = {"header", "classes", "objects", "map"};
  Objects::Scale scale;
  scale.Base.Self = Handle{0x5000};
  scale.Base.Owner = Handle{0xC};
  scale.Name = "1:7";
  drawing.Objects.emplace_back(scale);
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  CHECK(drawing.Classes.empty());
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  REQUIRE(back->Drawing.Classes.size() == 1);
  CHECK(back->Drawing.Classes[0].DxfName == "SCALE");
  CHECK(back->Drawing.Classes[0].CppName == "AcDbScale");
  CHECK(back->Drawing.Classes[0].Number == 500);
  REQUIRE(back->Drawing.Objects.size() == 1);
  CHECK(Holds<Objects::Scale>(back->Drawing.Objects[0]));
}

TEST_CASE("save/cross: a cut chain upgraded to R2004 keeps what it reaches and attributes chain back as R2000")
{
  // A chain cut after its second link becomes, in R2004, a list of what it reaches; the third entity stays owned and the
  // audit lists it again, in file order.
  Drawing older = Create();
  const Handle one = *Edit::Add(older, Entities::Line{});
  const Handle two = *Edit::Add(older, Entities::Line{});
  const Handle three = *Edit::Add(older, Entities::Line{});
  CommonOf(*Find(older, two))->Next = Handle{.Value = 0x999};
  const std::expected<Saved, Failure> raised = Save(older, SaveOptions{.Release = Version::R2004});
  REQUIRE(raised);
  REQUIRE(raised->Issues.size() == 1);
  CHECK(raised->Issues[0].What == "1 entity of BLOCK_HEADER 1F not on its chain, left out of its list");
  QueueAudit(raised->Bytes, "dwgio-cut-chain-raised-r2004.dwg");
  Drawing back = Load(raised->Bytes).value().Drawing;
  CHECK(View::Block(back, "*Model_Space")->Entities == std::vector<Handle>{one, two});
  CHECK(Find(back, three) != nullptr);
  const Audit::Report report = Audit::Repair(back);
  REQUIRE(report.Findings.size() == 1);
  CHECK(report.Findings[0].What == "entities of block *Model_Space broken");
  CHECK(View::Block(back, "*Model_Space")->Entities == std::vector<Handle>{one, two, three});
  QueueAudit(Save(back).value().Bytes, "dwgio-cut-chain-r2004.dwg");
  // An insert's attributes, listed in R2004, chain from First to Last in R2000, the SEQEND after them.
  Drawing newer = Create(Version::R2004);
  REQUIRE(Edit::AddBlock(newer, "Desk"));
  Entities::Insert insert;
  insert.Block = View::Block(newer, "Desk")->Record.Base.Self;
  insert.Follows = true;
  const Handle placed = *Edit::Add(newer, insert);
  std::vector<Handle> owned;
  for (const char *tag : {"ONE", "TWO"})
  {
    Entities::Attrib attrib;
    attrib.Base.Self = Edit::MakeHandle(newer);
    attrib.Base.Owner = placed;
    attrib.Common.Layer = newer.Header.Clayer;
    attrib.Common.Lineweight = Lineweight::ByLayer;
    attrib.Common.Space =
        Space::Owner; // A member names its owner, as AutoCAD writes them, whatever space its head is in.
    attrib.Tag = tag;
    attrib.Lettering.Value = tag;
    attrib.Lettering.Style = newer.Header.Textstyle;
    attrib.Lettering.Height = 2.5; // AutoCAD's audit fixes a height of zero.
    owned.push_back(attrib.Base.Self);
    newer.Objects.emplace_back(std::move(attrib));
  }
  Entities::SeqEnd seqend;
  seqend.Base.Self = Edit::MakeHandle(newer);
  seqend.Base.Owner = placed;
  seqend.Common.Layer = newer.Header.Clayer;
  seqend.Common.Lineweight = Lineweight::ByLayer;
  seqend.Common.Space = Space::Owner;
  newer.Objects.emplace_back(std::move(seqend));
  auto &head = Get<Entities::Insert>(*Find(newer, placed));
  head.Owned = owned;
  head.SeqEnd = BaseOf(newer.Objects.back()).Self;
  std::string findings;
  for (const Audit::Finding &finding : Audit::Check(newer).Findings)
  {
    findings += finding.What + "; ";
  }
  REQUIRE_MESSAGE(findings.empty(), findings);
  const std::expected<Saved, Failure> lowered = Save(newer, SaveOptions{.Release = Version::R2000});
  REQUIRE(lowered);
  const Drawing round = Load(lowered->Bytes).value().Drawing;
  const auto &was = Get<Entities::Insert>(*Find(round, placed));
  CHECK(was.First == owned[0]);
  CHECK(was.Last == owned[1]);
  CHECK(was.Owned.empty());
  CHECK(CommonOf(*Find(round, owned[0]))->Next == owned[1]);
  CHECK(CommonOf(*Find(round, owned[1]))->Previous == owned[0]);
  std::string left;
  for (const Audit::Finding &finding : Audit::Check(round).Findings)
  {
    left += finding.What + "; ";
  }
  // R2004 keeps no viewport entity header table, so the lowered drawing has none: a warning, as AutoCAD opens the
  // file as it is, and Repair makes the table.
  CHECK(left == "viewport header table missing; ");
  QueueAudit(lowered->Bytes, "dwgio-attributes-r2000.dwg");
  QueueAudit(Save(newer).value().Bytes, "dwgio-attributes-r2004.dwg");
}

TEST_CASE("save/cross: every fixture saved across R2000 and R2004 loads back")
{
  for (const std::string_view name : {"empty", "entities", "tables", "curves", "objects", "foundation"})
  {
    CAPTURE(name);
    const Drawing older = Load(ReadFixture(std::format("{}-r2000.dwg", name))).value().Drawing;
    const std::expected<Saved, Failure> raised = Save(older, SaveOptions{.Release = Version::R2004});
    REQUIRE(raised);
    const std::expected<Loaded, Failure> up = Load(raised->Bytes);
    REQUIRE(up);
    CHECK(up->Issues.empty());
    // An object dwgio does not type crosses where AutoCAD writes its class alike in both releases, or in ways its
    // layout recasts: the cell style map and the view styles do.
    CHECK(std::ranges::none_of(raised->Issues,
                               [](const Issue &issue) -> bool { return issue.What.contains("left out"); }));
    QueueAudit(raised->Bytes, std::format("dwgio-{}-r2000-as-r2004.dwg", name));
    const Drawing newer = Load(ReadFixture(std::format("{}-r2004.dwg", name))).value().Drawing;
    const std::expected<Saved, Failure> lowered = Save(newer, SaveOptions{.Release = Version::R2000});
    REQUIRE(lowered);
    const std::expected<Loaded, Failure> down = Load(lowered->Bytes);
    REQUIRE(down);
    CHECK(down->Issues.empty());
    QueueAudit(lowered->Bytes, std::format("dwgio-{}-r2004-as-r2000.dwg", name));
  }
}

TEST_CASE("save/cross: an R2000 or R14 drawing upgraded to R2004 lists its model space like AutoCAD")
{
  // *Model_Space owns its extension dictionary, as every block record of an R14 file does; no chain links it, and
  // the save as R2004 notes nothing.
  const Drawing theirs = Load(ReadFixture("foundation-r2004.dwg")).value().Drawing;
  for (const std::string_view from : {"r2000", "r14"})
  {
    CAPTURE(from);
    const Drawing older = Load(ReadFixture(std::format("foundation-{}.dwg", from))).value().Drawing;
    const Tables::BlockRecord &space = *View::Block(older, "*Model_Space");
    REQUIRE(space.Record.Base.Dictionary);
    CHECK(BaseOf(*Find(older, space.Record.Base.Dictionary)).Owner == space.Record.Base.Self);
    const std::expected<Saved, Failure> raised = Save(older, SaveOptions{.Release = Version::R2004});
    REQUIRE(raised);
    CHECK_FALSE(std::ranges::any_of(raised->Issues, [](const Issue &issue) -> bool
                                    { return issue.What.contains("not on its chain"); }));
    CHECK(View::Block(Load(raised->Bytes).value().Drawing, "*Model_Space")->Entities ==
          View::Block(theirs, "*Model_Space")->Entities);
  }
}

TEST_CASE("save/cross: a drawing from Create with no preview saves as R2007")
{
  for (const Version release : {Version::R2000, Version::R2007})
  {
    CAPTURE(NameOf(release));
    const Drawing made = Create(release);
    REQUIRE(made.Preview.empty());
    const std::expected<Saved, Failure> saved = Save(made, SaveOptions{.Release = Version::R2007});
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(back->Drawing.Release == Version::R2007);
    CHECK(back->Drawing.Maintenance == (release == Version::R2007 ? made.Maintenance : 50));
    QueueAudit(saved->Bytes, std::format("dwgio-created-{}-as-r2007.dwg", NameOf(release)));
  }
}

TEST_CASE("save/cross: every fixture saved across R2004 to R2018 loads back clean")
{
  struct Pair
  {
    std::string_view From;
    Version To;
  };

  for (const Pair pair : {Pair{.From = "r2004", .To = Version::R2018}, Pair{.From = "r2018", .To = Version::R2004},
                          Pair{.From = "r2010", .To = Version::R2013}, Pair{.From = "r2013", .To = Version::R2010},
                          Pair{.From = "r2018", .To = Version::R2000}, Pair{.From = "r2000", .To = Version::R2010},
                          Pair{.From = "r14", .To = Version::R2018}, Pair{.From = "r2004", .To = Version::R14},
                          Pair{.From = "r2018", .To = Version::R14}, Pair{.From = "r2004", .To = Version::R2007},
                          Pair{.From = "r2007", .To = Version::R2018}, Pair{.From = "r2018", .To = Version::R2007},
                          Pair{.From = "r2007", .To = Version::R2000}, Pair{.From = "r14", .To = Version::R2007},
                          Pair{.From = "r2007", .To = Version::R14}, Pair{.From = "r2013", .To = Version::R2018},
                          Pair{.From = "r2018", .To = Version::R2013}})
  {
    for (const std::string_view name : {"empty", "entities", "tables", "curves", "objects", "foundation"})
    {
      CAPTURE(name);
      CAPTURE(pair.From);
      CAPTURE(NameOf(pair.To));
      const Drawing drawing = Load(ReadFixture(std::format("{}-{}.dwg", name, pair.From))).value().Drawing;
      const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = pair.To});
      REQUIRE(saved);
      const std::expected<Loaded, Failure> back = Load(saved->Bytes);
      REQUIRE(back);
      CHECK(back->Issues.empty());
      CHECK(back->Drawing.Release == pair.To);
      if (name == "curves" || name == "objects")
      {
        QueueAudit(saved->Bytes, std::format("dwgio-{}-{}-as-{}.dwg", name, pair.From, NameOf(pair.To)));
      }
    }
  }
}

TEST_CASE("save/cross: a linetype's dash text converts across R2007 between UTF-16 and the code page")
{
  for (const auto &[from, to, area] : {std::tuple{"tables-r2004.dwg", Version::R2018, std::size_t{512}},
                                       std::tuple{"tables-r2018.dwg", Version::R2004, std::size_t{256}}})
  {
    CAPTURE(from);
    Drawing drawing = Load(ReadFixture(from)).value().Drawing;
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = to});
    REQUIRE(saved);
    const Drawing back = Load(saved->Bytes).value().Drawing;
    const Tables::LType *gas = View::Record<Tables::LType>(back, "GAS_LINE");
    REQUIRE(gas != nullptr);
    CHECK(gas->Strings.size() == area);
    const auto text =
        std::ranges::find_if(gas->Dashes, [](const Tables::Dash &dash) -> bool { return (dash.Flags & 2) != 0; });
    REQUIRE(text != gas->Dashes.end());
    CHECK(text->Text == "GAS");
    CHECK(text->Shape == 0);
  }
  // A text the narrower area cannot hold is left out and noted; a linetype with no text keeps no area from R2007.
  Drawing drawing = Load(ReadFixture("tables-r2018.dwg")).value().Drawing;
  for (Object &object : drawing.Objects)
  {
    if (auto *ltype = GetIf<Tables::LType>(&object); ltype != nullptr && ltype->Record.Name == "GAS_LINE")
    {
      for (Tables::Dash &dash : ltype->Dashes)
      {
        dash.Text = (dash.Flags & 2) != 0 ? std::string(300, 'g') : dash.Text;
      }
    }
  }
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2004});
  REQUIRE(saved);
  CHECK(std::ranges::any_of(saved->Issues, [](const Issue &issue) -> bool
                            { return issue.What.starts_with("text of a dash of LTYPE"); }));
  const Drawing created =
      Load(Save(Create(Version::R2004), SaveOptions{.Release = Version::R2018})->Bytes).value().Drawing;
  CHECK(View::Record<Tables::LType>(created, "Continuous")->Strings.empty());
}

TEST_CASE("save/cross: a pointer from R2007 naming a dropped object is cleared and noted")
{
  Drawing drawing = Load(ReadFixture("empty-r2010.dwg")).value().Drawing;
  const Handle unseen = Unseen(drawing);
  Tables::Layer *layer = nullptr;
  for (Object &object : drawing.Objects)
  {
    if (auto *one = GetIf<Tables::Layer>(&object))
    {
      layer = one;
    }
  }
  REQUIRE(layer != nullptr);
  layer->Material = unseen;
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2018});
  REQUIRE(saved);
  CHECK(std::ranges::any_of(saved->Issues,
                            [unseen](const Issue &issue) -> bool
                            {
                              return issue.What.starts_with("material of LAYER") &&
                                     issue.What.ends_with(std::format("cleared, {} left out", HexOf(unseen)));
                            }));
  const Drawing back = Load(saved->Bytes).value().Drawing;
  CHECK(View::Record<Tables::Layer>(back, layer->Record.Name)->Material == Handle{});
}

TEST_CASE("save/cross: the header's pointers and a view's live section and an entity's material are cleared")
{
  Drawing drawing = Load(ReadFixture("tables-r2010.dwg")).value().Drawing;
  // An untyped object of a class no save to another release carries is left out; the header's material names it.
  const Handle material = Unseen(drawing);
  drawing.Header.Cmaterial = material;
  Entities::Line line;
  line.Common.MaterialOrigin = Origin::Record;
  line.Common.Material = material;
  const Handle drawn = *Edit::Add(drawing, line);
  Tables::View *view = nullptr;
  for (Object &object : drawing.Objects)
  {
    view = view == nullptr ? GetIf<Tables::View>(&object) : view;
  }
  REQUIRE(view != nullptr);
  view->Section = material;
  const Handle viewed = view->Record.Base.Self;
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2013});
  REQUIRE(saved);
  const auto said = [&saved](const std::string &what) -> bool
  {
    return std::ranges::contains(saved->Issues, what, &Issue::What);
  };
  CHECK(said(std::format("CMATERIAL of the header cleared, {} left out", HexOf(material))));
  CHECK(said(std::format("material of LINE {} cleared, {} left out", HexOf(drawn), HexOf(material))));
  CHECK(said(std::format("live section of VIEW {} cleared, {} left out", HexOf(viewed), HexOf(material))));
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  const Header &header = back->Drawing.Header;
  for (const Handle pointer : {header.Cmaterial, header.Dragvs, header.Interfereobjvs, header.Interferevpvs})
  {
    CHECK((pointer == Handle{} || Find(back->Drawing, pointer) != nullptr));
  }
  const Common &common = *CommonOf(*Find(back->Drawing, drawn));
  CHECK(common.Material == Handle{});
  CHECK(common.MaterialOrigin == Origin::ByLayer);
  CHECK(Get<Tables::View>(*Find(back->Drawing, viewed)).Section == Handle{});
  QueueAudit(saved->Bytes, "dwgio-tables-r2010-detached-as-r2013.dwg");
}

TEST_CASE("save/cross: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
