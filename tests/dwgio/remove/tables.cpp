/***********************************************************************************************************************
 *  @file      tests/dwgio/remove/tables.cpp
 *  @brief     Removing blocks and table records, refused while something uses them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/dimension.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/text.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tables/ucs.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tables/vport.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <expected>
#include <format>
#include <string>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  The handles of the objects of a kind, in file order.
 */
template <class KindT>
auto All(const Drawing &drawing) -> std::vector<Handle>
{
  std::vector<Handle> handles;
  for (const Object &object : drawing.Objects)
  {
    if (Holds<KindT>(object))
    {
      handles.push_back(BaseOf(object).Self);
    }
  }
  return handles;
}

template <class RecordT>
auto RecordOf(const Drawing &drawing, const char *name) -> Handle
{
  const RecordT *record = View::Record<RecordT>(drawing, name);
  REQUIRE_MESSAGE(record, name);
  return record->Record.Base.Self;
}

/**
 *  A record's name as the drawing spells it: a file before R2000 keeps names in capitals.
 */
template <class RecordT>
auto RecordNameOf(const Drawing &drawing, const char *name) -> std::string
{
  return View::Record<RecordT>(drawing, name)->Record.Name;
}

/**
 *  Whether any object still names the handle as its own, its owner, a reactor or an extension dictionary.
 */
auto IsNamed(const Drawing &drawing, Handle handle) -> bool
{
  return std::ranges::any_of(drawing.Objects,
                             [handle](const Object &object) -> bool
                             {
                               const Base &base = BaseOf(object);
                               return base.Self == handle || base.Owner == handle || base.Dictionary == handle ||
                                      std::ranges::contains(base.Reactors, handle);
                             });
}

auto CheckRefusal(Drawing &drawing, Handle object) -> std::string
{
  const Drawing before = drawing;
  const std::expected<void, Failure> removed = Edit::Remove(drawing, object);
  REQUIRE(!removed);
  CHECK(drawing == before);
  const Failure &failure = removed.error();
  const FailureCode code = failure.What.ends_with(" in use")    ? FailureCode::InUse
                           : failure.What.ends_with(" missing") ? FailureCode::Missing
                                                                : FailureCode::Refused;
  CHECK(failure.Code == code);
  CHECK_FALSE(failure.Offset);
  return failure.What;
}

/**
 *  The drawing checks clean, saves and loads back with no issue, and AutoCAD's audit finds nothing when asked.
 */
void Sound(const Drawing &drawing, const std::string &name)
{
  CAPTURE(name);
  for (const Audit::Finding &finding : Audit::Check(drawing).Findings)
  {
    CAPTURE(finding.What);
    CHECK(false);
  }
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  for (const Issue &issue : saved->Issues)
  {
    CAPTURE(issue.What);
    CHECK(false);
  }
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  for (const Issue &issue : back->Issues)
  {
    CAPTURE(issue.What);
    CHECK(false);
  }
  CHECK(Audit::Check(back->Drawing).Findings.empty());
  QueueAudit(saved->Bytes, "dwgio-remove-" + name + ".dwg");
}
} // namespace

TEST_CASE("remove/tables: an unused block is removed with its BLOCK and ENDBLK and entities but a space stays")
{
  for (const std::string name : {"tables-r14", "tables-r2000", "tables-r2004", "tables-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    const Handle chair = RecordOf<Tables::BlockRecord>(drawing, "Chair");
    const std::string named = "block " + RecordNameOf<Tables::BlockRecord>(drawing, "Chair");
    CHECK(CheckRefusal(drawing, chair) == named + " in use");
    const std::string model = RecordNameOf<Tables::BlockRecord>(drawing, "*Model_Space");
    const std::string paper = RecordNameOf<Tables::BlockRecord>(drawing, "*Paper_Space");
    CHECK(CheckRefusal(drawing, drawing.Header.BlockRecordModelSpace) == "block " + model + " required");
    CHECK(CheckRefusal(drawing, drawing.Header.BlockRecordPaperSpace) == "block " + paper + " required");
    const std::string dot = RecordNameOf<Tables::BlockRecord>(drawing, "_Dot");
    CHECK(CheckRefusal(drawing, RecordOf<Tables::BlockRecord>(drawing, "_Dot")) == "block " + dot + " in use");
    const Tables::BlockRecord record = Get<Tables::BlockRecord>(*Find(drawing, chair));
    CHECK(CheckRefusal(drawing, record.Block) == std::format("BLOCK {} belongs to {}", HexOf(record.Block), named));
    CHECK(CheckRefusal(drawing, record.EndBlk) == std::format("ENDBLK {} belongs to {}", HexOf(record.EndBlk), named));
    for (const Handle insert : All<Entities::Insert>(drawing))
    {
      REQUIRE(Edit::Remove(drawing, insert));
    }
    const auto owned = [chair](const Object &object) -> bool
    {
      return BaseOf(object).Owner == chair;
    };
    REQUIRE(std::ranges::count_if(drawing.Objects, owned) >= 4);
    REQUIRE(Edit::Remove(drawing, chair));
    CHECK(std::ranges::none_of(drawing.Objects, owned));
    CHECK(Find(drawing, record.Block) == nullptr);
    CHECK(Find(drawing, record.EndBlk) == nullptr);
    CHECK(View::Block(drawing, "Chair") == nullptr);
    CHECK(!std::ranges::contains(View::Control(drawing, Tables::Table::Block)->Entries, chair));
    CHECK(!IsNamed(drawing, chair));
    Sound(drawing, name + "-block");
  }
}

TEST_CASE("remove/tables: a dimension's block and a layout's space are refused")
{
  Drawing curves = LoadFixture("curves-r2000.dwg").Drawing;
  const Handle dimension = All<Entities::Dimension>(curves).front();
  const Handle block = Get<Entities::Dimension>(*Find(curves, dimension)).Block;
  const std::string word = Get<Tables::BlockRecord>(*Find(curves, block)).Record.Name;
  CHECK(CheckRefusal(curves, block) == "block " + word + " in use");
  Drawing objects = LoadFixture("objects-r2018.dwg").Drawing;
  const Objects::Layout *sheet = View::Layout(objects, "Sheet");
  REQUIRE(sheet);
  const std::string space = Get<Tables::BlockRecord>(*Find(objects, sheet->Block)).Record.Name;
  CHECK(CheckRefusal(objects, sheet->Block) == "block " + space + " required");
  CHECK(CheckRefusal(objects, sheet->Base.Self) == "layout Sheet in use");
  for (const Objects::Layout *layout : View::Layouts(objects))
  {
    const auto &record = Get<Tables::BlockRecord>(*Find(objects, layout->Block));
    CHECK(CheckRefusal(objects, layout->Block) == "block " + record.Record.Name + " required");
  }
}

TEST_CASE("remove/tables: removing the viewport a layout last made active leaves it with none")
{
  Drawing drawing = Create();
  Entities::Viewport viewport;
  viewport.Width = 100;
  viewport.Height = 50;
  const Handle added = *Edit::Add(drawing, viewport, Space::Paper);
  const Handle layout = Get<Tables::BlockRecord>(*Find(drawing, drawing.Header.BlockRecordPaperSpace)).Layout;
  Get<Objects::Layout>(*Find(drawing, layout)).Viewport = added;
  Get<Objects::Layout>(*Find(drawing, layout)).Viewports.push_back(added);
  REQUIRE(Edit::Remove(drawing, added));
  CHECK(Get<Objects::Layout>(*Find(drawing, layout)).Viewport == Handle{});
  CHECK(Get<Objects::Layout>(*Find(drawing, layout)).Viewports.empty());
  Sound(drawing, "created-viewport");
}

TEST_CASE("remove/tables: a table record is removed once nothing uses it but one every drawing has stays")
{
  for (const std::string name : {"tables-r14", "tables-r2000", "tables-r2004", "tables-r2018"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name + ".dwg").Drawing;
    CHECK(CheckRefusal(drawing, RecordOf<Tables::LType>(drawing, "HIDDEN")) == "linetype HIDDEN in use");
    REQUIRE(Edit::Remove(drawing, RecordOf<Tables::Layer>(drawing, "Doors")));
    REQUIRE(Edit::Remove(drawing, RecordOf<Tables::LType>(drawing, "HIDDEN")));
    REQUIRE(Edit::Remove(drawing, RecordOf<Tables::Layer>(drawing, "Walls")));
    REQUIRE(Edit::Remove(drawing, RecordOf<Tables::LType>(drawing, "CENTER")));
    const Handle metric = RecordOf<Tables::DimStyle>(drawing, "Metric");
    REQUIRE(drawing.Header.Dimstyle == metric);
    CHECK(CheckRefusal(drawing, metric) ==
          "dimension style " + RecordNameOf<Tables::DimStyle>(drawing, "Metric") + " in use");
    const Handle notes = RecordOf<Tables::Style>(drawing, "Notes");
    const Handle lettering = Get<Tables::DimStyle>(*Find(drawing, metric)).Dimtxsty;
    Get<Tables::DimStyle>(*Find(drawing, metric)).Dimtxsty = notes;
    CHECK(CheckRefusal(drawing, notes) == "text style " + RecordNameOf<Tables::Style>(drawing, "Notes") + " in use");
    Get<Tables::DimStyle>(*Find(drawing, metric)).Dimtxsty = lettering;
    REQUIRE(drawing.Header.Textstyle == RecordOf<Tables::Style>(drawing, "Mono"));
    CHECK(CheckRefusal(drawing, drawing.Header.Textstyle) ==
          "text style " + RecordNameOf<Tables::Style>(drawing, "Mono") + " in use");
    REQUIRE(Edit::Remove(drawing, notes));
    REQUIRE(Edit::Remove(drawing, RecordOf<Tables::View>(drawing, "Plan")));
    REQUIRE(Edit::Remove(drawing, RecordOf<Tables::Ucs>(drawing, "Shifted")));
    REQUIRE(Edit::Remove(drawing, RecordOf<Tables::AppId>(drawing, "DWGIO")));
    for (const Tables::VPort *vport : View::Records<Tables::VPort>(drawing))
    {
      if (View::Matches(vport->Record.Name, "Pair"))
      {
        REQUIRE(Edit::Remove(drawing, vport->Record.Base.Self));
      }
    }
    CHECK(View::Record<Tables::VPort>(drawing, "Pair") == nullptr);
    CHECK(CheckRefusal(drawing, RecordOf<Tables::Layer>(drawing, "0")) == "layer 0 required");
    for (const char *linetype : {"ByBlock", "ByLayer", "Continuous"})
    {
      CHECK(CheckRefusal(drawing, RecordOf<Tables::LType>(drawing, linetype)) ==
            "linetype " + RecordNameOf<Tables::LType>(drawing, linetype) + " required");
    }
    CHECK(CheckRefusal(drawing, RecordOf<Tables::Style>(drawing, "Standard")) ==
          "text style " + RecordNameOf<Tables::Style>(drawing, "Standard") + " required");
    CHECK(CheckRefusal(drawing, RecordOf<Tables::DimStyle>(drawing, "Standard")) ==
          "dimension style " + RecordNameOf<Tables::DimStyle>(drawing, "Standard") + " required");
    CHECK(CheckRefusal(drawing, RecordOf<Tables::AppId>(drawing, "ACAD")) == "application ACAD required");
    CHECK(CheckRefusal(drawing, RecordOf<Tables::VPort>(drawing, "*Active")) ==
          "viewport " + RecordNameOf<Tables::VPort>(drawing, "*Active") + " required");
    CHECK(CheckRefusal(drawing, drawing.Header.LayerControlObject) == "layer table required");
    Sound(drawing, name + "-records");
  }
}

TEST_CASE("remove/tables: a layer or linetype or style an entity uses is refused")
{
  Drawing drawing = LoadFixture("entities-r2000.dwg").Drawing;
  CHECK(CheckRefusal(drawing, RecordOf<Tables::Layer>(drawing, "Walls")) == "layer Walls in use");
  CHECK(CheckRefusal(drawing, RecordOf<Tables::LType>(drawing, "DASHED")) == "linetype DASHED in use");
  CHECK(CheckRefusal(drawing, RecordOf<Tables::Style>(drawing, "Notes")) == "text style Notes in use");
  CHECK(CheckRefusal(drawing, RecordOf<Tables::BlockRecord>(drawing, "Dot")) == "block Dot in use");
  Handle shapes;
  for (const Tables::Style *style : View::Records<Tables::Style>(drawing))
  {
    shapes = style->Shape ? style->Record.Base.Self : shapes;
  }
  REQUIRE(shapes);
  CHECK(CheckRefusal(drawing, shapes) == std::format("text style {} in use", HexOf(shapes)));
  REQUIRE(Edit::Remove(drawing, RecordOf<Tables::LType>(drawing, "CENTER")));
  Sound(drawing, "entities-r2000-linetype");
}

TEST_CASE("remove/tables: the current layer is refused until another is current")
{
  Drawing drawing = Create();
  Tables::Layer walls;
  walls.Record.Name = "Walls";
  walls.Linetype = drawing.Header.LTypeContinuous;
  const Handle layer = *Edit::Add(drawing, walls);
  const Handle zero = drawing.Header.Clayer;
  drawing.Header.Clayer = layer;
  CHECK(CheckRefusal(drawing, layer) == "layer Walls in use");
  drawing.Header.Clayer = zero;
  REQUIRE(Edit::Remove(drawing, layer));
  CHECK(View::Records<Tables::Layer>(drawing).size() == 1);
  CHECK(drawing.Objects.size() == Create().Objects.size());
  Sound(drawing, "created-layer");
}

TEST_CASE("remove/tables: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
