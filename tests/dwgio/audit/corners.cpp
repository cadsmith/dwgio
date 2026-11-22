/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/corners.cpp
 *  @brief     The corners of the audit: what a damaged drawing may hold that the plain tests do not reach.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/findings.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/mline.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/plot.hpp"
#include "dwgio/objects/scale.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tables/ucs.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

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
constexpr Handle Nowhere{.Value = 0x999};

void Erase(Drawing &drawing, Handle self)
{
  std::erase_if(drawing.Objects, [self](const Object &object) -> bool { return BaseOf(object).Self == self; });
}

auto Model(Drawing &drawing) -> Tables::BlockRecord &
{
  return *GetIf<Tables::BlockRecord>(Find(drawing, drawing.Header.BlockRecordModelSpace));
}
} // namespace

TEST_CASE("audit/corners: header handles naming the wrong object or one in use are corrected")
{
  Drawing drawing = Create();
  Erase(drawing, drawing.Header.LayerControlObject);
  drawing.Header.LayerControlObject = drawing.Header.Clayer; // In use by the layer itself.
  drawing.Header.DictionaryAcadGroup = Nowhere;
  drawing.Header.Textstyle = drawing.Header.Clayer; // A layer where a text style belongs.
  Get<Tables::Control>(*Find(drawing, drawing.Header.BlockControlObject)).PaperSpace = {};
  Get<Tables::Control>(*Find(drawing, drawing.Header.LTypeControlObject)).ByLayer = {};
  CHECK(FindingTexts(Audit::Check(drawing)) ==
        std::vector<std::string>{"layer table missing", "dictionary ACAD_GROUP not the header's",
                                 "current text style 10 is no text style",
                                 "paper space of the block table is not the header's",
                                 "ByLayer of the linetype table is not the header's", "layer 0 not listed"});
  Audit::Repair(drawing);
  CHECK(drawing.Header.LayerControlObject != drawing.Header.Clayer);
  CHECK(View::Control(drawing, Tables::Table::Layer)->Base.Self == drawing.Header.LayerControlObject);
  CHECK(drawing.Header.DictionaryAcadGroup == Handle{.Value = 0xD});
  CHECK(drawing.Header.Textstyle == Handle{.Value = 0x11});
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/corners: every missing required record is made with the Model layout and its plot settings")
{
  Drawing drawing = Create();
  for (const std::string name : {"ByLayer", "Continuous"})
  {
    Erase(drawing, View::Record<Tables::LType>(drawing, name)->Record.Base.Self);
  }
  Erase(drawing, View::Record<Tables::DimStyle>(drawing, "Standard")->Record.Base.Self);
  Erase(drawing, View::Record<Tables::AppId>(drawing, "ACAD")->Record.Base.Self);
  Erase(drawing, View::Layouts(drawing)[0]->Base.Self);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(std::ranges::count(FindingTexts(checked), "application ACAD missing") == 1);
  CHECK(std::ranges::count(FindingTexts(checked), "layout of block *Model_Space missing") == 1);
  Audit::Repair(drawing);
  CHECK(View::Record<Tables::LType>(drawing, "ByLayer")->Record.Base.Self == drawing.Header.LTypeByLayer);
  CHECK(View::Record<Tables::LType>(drawing, "Continuous")->Description == "Solid line");
  CHECK(View::Record<Tables::DimStyle>(drawing, "Standard")->Dimtxsty == drawing.Header.Textstyle);
  CHECK(View::Record<Tables::AppId>(drawing, "ACAD") != nullptr);
  const std::vector<const Objects::Layout *> layouts = View::Layouts(drawing);
  REQUIRE(layouts.size() == 2);
  CHECK(layouts[0]->Name == "Model");
  CHECK(layouts[0]->Plot.Device == "none_device");
  CHECK(layouts[0]->Plot.Flags == 1712);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/corners: a reference to the wrong kind or to a missing STANDARD mline style is fixed")
{
  Drawing drawing = Create();
  Entities::Line line;
  line.Common.Layer = drawing.Header.Textstyle;
  const Handle drawn = *Edit::Add(drawing, line);
  Entities::MLine mline;
  mline.Style = Nowhere;
  const Handle lined = *Edit::Add(drawing, mline);
  Objects::Dictionary *styles = GetIf<Objects::Dictionary>(Find(drawing, drawing.Header.DictionaryAcadMLineStyle));
  styles->Entries.front().Object = drawing.Header.Clayer; // Standard names a layer now.
  Tables::BlockRecord &model = Model(drawing);
  model.Block = drawn;
  model.EndBlk = drawn;
  model.Layout = drawn;
  CHECK(FindingTexts(Audit::Check(drawing)) ==
        std::vector<std::string>{"layer 11 of LINE 2B missing", "multiline style 999 of MLINE 2C missing",
                                 "BLOCK of block *Model_Space missing", "ENDBLK of block *Model_Space missing",
                                 "layout of block *Model_Space missing"});
  Audit::Repair(drawing);
  CHECK(CommonOf(*Find(drawing, drawn))->Layer == drawing.Header.Clayer);
  const Handle made = Get<Entities::MLine>(*Find(drawing, lined)).Style;
  CHECK(Holds<Objects::MLineStyle>(*Find(drawing, made)));
  CHECK(BaseOf(*View::Entry(drawing, *View::Dictionary(drawing, "ACAD_MLINESTYLE"), "Standard")).Self == made);
  CHECK(Audit::Check(drawing).Findings.empty());

  Drawing bare = Create();
  Entities::MLine plain;
  plain.Style = Nowhere;
  REQUIRE(Edit::Add(bare, plain));
  GetIf<Objects::Dictionary>(Find(bare, bare.Header.DictionaryAcadMLineStyle))->Entries.clear();
  Audit::Repair(bare);
  CHECK(Holds<Objects::MLineStyle>(bare.Objects.back()));
  CHECK(Audit::Check(bare).Findings.empty());
}

TEST_CASE("audit/corners: a chain broken by a dead or foreign or wrong-kind link or a wrong Previous is rebuilt")
{
  for (const int way : {0, 1, 2, 3})
  {
    CAPTURE(way);
    Drawing drawing = Create();
    const Handle first = *Edit::Add(drawing, Entities::Line{});
    const Handle second = *Edit::Add(drawing, Entities::Line{});
    const Handle elsewhere = *Edit::Add(drawing, Entities::Line{}, Space::Paper);
    Common &link = *CommonOf(*Find(drawing, first));
    if (way == 0)
    {
      link.Next = Nowhere;
    }
    else if (way == 1)
    {
      link.Next = drawing.Header.Clayer;
    }
    else if (way == 2)
    {
      link.Next = elsewhere;
    }
    else
    {
      CommonOf(*Find(drawing, second))->Previous = Nowhere;
    }
    CHECK(FindingTexts(Audit::Check(drawing)) == std::vector<std::string>{"chain of block *Model_Space broken"});
    Audit::Repair(drawing);
    CHECK(View::Entities(drawing, Model(drawing)).size() == 2);
    CHECK(Audit::Check(drawing).Findings.empty());
  }
}

TEST_CASE("audit/corners: a dropped entity is unlinked from its chain whatever its neighbours and ends")
{
  Drawing drawing = Create();
  Entities::Insert alone;
  alone.Block = Nowhere;
  const Handle only = *Edit::Add(drawing, alone);
  CHECK(Model(drawing).First == only);
  Audit::Repair(drawing);
  CHECK(Model(drawing).First == Handle{});
  CHECK(Model(drawing).Last == Handle{});
  CHECK(Audit::Check(drawing).Findings.empty());

  Drawing odd = Create();
  REQUIRE(Edit::Add(odd, Entities::Line{}));
  Entities::Insert placed;
  placed.Block = Nowhere;
  const Handle middle = *Edit::Add(odd, placed);
  REQUIRE(Edit::Add(odd, Entities::Line{}));
  CommonOf(*Find(odd, middle))->Previous = odd.Header.Clayer;
  CommonOf(*Find(odd, middle))->Next = odd.Header.Textstyle;
  Audit::Repair(odd);
  CHECK(View::Entities(odd, Model(odd)).size() == 2);
  CHECK(Audit::Check(odd).Findings.empty());

  Drawing orphaned = Create();
  Entities::Insert stray;
  stray.Base.Self = Edit::MakeHandle(orphaned);
  stray.Base.Owner = Nowhere;
  stray.Block = Nowhere;
  orphaned.Objects.emplace_back(stray);
  Audit::Repair(orphaned);
  CHECK(Find(orphaned, stray.Base.Self) == nullptr);
  CHECK(Audit::Check(orphaned).Findings.empty());

  // A left-out member that heads its polyline's or its insert's sequence: the ends move off it.
  Drawing headed = Create();
  const Handle polyline = *Edit::Add(headed, Entities::Polyline{});
  const Handle wrong = AppendEntity<Entities::Attrib>(headed, polyline);
  Get<Entities::Attrib>(*Find(headed, wrong)).Lettering.Style = headed.Header.Textstyle;
  auto &drawn = Get<Entities::Polyline>(*Find(headed, polyline));
  drawn.First = wrong;
  drawn.Last = wrong;
  Entities::Insert tagged;
  tagged.Block = headed.Header.BlockRecordPaperSpace;
  tagged.Follows = true;
  const Handle insert = *Edit::Add(headed, tagged);
  const Handle vertex = AppendEntity<Entities::Vertex>(headed, insert);
  auto &inserted = Get<Entities::Insert>(*Find(headed, insert));
  inserted.First = vertex;
  inserted.Last = vertex;
  Audit::Repair(headed);
  CHECK(Get<Entities::Polyline>(*Find(headed, polyline)).First == Handle{});
  CHECK(Get<Entities::Insert>(*Find(headed, insert)).First == Handle{});
  CHECK(!Get<Entities::Insert>(*Find(headed, insert)).Follows);
  CHECK(Audit::Check(headed).Findings.empty());

  // A left-out member of an insert that does not head its sequence: the ends stay.
  Drawing kept = Create();
  Entities::Insert labelled;
  labelled.Block = kept.Header.BlockRecordPaperSpace;
  labelled.Follows = true;
  const Handle owner = *Edit::Add(kept, labelled);
  const Handle tag = AppendEntity<Entities::Attrib>(kept, owner);
  Get<Entities::Attrib>(*Find(kept, tag)).Lettering.Style = kept.Header.Textstyle;
  AppendEntity<Entities::Vertex>(kept, owner);
  auto &head = Get<Entities::Insert>(*Find(kept, owner));
  head.First = tag;
  head.Last = tag;
  Audit::Repair(kept);
  CHECK(Get<Entities::Insert>(*Find(kept, owner)).First == tag);
  CHECK(Audit::Check(kept).Findings.empty());
}

TEST_CASE("audit/corners: an entity owned by a non-block or by nothing moves to model space")
{
  Drawing drawing = Create();
  const Handle layered = AppendEntity<Entities::Line>(drawing, drawing.Header.Clayer);
  CHECK(FindingTexts(Audit::Check(drawing)) == std::vector<std::string>{"owner 10 of LINE 2B is no block record"});
  Audit::Repair(drawing);
  CHECK(Model(drawing).First == layered);
  CHECK(Model(drawing).Last == layered);
  CHECK(Audit::Check(drawing).Findings.empty());

  for (const Handle last : {Nowhere, Handle{.Value = 0x10}})
  {
    Drawing cut = Create();
    REQUIRE(Edit::Add(cut, Entities::Line{}));
    Model(cut).Last = last; // Nothing, or the layer 0: no entity either way.
    AppendEntity<Entities::Line>(cut, Nowhere);
    Audit::Repair(cut);
    CHECK(View::Entities(cut, Model(cut)).size() == 2);
    CHECK(Audit::Check(cut).Findings.empty());
  }

  Drawing members = Create();
  const Handle vertex = AppendEntity<Entities::Vertex>(members, members.Header.BlockRecordModelSpace);
  const Handle attrib = AppendEntity<Entities::Attrib>(members, members.Header.BlockRecordModelSpace);
  Get<Entities::Attrib>(*Find(members, attrib)).Lettering.Style = members.Header.Textstyle;
  const Handle seqend = AppendEntity<Entities::SeqEnd>(members, members.Header.BlockRecordModelSpace);
  const Handle lost = AppendEntity<Entities::Vertex>(members, Nowhere);
  CHECK(FindingTexts(Audit::Check(members)) == std::vector<std::string>{"VERTEX_2D 2B owned by no polyline, left out",
                                                                        "ATTRIB 2C owned by no insert, left out",
                                                                        "SEQEND 2D owned by no sequence, left out",
                                                                        "VERTEX_2D 2E owned by no polyline, left out"});
  Audit::Repair(members);
  for (const Handle removed : {vertex, attrib, seqend, lost})
  {
    CHECK(Find(members, removed) == nullptr);
  }
  CHECK(Audit::Check(members).Findings.empty());
}

TEST_CASE("audit/corners: nameless table defaults and a nameless shape style and a block named twice are fixed")
{
  Drawing drawing = Create();
  REQUIRE(Edit::Add(drawing, Tables::Ucs{}));
  Tables::Style shapes;
  shapes.Shape = true;
  shapes.Font = "ltypeshp.shx";
  REQUIRE(Edit::Add(drawing, shapes));
  Tables::Style unnamed; // Slipped in past Edit, which takes the nameless shape style for one of that name.
  unnamed.Record.Base.Self = Handle{.Value = 0x777};
  unnamed.Record.Base.Owner = drawing.Header.StyleControlObject;
  drawing.Objects.emplace_back(unnamed);
  Get<Tables::Control>(*Find(drawing, drawing.Header.StyleControlObject)).Entries.push_back(Handle{.Value = 0x777});
  const Handle chair = *Edit::AddBlock(drawing, "Chair");
  Entities::Insert placed;
  placed.Block = chair;
  REQUIRE(Edit::Add(drawing, placed));
  Tables::BlockRecord twin = *View::Block(drawing, "Chair");
  twin.Record.Base.Self = Edit::MakeHandle(drawing);
  twin.Record.Name = "chair";
  drawing.Objects.emplace_back(twin);
  Get<Tables::Control>(*Find(drawing, drawing.Header.BlockControlObject)).Entries.push_back(twin.Record.Base.Self);
  Entities::Insert again;
  again.Block = twin.Record.Base.Self;
  const Handle seated = *Edit::Add(drawing, again);
  CHECK(FindingTexts(Audit::Check(drawing)) ==
        std::vector<std::string>{"block chair repeats Chair", "text style 777 has no name", "ucs 2B has no name"});
  Audit::Repair(drawing);
  CHECK(Get<Entities::Insert>(*Find(drawing, seated)).Block == chair);
  CHECK(Find(drawing, twin.Record.Base.Self) == nullptr);
  CHECK(View::Records<Tables::Ucs>(drawing).empty());
  CHECK(View::Records<Tables::Style>(drawing).size() == 2);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/corners: a negative lineweight or a repeated class or foreign xdata is fixed")
{
  Drawing drawing = Create();
  Entities::Line thin;
  thin.Base.Extended.push_back(Xdata{.Application = "", .AppId = drawing.Header.Clayer, .Tags = {}});
  const Handle drawn = *Edit::Add(drawing, thin);
  CommonOf(*Find(drawing, drawn))->Lineweight = static_cast<Lineweight>(27); // A code AutoCAD does not define.
  for (const std::string name : {"1:1", "1:2"})
  {
    Objects::Scale scale;
    scale.Base.Self = Edit::MakeHandle(drawing);
    scale.Base.Owner = drawing.Header.DictionaryNamedObjects;
    scale.Name = name;
    drawing.Objects.emplace_back(scale);
    Unknown widget;
    widget.Type = 500;
    widget.Class = Symbol{"ACME_WIDGET"};
    widget.Base.Self = Edit::MakeHandle(drawing);
    widget.Base.Owner = drawing.Header.DictionaryNamedObjects;
    drawing.Objects.emplace_back(widget);
  }
  CHECK(FindingTexts(Audit::Check(drawing)) ==
        std::vector<std::string>{"class SCALE not declared", "class ACME_WIDGET not declared",
                                 "application 10 of the extended data of LINE 2B missing",
                                 "lineweight 27 of LINE 2B unknown"});
  const Audit::Report repaired = Audit::Repair(drawing);
  CHECK(!repaired.IsClean());
  CHECK(CommonOf(*Find(drawing, drawn))->Lineweight == Lineweight::ByLayer);
  CHECK(BaseOf(*Find(drawing, drawn)).Extended.empty());
  CHECK(drawing.Classes.size() == 4);
  CHECK(FindingTexts(Audit::Check(drawing)) == std::vector<std::string>{"class ACME_WIDGET not declared"});
}
} // namespace DwgIO
