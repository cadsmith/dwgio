/***********************************************************************************************************************
 *  @file      tests/dwgio/edit.cpp
 *  @brief     The Edit functions: new handles, entities, blocks and table records added to a drawing.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/edit.hpp"

#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit/editor.hpp"
#include "dwgio/entities/attdef.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/circle.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/text.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/placeholder.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <expected>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
auto LineAt(double x) -> Object
{
  return Entities::Line{.Base = {},
                        .Common = {},
                        .Thickness = 0,
                        .Start = {.X = x, .Y = 0, .Z = 0},
                        .End = {.X = x, .Y = 1, .Z = 0},
                        .Extrusion = Up};
}
} // namespace

TEST_CASE("edit: NewHandle gives the seed and moves it past every handle in the drawing")
{
  Drawing drawing = Create();
  const Handle seed = drawing.Header.Handseed;
  CHECK(Edit::MakeHandle(drawing) == seed);
  CHECK(drawing.Header.Handseed.Value == seed.Value + 1);
  drawing.Header.Handseed = Handle{.Value = 5};
  const Handle given = Edit::MakeHandle(drawing);
  CHECK(given == seed);
  CHECK(drawing.Header.Handseed.Value == seed.Value + 1);
  CHECK(Find(drawing, given) == nullptr);
}

TEST_CASE("edit: an added entity takes its handle owner space layer and place in the chain")
{
  Drawing drawing = Create();
  const std::expected<Handle, Failure> first = Edit::Add(drawing, LineAt(0));
  REQUIRE(first);
  const Object *object = Find(drawing, *first);
  REQUIRE(object);
  const Common *common = CommonOf(*object);
  REQUIRE(common);
  CHECK(BaseOf(*object).Owner == drawing.Header.BlockRecordModelSpace);
  CHECK(common->Space == Space::Model);
  CHECK(common->Layer == drawing.Header.Clayer);
  CHECK(common->Lineweight == Lineweight::ByLayer);
  CHECK(common->Previous == Handle{});
  CHECK(common->Next == Handle{});
  const Tables::BlockRecord *model = View::Block(drawing, "*Model_Space");
  REQUIRE(model);
  CHECK(model->First == *first);
  CHECK(model->Last == *first);
  const std::expected<Handle, Failure> second = Edit::Add(drawing, LineAt(1), Space::Model);
  REQUIRE(second);
  CHECK(second->Value > first->Value);
  model = View::Block(drawing, "*Model_Space");
  CHECK(model->First == *first);
  CHECK(model->Last == *second);
  CHECK(CommonOf(*Find(drawing, *first))->Next == *second);
  CHECK(CommonOf(*Find(drawing, *second))->Previous == *first);
  CHECK(CommonOf(*Find(drawing, *second))->Next == Handle{});
  const std::vector<const Object *> entities = View::Entities(drawing, *model);
  REQUIRE(entities.size() == 2);
  CHECK(BaseOf(*entities[0]).Self == *first);
  CHECK(BaseOf(*entities[1]).Self == *second);

  Entities::Circle circle;
  circle.Base.Self = Handle{.Value = 0x1000};
  circle.Common.Layer = Handle{.Value = 0x1001};
  circle.Common.Lineweight = Lineweight::Mm018;
  circle.Radius = 2;
  const std::expected<Handle, Failure> third = Edit::Add(drawing, circle, Space::Paper);
  REQUIRE(third);
  CHECK(*third == Handle{.Value = 0x1000});
  CHECK(CommonOf(*Find(drawing, *third))->Space == Space::Paper);
  CHECK(CommonOf(*Find(drawing, *third))->Layer == Handle{.Value = 0x1001});
  CHECK(CommonOf(*Find(drawing, *third))->Lineweight == Lineweight::Mm018);
  CHECK(BaseOf(*Find(drawing, *third)).Owner == drawing.Header.BlockRecordPaperSpace);
  CHECK(View::Block(drawing, "*Paper_Space")->First == *third);
  CHECK(Edit::MakeHandle(drawing).Value > 0x1000);
}

TEST_CASE("edit: a new block comes with its BLOCK and ENDBLK and can be filled and inserted")
{
  Drawing drawing = Create();
  const std::expected<Handle, Failure> chair = Edit::AddBlock(drawing, "Chair");
  REQUIRE(chair);
  const Tables::BlockRecord *record = View::Block(drawing, "chair");
  REQUIRE(record);
  CHECK(record->Record.Base.Self == *chair);
  CHECK(record->Record.Base.Owner == drawing.Header.BlockControlObject);
  CHECK(record->Record.Name == "Chair");
  const Tables::Control *control = View::Control(drawing, Tables::Table::Block);
  REQUIRE(control);
  CHECK(std::ranges::contains(control->Entries, *chair));
  const Object *begin = Find(drawing, record->Block);
  const Object *end = Find(drawing, record->EndBlk);
  REQUIRE(begin);
  REQUIRE(end);
  REQUIRE(Holds<Entities::Block>(*begin));
  REQUIRE(Holds<Entities::EndBlk>(*end));
  CHECK(Get<Entities::Block>(*begin).Name == "Chair");
  CHECK(BaseOf(*begin).Owner == *chair);
  CHECK(BaseOf(*end).Owner == *chair);
  CHECK(CommonOf(*begin)->Space == Space::Owner);
  CHECK(CommonOf(*end)->Layer == drawing.Header.Clayer);
  CHECK(View::Entities(drawing, *record).empty());

  const std::expected<Handle, Failure> leg = Edit::Add(drawing, LineAt(0), "chair");
  REQUIRE(leg);
  CHECK(BaseOf(*Find(drawing, *leg)).Owner == *chair);
  CHECK(CommonOf(*Find(drawing, *leg))->Space == Space::Owner);
  record = View::Block(drawing, "Chair");
  CHECK(record->First == *leg);
  CHECK(record->Last == *leg);

  Entities::Insert insert;
  insert.Block = *chair;
  insert.Position = Vector{.X = 3, .Y = 4, .Z = 0};
  const std::expected<Handle, Failure> placed = Edit::Add(drawing, insert);
  REQUIRE(placed);
  CHECK(Get<Entities::Insert>(*Find(drawing, *placed)).Block == *chair);
  CHECK(View::Block(drawing, "*Model_Space")->First == *placed);

  const std::expected<Handle, Failure> again = Edit::AddBlock(drawing, "CHAIR");
  REQUIRE(!again);
  CHECK(again.error().What == "block CHAIR exists");
  const std::expected<Handle, Failure> nowhere = Edit::Add(drawing, LineAt(0), "Table");
  REQUIRE(!nowhere);
  CHECK(nowhere.error().What == "block Table missing");
  CHECK(drawing.Objects.size() == 35 + 3 + 2);
}

TEST_CASE("edit: a record is added once under a name no other record has in any case")
{
  Drawing drawing = Create();
  Tables::Layer walls;
  walls.Record.Name = "Walls";
  walls.Linetype = drawing.Header.LTypeContinuous;
  const std::expected<Handle, Failure> added = Edit::Add(drawing, walls);
  REQUIRE(added);
  const Tables::Layer *found = View::Record<Tables::Layer>(drawing, "WALLS");
  REQUIRE(found);
  CHECK(found->Record.Base.Self == *added);
  CHECK(found->Record.Base.Owner == drawing.Header.LayerControlObject);
  CHECK(View::Records<Tables::Layer>(drawing).size() == 2);
  walls.Record.Name = "walls";
  const std::expected<Handle, Failure> twice = Edit::Add(drawing, walls);
  REQUIRE(!twice);
  CHECK(twice.error().What == "layer walls exists");
  Tables::LType dashed;
  dashed.Record.Name = "Dashed";
  dashed.Record.Base.Self = Handle{.Value = 0x2000};
  REQUIRE(Edit::Add(drawing, dashed));
  CHECK(View::Record<Tables::LType>(drawing, "dashed")->Record.Base.Self == Handle{.Value = 0x2000});
  const std::expected<Handle, Failure> wrong = Edit::Add(drawing, Object{walls});
  REQUIRE(!wrong);
  CHECK(wrong.error().What == "object LAYER is no entity");
}

TEST_CASE("edit: a refusal names the kind refused")
{
  Drawing drawing = Create();
  CHECK(Edit::Add(drawing, Tables::Layer{.Record = {.Name = "0"}}).error().Code == FailureCode::Exists);
  CHECK(Edit::Add(drawing, LineAt(0), "Missing").error().Code == FailureCode::Missing);
  CHECK(Edit::Add(drawing, LineAt(0), Space::Owner).error().Code == FailureCode::Refused);
  CHECK(Edit::Add(drawing, Objects::Placeholder{}).error().Code == FailureCode::Refused);
}

TEST_CASE("edit: a drawing without its tables refuses every addition and names what it lacks")
{
  Drawing bare;
  CHECK(Edit::Add(bare, LineAt(0)).error().What == "model space missing");
  CHECK(Edit::Add(bare, LineAt(0), Space::Paper).error().What == "paper space missing");
  CHECK(Edit::Add(bare, LineAt(0), Space::Owner).error().What == "owner space refused");
  CHECK(Edit::AddBlock(bare, "Chair").error().What == "block table missing");
  CHECK(Edit::Add(bare, Tables::Layer{}).error().What == "layer table missing");
  CHECK(bare.Objects.empty());
}

TEST_CASE("edit: the filled drawing saves and loads back clean and equal")
{
  Drawing drawing = Create();
  REQUIRE(Edit::AddBlock(drawing, "Chair"));
  REQUIRE(Edit::Add(drawing, LineAt(0), "Chair"));
  REQUIRE(Edit::Add(drawing, LineAt(1), "Chair"));
  Tables::Layer walls;
  walls.Record.Name = "Walls";
  walls.Linetype = drawing.Header.LTypeContinuous;
  const std::expected<Handle, Failure> layer = Edit::Add(drawing, walls);
  REQUIRE(layer);
  Entities::Line wall;
  wall.Common.Layer = *layer;
  wall.End = Vector{.X = 10, .Y = 0, .Z = 0};
  REQUIRE(Edit::Add(drawing, wall));
  Entities::Insert insert;
  insert.Block = View::Block(drawing, "Chair")->Record.Base.Self;
  REQUIRE(Edit::Add(drawing, insert));
  Entities::Circle circle;
  circle.Radius = 1;
  REQUIRE(Edit::Add(drawing, circle, Space::Paper));
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  Drawing again = back->Drawing;
  again.Order.clear();
  again.Kept.clear();
  CHECK(again == drawing);
  CHECK(View::Entities(drawing, *View::Block(drawing, "*Model_Space")).size() == 2);
  QueueAudit(saved->Bytes, "dwgio-edit.dwg");
}

TEST_CASE("edit: a handle an addition brings moves the seed past it")
{
  Drawing drawing = Create();
  const Handle seed = drawing.Header.Handseed;
  Entities::Line far;
  far.Base.Self = Handle{.Value = seed.Value + 0x100};
  REQUIRE(Edit::Add(drawing, far));
  CHECK(drawing.Header.Handseed.Value == seed.Value + 0x101);
  Tables::AppId application;
  application.Record.Name = "DWGIO";
  application.Record.Base.Self = Handle{.Value = seed.Value + 0x10};
  REQUIRE(Edit::Add(drawing, application));
  CHECK(drawing.Header.Handseed.Value == seed.Value + 0x101);
  CHECK(Audit::Check(drawing).IsClean());
}

TEST_CASE("edit: a handle in use is refused and the drawing left untouched")
{
  Drawing drawing = Create();
  const std::size_t count = drawing.Objects.size();
  Entities::Line line;
  line.Base.Self = drawing.Header.Clayer;
  const std::expected<Handle, Failure> taken = Edit::Add(drawing, line);
  REQUIRE(!taken);
  CHECK(taken.error().What == "handle 10 exists");
  Tables::Layer walls;
  walls.Record.Name = "Walls";
  walls.Record.Base.Self = drawing.Header.Clayer;
  CHECK(Edit::Add(drawing, walls).error().What == "handle 10 exists");
  CHECK(drawing.Objects.size() == count);
  CHECK(View::Block(drawing, "*Model_Space")->First == Handle{});
  CHECK(View::Records<Tables::Layer>(drawing).size() == 1);
}

TEST_CASE("edit: a block whose chain is cut refuses a new entity")
{
  Drawing drawing = Create();
  const std::size_t count = drawing.Objects.size();
  const Handle seed = drawing.Header.Handseed;
  Tables::BlockRecord *model = GetIf<Tables::BlockRecord>(Find(drawing, drawing.Header.BlockRecordModelSpace));
  REQUIRE(model);
  model->Last = Handle{.Value = 0x999};
  CHECK(Edit::Add(drawing, LineAt(0)).error().What == "chain of block *Model_Space cut");
  model->Last = drawing.Header.Clayer;
  CHECK(Edit::Add(drawing, LineAt(0)).error().What == "chain of block *Model_Space cut");
  CHECK(drawing.Objects.size() == count);
  CHECK(drawing.Header.Handseed == seed);
}

TEST_CASE("edit: a record is listed in its table's control however the control is numbered")
{
  Drawing drawing;
  drawing.Objects.emplace_back(Objects::Placeholder{});
  Tables::Control layers;
  layers.Table = Tables::Table::Layer;
  drawing.Objects.emplace_back(layers);
  Tables::Layer zero;
  zero.Record.Name = "0";
  const std::expected<Handle, Failure> added = Edit::Add(drawing, zero);
  REQUIRE(added);
  CHECK(*added == Handle{.Value = 1});
  CHECK(Get<Tables::Control>(drawing.Objects[1]).Entries == std::vector<Handle>{*added});
  CHECK(BaseOf(drawing.Objects[2]).Owner == Handle{});
}

TEST_CASE("edit: an ATTDEF marks its block as holding definitions")
{
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2018})
  {
    CAPTURE(NameOf(release));
    Drawing drawing = Create(release);
    const Handle door = *Edit::AddBlock(drawing, "Door");
    REQUIRE(Edit::Add(drawing, LineAt(0), "Door"));
    CHECK(!View::Block(drawing, "Door")->Attributes);
    Entities::AttDef number;
    number.Tag = "NUMBER";
    number.Lettering.Height = 1;
    REQUIRE(Edit::Add(drawing, number, "Door"));
    CHECK(View::Block(drawing, "Door")->Attributes);
    REQUIRE(Edit::Add(drawing, LineAt(1), "Door"));
    CHECK(View::Block(drawing, "Door")->Attributes);
    CHECK(!View::Block(drawing, "*Model_Space")->Attributes);
    Entities::Insert insert;
    insert.Block = door;
    const Handle one = *Edit::Add(drawing, insert);
    const Handle two = *Edit::Add(drawing, insert, Space::Paper);
    const std::vector<Handle> listed =
        release >= Version::R2000 ? std::vector<Handle>{one, two} : std::vector<Handle>{};
    CHECK(View::Block(drawing, "Door")->Inserts == listed);
    insert.Block = {};
    REQUIRE(Edit::Add(drawing, insert));
    CHECK(View::Block(drawing, "Door")->Inserts == listed);
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    const Drawing back = Load(saved->Bytes).value().Drawing;
    CHECK(View::Block(back, "Door")->Attributes);
    CHECK(View::Block(back, "Door")->Inserts == listed);
    REQUIRE(Edit::Remove(drawing, one));
    CHECK(View::Block(drawing, "Door")->Inserts ==
          (release >= Version::R2000 ? std::vector<Handle>{two} : std::vector<Handle>{}));
  }
}

TEST_CASE("edit: adding to an R2004 drawing appends to the block's list and links no chain")
{
  Drawing drawing = Create(Version::R2004);
  const Handle drawn = *Edit::Add(drawing, Entities::Line{});
  const Handle second = *Edit::Add(drawing, Entities::Line{});
  const Tables::BlockRecord &model = *View::Block(drawing, "*Model_Space");
  CHECK(model.Entities == std::vector<Handle>{drawn, second});
  CHECK(model.First == Handle{});
  CHECK(model.Last == Handle{});
  CHECK(CommonOf(*Find(drawing, second))->Previous == Handle{});
  CHECK(View::Entities(drawing, model).size() == 2);
}
} // namespace DwgIO

namespace DwgIO
{
TEST_CASE("edit: a text added with no style takes $TEXTSTYLE")
{
  Drawing drawing = Load(ReadFixture("empty-r2018.dwg")).value().Drawing;
  Entities::MText note;
  note.Value = "Added";
  note.Height = 2.5;
  const Handle added = *Edit::Add(drawing, note);
  CHECK(Get<Entities::MText>(*Find(drawing, added)).Style == drawing.Header.Textstyle);
  Entities::Text label;
  label.Lettering.Value = "Label";
  label.Lettering.Height = 2.5;
  label.Lettering.Style = Handle{0x99};
  const Handle kept = *Edit::Add(drawing, label);
  CHECK(Get<Entities::Text>(*Find(drawing, kept)).Lettering.Style == Handle{0x99});
  Get<Entities::Text>(*Find(drawing, kept)).Lettering.Style = drawing.Header.Textstyle;
  Entities::MText styled = note;
  styled.Style = drawing.Header.Textstyle;
  const Handle own = *Edit::Add(drawing, styled);
  CHECK(Get<Entities::MText>(*Find(drawing, own)).Style == drawing.Header.Textstyle);
  Entities::Text plain;
  plain.Lettering.Value = "Plain";
  plain.Lettering.Height = 2.5;
  const Handle filled = *Edit::Add(drawing, plain);
  CHECK(Get<Entities::Text>(*Find(drawing, filled)).Lettering.Style == drawing.Header.Textstyle);
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  QueueAudit(saved->Bytes, "dwgio-edit-r2018.dwg");
}

TEST_CASE("edit: AddRecord takes only records and an entity keeps its own text style")
{
  Drawing drawing = Create();
  CHECK(Edit::AddRecord(drawing, Object{Entities::Line{}}).error() ==
        Failure{.Code = FailureCode::Refused, .What = "LINE is no record to add"});
  CHECK(Edit::AddRecord(drawing, Object{Tables::BlockRecord{}}).error().Code == FailureCode::Refused);
  Entities::Text text;
  text.Lettering.Style = Handle{.Value = 0x11};
  const Handle written = *Edit::Add(drawing, Object{text});
  CHECK(Get<Entities::Text>(*Find(drawing, written)).Lettering.Style == Handle{.Value = 0x11});
  const Edit::Editor edits{drawing};
  CHECK(edits.Find(written) == Find(std::as_const(drawing), written));
}

TEST_CASE("edit: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
