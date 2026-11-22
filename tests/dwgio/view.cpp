/***********************************************************************************************************************
 *  @file      tests/dwgio/view.cpp
 *  @brief     The View functions: names, tables, spaces, layouts and a block's entities in order.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/view.hpp"

#include "support/fixtures.hpp"

#include "dwgio/appearance.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attdef.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/circle.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/index.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tables/viewportheader.hpp"

#include <doctest/doctest.h>

#include <clocale>
#include <cstddef>
#include <expected>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  The handles of a block record's entities as the file has them, in file order: every entity it owns but its BLOCK and
 *  ENDBLK.
 */
auto CollectEntities(const Drawing &drawing, const Tables::BlockRecord &record) -> std::vector<Handle>
{
  std::vector<Handle> handles;
  for (const Object &object : drawing.Objects)
  {
    const Handle self = BaseOf(object).Self;
    if (CommonOf(object) != nullptr && BaseOf(object).Owner == record.Record.Base.Self && self != record.Block &&
        self != record.EndBlk)
    {
      handles.push_back(self);
    }
  }
  return handles;
}

auto Handles(const std::vector<const Object *> &objects) -> std::vector<Handle>
{
  std::vector<Handle> handles;
  for (const Object *object : objects)
  {
    handles.push_back(BaseOf(*object).Self);
  }
  return handles;
}
} // namespace

TEST_CASE("view: names compare without case like AutoCAD")
{
  CHECK(View::Matches("Walls", "WALLS"));
  CHECK(View::Matches("walls", "Walls"));
  CHECK_FALSE(View::Matches("Walls", "Wall"));
  CHECK_FALSE(View::Matches("", "a"));
  CHECK(View::Matches("", ""));
}

TEST_CASE("view: names fold ASCII case only whatever the locale")
{
  const std::string kept = std::setlocale(LC_CTYPE, nullptr);
  // Where the locale exists its toupper maps 0xE9 to 0xC9; AutoCAD's names fold only ASCII.
  std::setlocale(LC_CTYPE, "fr_FR.ISO8859-1");
  CHECK_FALSE(View::Matches("\xe9", "\xc9"));
  CHECK(View::Matches("\xe9t\xe9", "\xe9T\xe9"));
  std::setlocale(LC_CTYPE, kept.c_str());
}

TEST_CASE("view: the tables answer by kind and by name through their controls")
{
  const Drawing drawing = LoadFixture("tables-r2000.dwg").Drawing;
  const Tables::Control *control = View::Control(drawing, Tables::Table::Layer);
  REQUIRE(control);
  CHECK(control->Base.Self == drawing.Header.LayerControlObject);
  CHECK(View::Control(drawing, Tables::Table::ViewportEntityHeader)->Base.Self ==
        drawing.Header.ViewportEntityHeaderControlObject);
  const Tables::Layer *walls = View::Record<Tables::Layer>(drawing, "walls");
  REQUIRE(walls);
  CHECK(walls->Record.Name == "Walls");
  CHECK(View::Record<Tables::Layer>(drawing, "Roof") == nullptr);
  const std::vector<const Tables::LType *> ltypes = View::Records<Tables::LType>(drawing);
  const Tables::Control *linetypes = View::Control(drawing, Tables::Table::LType);
  REQUIRE(linetypes);
  REQUIRE(ltypes.size() == linetypes->Entries.size() + 2);
  for (std::size_t at = 0; at < linetypes->Entries.size(); ++at)
  {
    CHECK(ltypes[at]->Record.Base.Self == linetypes->Entries[at]);
  }
  CHECK(ltypes[ltypes.size() - 2]->Record.Base.Self == linetypes->ByBlock);
  CHECK(ltypes.back()->Record.Base.Self == linetypes->ByLayer);
  CHECK(View::Record<Tables::LType>(drawing, "bylayer")->Record.Base.Self == drawing.Header.LTypeByLayer);
  CHECK(View::Records<Tables::ViewportEntityHeader>(drawing).size() ==
        View::Control(drawing, Tables::Table::ViewportEntityHeader)->Entries.size());
  const Tables::BlockRecord *model = View::Block(drawing, "*model_space");
  REQUIRE(model);
  CHECK(model->Record.Base.Self == drawing.Header.BlockRecordModelSpace);
  CHECK(View::Block(drawing, "Nowhere") == nullptr);
}

TEST_CASE("view: *Paper_Space is the active layout's paper space block")
{
  for (const char *name : {"curves-r2000.dwg", "curves-r2004.dwg", "curves-r2018.dwg"})
  {
    CAPTURE(name);
    const Drawing drawing = LoadFixture(name).Drawing;
    const Tables::Control *control = View::Control(drawing, Tables::Table::Block);
    REQUIRE(control);
    std::size_t named = 0;
    for (const Handle entry : control->Entries)
    {
      const auto *record = GetIf<Tables::BlockRecord>(Find(drawing, entry));
      named += record != nullptr && View::Matches(record->Record.Name, "*Paper_Space") ? 1U : 0U;
    }
    REQUIRE(named > 0);
    const Tables::BlockRecord *paper = View::Block(drawing, "*PAPER_SPACE");
    REQUIRE(paper);
    CHECK(paper->Record.Base.Self == drawing.Header.BlockRecordPaperSpace);
    CHECK(paper->Record.Base.Self == control->PaperSpace);
    CHECK(View::Block(drawing, "*Model_Space")->Record.Base.Self == control->ModelSpace);
    const std::vector<const Tables::BlockRecord *> records = View::Records<Tables::BlockRecord>(drawing);
    REQUIRE(records.size() >= 2);
    CHECK(records[records.size() - 2]->Record.Base.Self == control->ModelSpace);
    CHECK(records.back()->Record.Base.Self == control->PaperSpace);
    CHECK(View::Record<Tables::LType>(drawing, "ByBlock")->Record.Base.Self == drawing.Header.LTypeByBlock);
  }
}

TEST_CASE("view: a search by name ignores an entry naming nothing or a record of another kind")
{
  Drawing drawing = Create();
  Tables::Layer layer;
  layer.Record.Name = "Walls";
  const Handle walls = *Edit::Add(drawing, layer);
  auto &control = Get<Tables::Control>(*Find(drawing, drawing.Header.LayerControlObject));
  control.Entries.insert(control.Entries.begin(), {Handle{.Value = 0x999}, drawing.Header.LTypeContinuous});
  CHECK(View::Record<Tables::Layer>(drawing, "walls")->Record.Base.Self == walls);
  CHECK(View::Record<Tables::Layer>(drawing, "Continuous") == nullptr);
}

TEST_CASE("view: a block's entities come in drawing order from a chain or from their owner")
{
  Drawing drawing = LoadFixture("entities-r2000.dwg").Drawing;
  const Tables::BlockRecord *model = View::Block(drawing, "*Model_Space");
  REQUIRE(model);
  const std::vector<const Object *> entities = View::Entities(drawing, *model);
  CHECK(Handles(entities) == CollectEntities(drawing, *model));
  CHECK(entities.size() > 20);
  const Tables::BlockRecord *tag = View::Block(drawing, "Tag");
  REQUIRE(tag);
  const std::vector<const Object *> parts = View::Entities(drawing, *tag);
  REQUIRE(parts.size() == 3);
  CHECK(Holds<Entities::Line>(*parts[0]));
  CHECK(Holds<Entities::AttDef>(*parts[1]));
  CHECK(Holds<Entities::AttDef>(*parts[2]));
  CHECK(Handles(parts) == CollectEntities(drawing, *tag));
  // A chain cut at its start, and one that loops back on itself, both fall back to the owner.
  Object *record = Find(drawing, tag->Record.Base.Self);
  REQUIRE(record);
  auto &mutableTag = Get<Tables::BlockRecord>(*record);
  mutableTag.First = Handle{0xFFFFFF};
  CHECK(Handles(View::Entities(drawing, mutableTag)) == CollectEntities(drawing, mutableTag));
  mutableTag.First = parts.front() != nullptr ? BaseOf(*parts.front()).Self : Handle{};
  Object *first = Find(drawing, mutableTag.First);
  REQUIRE(first);
  Get<Entities::Line>(*first).Common.Next = mutableTag.First;
  CHECK(Handles(View::Entities(drawing, mutableTag)) == CollectEntities(drawing, mutableTag));
  mutableTag.First = Handle{};
  CHECK(Handles(View::Entities(drawing, mutableTag)) == CollectEntities(drawing, mutableTag));
}

TEST_CASE("view: layouts come in tab order and dictionaries by name from the root")
{
  const Drawing drawing = LoadFixture("objects-r2000.dwg").Drawing;
  const std::vector<const Objects::Layout *> layouts = View::Layouts(drawing);
  REQUIRE(layouts.size() == 4);
  CHECK(layouts[0]->Name == "Model");
  CHECK(layouts[1]->Name == "Layout1");
  CHECK(layouts[2]->Name == "Layout2");
  CHECK(layouts[3]->Name == "Sheet");
  const Objects::Layout *sheet = View::Layout(drawing, "sheet");
  REQUIRE(sheet);
  CHECK(sheet->Order == 3);
  CHECK(View::Layout(drawing, "Plan") == nullptr);
  const Objects::Dictionary *root = View::Root(drawing);
  REQUIRE(root);
  CHECK(root->Base.Self == drawing.Header.DictionaryNamedObjects);
  const Objects::Dictionary *named = View::Dictionary(drawing, "acad_layout");
  REQUIRE(named);
  CHECK(named->Entries.size() == 4);
  CHECK(View::Dictionary(drawing, "ACAD_NOTHING") == nullptr);
  const Object *own = View::Entry(drawing, *root, "DWGIO_DICT");
  REQUIRE(own);
  CHECK(Holds<Objects::Dictionary>(*own));
  CHECK(View::Entry(drawing, *root, "dwgio_dict") == own);
  CHECK(View::Entry(drawing, *root, "DWGIO_NONE") == nullptr);
}

TEST_CASE("view: Find gives an object that can be changed")
{
  Drawing drawing = LoadFixture("tables-r2000.dwg").Drawing;
  Object *object = Find(drawing, drawing.Header.BlockRecordModelSpace);
  REQUIRE(object);
  CHECK(BaseOf(*object).Self == drawing.Header.BlockRecordModelSpace);
  CHECK(Find(drawing, Handle{}) == nullptr);
  auto &record = Get<Tables::BlockRecord>(*object);
  record.Description = "changed";
  CHECK(View::Block(drawing, "*Model_Space")->Description == "changed");
}

TEST_CASE("view: a drawing without its roots gives nothing and so do dangling chains and layout entries")
{
  const Drawing bare;
  CHECK(View::Root(bare) == nullptr);
  CHECK(View::Dictionary(bare, "ACAD_LAYOUT") == nullptr);
  CHECK(View::Layouts(bare).empty());
  CHECK(View::Control(bare, Tables::Table::Layer) == nullptr);
  CHECK(View::Records<Tables::Layer>(bare).empty());
  CHECK(View::Record<Tables::Layer>(bare, "0") == nullptr);

  Drawing drawing = Create();
  REQUIRE(Edit::AddBlock(drawing, "Chair"));
  const std::expected<Handle, Failure> leg = Edit::Add(drawing, Entities::Circle{}, "Chair");
  REQUIRE(leg);
  // Model space's chain starting at an entity another block owns is no chain: the fallback lists what it owns.
  REQUIRE(Edit::Add(drawing, Entities::Circle{}));
  Tables::BlockRecord *model = GetIf<Tables::BlockRecord>(Find(drawing, drawing.Header.BlockRecordModelSpace));
  REQUIRE(model);
  const Handle own = model->First;
  model->First = *leg;
  const std::vector<const Object *> entities = View::Entities(drawing, *model);
  REQUIRE(entities.size() == 1);
  CHECK(BaseOf(*entities[0]).Self == own);
  // A layout entry naming no object is passed over.
  Objects::Dictionary *layouts = GetIf<Objects::Dictionary>(Find(drawing, drawing.Header.DictionaryLayouts));
  REQUIRE(layouts);
  layouts->Entries.push_back(Objects::Entry{.Name = "Ghost", .Object = Handle{.Value = 0x999}});
  CHECK(View::Layouts(drawing).size() == 2);
}

TEST_CASE("view: a chain that ends early or starts at the BLOCK falls back to the entities the block owns")
{
  Drawing drawing = Create();
  const std::expected<Handle, Failure> first = Edit::Add(drawing, Entities::Circle{});
  const std::expected<Handle, Failure> second = Edit::Add(drawing, Entities::Circle{});
  const std::expected<Handle, Failure> third = Edit::Add(drawing, Entities::Circle{});
  REQUIRE((first && second && third));
  CommonOf(*Find(drawing, *second))->Next = {};
  Tables::BlockRecord *model = GetIf<Tables::BlockRecord>(Find(drawing, drawing.Header.BlockRecordModelSpace));
  REQUIRE(model);
  std::vector<const Object *> entities = View::Entities(drawing, *model);
  REQUIRE(entities.size() == 3);
  CHECK(BaseOf(*entities[0]).Self == *first);
  CHECK(BaseOf(*entities[2]).Self == *third);
  CommonOf(*Find(drawing, *second))->Next = *third;
  model->First = model->Block;
  entities = View::Entities(drawing, *model);
  REQUIRE(entities.size() == 3);
  CHECK(BaseOf(*entities[0]).Self == *first);
  model->First = model->EndBlk;
  CHECK(View::Entities(drawing, *model).size() == 3);
}

TEST_CASE("view: an R2004 block's entities come from its list in its order")
{
  const Drawing drawing = Load(ReadFixture("entities-r2004.dwg")).value().Drawing;
  const Tables::BlockRecord &model = *View::Block(drawing, "*Model_Space");
  REQUIRE_FALSE(model.Entities.empty());
  const std::vector<const Object *> entities = View::Entities(drawing, model);
  CHECK(entities.size() == model.Entities.size());
  CHECK(BaseOf(*entities.front()).Self == model.Entities.front());
  CHECK(BaseOf(*entities.back()).Self == model.Entities.back());
  // A handle naming nothing, or no entity, is passed over.
  Drawing odd = drawing;
  auto &listed = Get<Tables::BlockRecord>(*Find(odd, odd.Header.BlockRecordModelSpace));
  listed.Entities.push_back(Handle{.Value = 0x999});
  listed.Entities.push_back(odd.Header.Clayer);
  CHECK(View::Entities(odd, listed).size() == entities.size());
}
} // namespace DwgIO

namespace DwgIO
{
TEST_CASE("view: an index finds every object the drawing finds and the view gives the same through it")
{
  for (const char *name : {"tables-r2000.dwg", "objects-r2004.dwg", "curves-r2018.dwg", "kinds-r14.dwg"})
  {
    CAPTURE(name);
    const Drawing drawing = LoadFixture(name).Drawing;
    const Index index{drawing};
    CHECK(&index.Source() == &drawing);
    for (const Object &object : drawing.Objects)
    {
      const Handle self = BaseOf(object).Self;
      CHECK(Find(index, self) == Find(drawing, self));
    }
    CHECK(Find(index, Handle{}) == nullptr);
    CHECK(Find(index, Handle{.Value = 0xFFFFFFF}) == nullptr);
    CHECK(View::Records<Tables::Layer>(index) == View::Records<Tables::Layer>(drawing));
    CHECK(View::Record<Tables::LType>(index, "continuous") == View::Record<Tables::LType>(drawing, "CONTINUOUS"));
    CHECK(View::Control(index, Tables::Table::Style) == View::Control(drawing, Tables::Table::Style));
    CHECK(View::Block(index, "*model_space") == View::ModelSpace(index));
    CHECK(View::ModelSpace(index) == View::ModelSpace(drawing));
    CHECK(View::PaperSpace(index) == View::PaperSpace(drawing));
    CHECK(View::PaperSpace(index) == View::Block(drawing, "*Paper_Space"));
    CHECK(View::Entities(index, *View::ModelSpace(index)) == View::Entities(drawing, *View::ModelSpace(drawing)));
    CHECK(View::Layouts(index) == View::Layouts(drawing));
    CHECK(View::Layout(index, "model") == View::Layout(drawing, "Model"));
    CHECK(View::Root(index) == View::Root(drawing));
    CHECK(View::Dictionary(index, "ACAD_GROUP") == View::Dictionary(drawing, "ACAD_GROUP"));
    if (const Objects::Dictionary *root = View::Root(index))
    {
      CHECK(View::Entry(index, *root, "acad_group") == View::Entry(drawing, *root, "ACAD_GROUP"));
    }
  }
}

TEST_CASE("view: a drawing without tables root or spaces gives nothing through an index")
{
  const Drawing bare;
  const Index index{bare};
  CHECK(View::Records<Tables::Layer>(index).empty());
  CHECK(View::Record<Tables::Layer>(index, "0") == nullptr);
  CHECK(View::ModelSpace(index) == nullptr);
  CHECK(View::PaperSpace(bare) == nullptr);
  CHECK(View::Root(index) == nullptr);
  CHECK(View::Dictionary(index, "ACAD_LAYOUT") == nullptr);
  CHECK(View::Layouts(index).empty());
  CHECK(View::Control(index, static_cast<Tables::Table>(200)) == nullptr);
}

TEST_CASE("view: a record found in a mutable drawing can be changed in place")
{
  Drawing drawing = LoadFixture("tables-r2000.dwg").Drawing;
  Tables::Layer *walls = View::Record<Tables::Layer>(drawing, "walls");
  REQUIRE(walls != nullptr);
  walls->Locked = true;
  CHECK(View::Record<Tables::Layer>(std::as_const(drawing), "Walls")->Locked);
  for (Tables::Layer *layer : View::Records<Tables::Layer>(drawing))
  {
    layer->Plot = false;
  }
  CHECK_FALSE(View::Record<Tables::Layer>(std::as_const(drawing), "0")->Plot);
}

TEST_CASE("view: an insert's attributes are the same listed from R2004 or chained before")
{
  std::vector<std::vector<std::string>> seen;
  for (const char *name : {"entities-r2000.dwg", "entities-r2004.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const Drawing drawing = LoadFixture(name).Drawing;
    const Index index{drawing};
    std::vector<std::string> values;
    for (const Object &object : drawing.Objects)
    {
      if (const auto *insert = GetIf<Entities::Insert>(&object); insert != nullptr && insert->Follows)
      {
        const std::vector<const Entities::Attrib *> attributes = View::Attributes(index, *insert);
        CHECK(attributes == View::Attributes(drawing, *insert));
        CHECK_FALSE(attributes.empty());
        for (const Entities::Attrib *attribute : attributes)
        {
          values.push_back(attribute->Tag + "=" + attribute->Lettering.Value);
        }
      }
    }
    seen.push_back(values);
  }
  CHECK(seen[0] == seen[1]);
  CHECK(seen[0] == seen[2]);
}

TEST_CASE("view: a sequence skips objects that are not members and stops at a loop or a gap")
{
  Drawing drawing;
  Entities::Attrib first;
  first.Base.Self = Handle{.Value = 0x10};
  first.Common.Next = Handle{.Value = 0x11};
  Entities::Attrib second;
  second.Base.Self = Handle{.Value = 0x11};
  second.Common.Next = Handle{.Value = 0x10};
  Entities::Line stray;
  stray.Base.Self = Handle{.Value = 0x12};
  Entities::Vertex corner;
  corner.Base.Self = Handle{.Value = 0x20};
  corner.Common.Next = Handle{.Value = 0x20};
  drawing.Objects = {Object{first}, Object{second}, Object{stray}, Object{corner}};
  const Index index{drawing};
  Entities::Insert looped;
  looped.First = Handle{.Value = 0x10};
  looped.Last = Handle{.Value = 0x99};
  CHECK(View::Attributes(index, looped).size() == 2);
  Entities::Insert gapped;
  gapped.First = Handle{.Value = 0x12};
  CHECK(View::Attributes(index, gapped).empty());
  gapped.First = Handle{.Value = 0x77};
  CHECK(View::Attributes(index, gapped).empty());
  Entities::Insert listed;
  listed.Owned = {Handle{.Value = 0x12}, Handle{.Value = 0x11}};
  CHECK(View::Attributes(drawing, listed).size() == 1);
  Entities::Polyline polyline;
  polyline.First = Handle{.Value = 0x20};
  CHECK(View::Vertices(index, polyline).size() == 1);
  polyline.First = Handle{.Value = 0x12};
  CHECK(View::Vertices(index, polyline).empty());
  polyline.Vertices = {Handle{.Value = 0x12}, Handle{.Value = 0x20}};
  CHECK(View::Vertices(index, polyline).size() == 1);
  CHECK(View::Layout(index, "Missing") == nullptr);
  const Drawing fixture = LoadFixture("tables-r2004.dwg").Drawing;
  const Index whole{fixture};
  CHECK(View::Entry(whole, *View::Root(whole), "NO_SUCH_ENTRY") == nullptr);
}

TEST_CASE("view: a polyline's vertices are the same listed from R2004 or chained before")
{
  std::vector<std::size_t> counts;
  for (const char *name : {"curves-r2000.dwg", "curves-r2004.dwg"})
  {
    CAPTURE(name);
    const Drawing drawing = LoadFixture(name).Drawing;
    const Index index{drawing};
    std::size_t count = 0;
    for (const Object &object : drawing.Objects)
    {
      if (const auto *polyline = GetIf<Entities::Polyline>(&object))
      {
        const std::vector<const Entities::Vertex *> vertices = View::Vertices(index, *polyline);
        CHECK(vertices == View::Vertices(drawing, *polyline));
        CHECK_FALSE(vertices.empty());
        count += vertices.size();
      }
    }
    counts.push_back(count);
  }
  CHECK(counts[0] == counts[1]);
}
} // namespace DwgIO
