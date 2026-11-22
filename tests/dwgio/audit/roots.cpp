/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/roots.cpp
 *  @brief     The audit of the drawing's roots: missing tables, spaces, required records and dictionaries.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/findings.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
void Erase(Drawing &drawing, Handle self)
{
  std::erase_if(drawing.Objects, [self](const Object &object) -> bool { return BaseOf(object).Self == self; });
}

auto AllErrors(const Audit::Report &report) -> bool
{
  return std::ranges::all_of(report.Findings, [](const Audit::Finding &finding) -> bool
                             { return finding.Level == Audit::Severity::Error; });
}
} // namespace

TEST_CASE("audit/roots: a missing table is made and named by the header with its records listed again")
{
  Drawing drawing = Create();
  Erase(drawing, drawing.Header.LayerControlObject);
  drawing.Header.LayerControlObject = {};
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) == std::vector<std::string>{"layer table missing", "layer 0 not listed"});
  CHECK(AllErrors(checked));
  CHECK(View::Control(drawing, Tables::Table::Layer) == nullptr);
  CHECK(Audit::Repair(drawing).IsClean());
  const Tables::Control *layers = View::Control(drawing, Tables::Table::Layer);
  REQUIRE(layers);
  CHECK(layers->Base.Self == drawing.Header.LayerControlObject);
  CHECK(layers->Base.Self == Handle{.Value = 0x2B});
  CHECK(layers->Entries == std::vector<Handle>{drawing.Header.Clayer});
  CHECK(View::Record<Tables::Layer>(drawing, "0")->Record.Base.Owner == layers->Base.Self);
  CHECK(Audit::Check(drawing).Findings.empty());

  Drawing unnamed = Create();
  unnamed.Header.StyleControlObject = {};
  CHECK(FindingTexts(Audit::Check(unnamed)) == std::vector<std::string>{"text style table not the header's"});
  Audit::Repair(unnamed);
  CHECK(unnamed.Header.StyleControlObject == View::Control(unnamed, Tables::Table::Style)->Base.Self);
}

TEST_CASE("audit/roots: a missing space is remade under the header's handle and keeps what it owned")
{
  Drawing drawing = Create();
  const std::expected<Handle, Failure> first = Edit::Add(drawing, Entities::Line{});
  const std::expected<Handle, Failure> second = Edit::Add(drawing, Entities::Line{});
  REQUIRE((first && second));
  const Handle model = drawing.Header.BlockRecordModelSpace;
  Erase(drawing, model);
  CHECK(FindingTexts(Audit::Check(drawing)) == std::vector<std::string>{"block *Model_Space missing"});
  CHECK(Audit::Repair(drawing).IsClean());
  const Tables::BlockRecord *record = View::Block(drawing, "*Model_Space");
  REQUIRE(record);
  CHECK(record->Record.Base.Self == model);
  CHECK(record->Record.Base.Owner == drawing.Header.BlockControlObject);
  CHECK(record->Block == Handle{.Value = 0x20});
  CHECK(record->EndBlk == Handle{.Value = 0x21});
  CHECK(record->Layout == Handle{.Value = 0x22});
  CHECK(View::Control(drawing, Tables::Table::Block)->ModelSpace == model);
  CHECK(BaseOf(*Find(drawing, *first)).Owner == model);
  CHECK(View::Entities(drawing, *record).size() == 2);
  CHECK(Audit::Check(drawing).Findings.empty());

  // Paper space erased with its BLOCK, ENDBLK and layout: all four made again, the layout entered.
  Drawing paper = Create();
  const Tables::BlockRecord *paperSpace = View::Block(paper, "*Paper_Space");
  const Handle self = paperSpace->Record.Base.Self;
  for (const Handle handle : {paperSpace->Block, paperSpace->EndBlk, paperSpace->Layout, self})
  {
    Erase(paper, handle);
  }
  Objects::Dictionary *layouts = GetIf<Objects::Dictionary>(Find(paper, paper.Header.DictionaryLayouts));
  std::erase_if(layouts->Entries, [](const Objects::Entry &entry) -> bool { return entry.Name == "Layout1"; });
  CHECK(FindingTexts(Audit::Check(paper)) == std::vector<std::string>{"block *Paper_Space missing"});
  CHECK(Audit::Repair(paper).IsClean());
  const Tables::BlockRecord *made = View::Block(paper, "*Paper_Space");
  REQUIRE(made);
  CHECK(made->Record.Base.Self == self);
  CHECK(Holds<Entities::Block>(*Find(paper, made->Block)));
  CHECK(Holds<Entities::EndBlk>(*Find(paper, made->EndBlk)));
  const std::vector<const Objects::Layout *> tabs = View::Layouts(paper);
  REQUIRE(tabs.size() == 2);
  CHECK(tabs[1]->Name == "Layout1");
  CHECK(tabs[1]->Block == self);
  CHECK(tabs[1]->Base.Self == made->Layout);
  CHECK(Audit::Check(paper).Findings.empty());
}

TEST_CASE("audit/roots: a missing required record is made as Create makes it and the header names it")
{
  Drawing drawing = Create();
  Erase(drawing, drawing.Header.Clayer);
  CHECK(FindingTexts(Audit::Check(drawing)) ==
        std::vector<std::string>{"entry 10 of the layer table is no layer", "layer 0 missing"});
  CHECK(Audit::Repair(drawing).IsClean());
  const Tables::Layer *zero = View::Record<Tables::Layer>(drawing, "0");
  REQUIRE(zero);
  CHECK(zero->Record.Base.Self == Handle{.Value = 0x10});
  CHECK(zero->Linetype == drawing.Header.LTypeContinuous);
  CHECK(drawing.Header.Clayer == zero->Record.Base.Self);
  CHECK(View::Control(drawing, Tables::Table::Layer)->Entries == std::vector<Handle>{zero->Record.Base.Self});
  CHECK(Audit::Check(drawing).Findings.empty());

  Drawing styles = Create();
  Erase(styles, styles.Header.Textstyle);
  Erase(styles, styles.Header.LTypeByBlock);
  Audit::Repair(styles);
  CHECK(View::Record<Tables::Style>(styles, "Standard")->Font == "Arial.ttf");
  CHECK(View::Record<Tables::LType>(styles, "ByBlock")->Record.Base.Self == styles.Header.LTypeByBlock);
  CHECK(View::Control(styles, Tables::Table::LType)->ByBlock == styles.Header.LTypeByBlock);
  CHECK(Audit::Check(styles).Findings.empty());

  Drawing roots = Create();
  roots.Header.Clayer = Handle{.Value = 0x999};
  roots.Header.Celtype = {};
  roots.Header.Textstyle = roots.Header.Clayer;
  CHECK(FindingTexts(Audit::Check(roots)) == std::vector<std::string>{"current layer 999 is no layer",
                                                                      "current text style 999 is no text style",
                                                                      "current linetype 0 is no linetype"});
  Audit::Repair(roots);
  CHECK(roots.Header.Clayer == Handle{.Value = 0x10});
  CHECK(roots.Header.Textstyle == Handle{.Value = 0x11});
  CHECK(roots.Header.Celtype == roots.Header.LTypeByLayer);
}

TEST_CASE("audit/roots: a dictionary the root lacks is entered or made and a dangling entry dropped")
{
  Drawing drawing = Create();
  Objects::Dictionary *root = GetIf<Objects::Dictionary>(Find(drawing, drawing.Header.DictionaryNamedObjects));
  REQUIRE(root);
  std::erase_if(root->Entries, [](const Objects::Entry &entry) -> bool { return entry.Name == "ACAD_LAYOUT"; });
  root->Entries.push_back(Objects::Entry{.Name = "Ghost", .Object = Handle{.Value = 0x999}});
  const Audit::Report dangling = Audit::Check(drawing);
  CHECK(FindingTexts(dangling) ==
        std::vector<std::string>{"dictionary ACAD_LAYOUT missing", "entry Ghost of dictionary C names nothing"});
  CHECK(dangling.Warnings() == 2);
  CHECK(dangling.IsClean());
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(View::Dictionary(drawing, "ACAD_LAYOUT")->Base.Self == drawing.Header.DictionaryLayouts);
  CHECK(View::Layouts(drawing).size() == 2);
  CHECK(View::Entry(drawing, *View::Root(drawing), "Ghost") == nullptr);
  CHECK(Audit::Check(drawing).Findings.empty());

  Drawing damaged = Create();
  Erase(damaged, damaged.Header.DictionaryAcadGroup);
  Objects::Dictionary *styles = GetIf<Objects::Dictionary>(Find(damaged, damaged.Header.DictionaryPlotStyles));
  styles->Default = Handle{.Value = 0x999};
  CHECK(FindingTexts(Audit::Check(damaged)) ==
        std::vector<std::string>{"dictionary ACAD_GROUP missing", "default of dictionary E names nothing"});
  Audit::Repair(damaged);
  CHECK(View::Dictionary(damaged, "ACAD_GROUP")->Base.Self == damaged.Header.DictionaryAcadGroup);
  CHECK(damaged.Header.DictionaryAcadGroup == Handle{.Value = 0xD});
  CHECK(!View::Dictionary(damaged, "ACAD_PLOTSTYLENAME")->Default);
  CHECK(Audit::Check(damaged).Findings.empty());

  Drawing rootless = Create();
  Erase(rootless, rootless.Header.DictionaryNamedObjects);
  const Audit::Report checked = Audit::Check(rootless);
  REQUIRE(checked.Findings.size() == 6);
  CHECK(checked.Findings[0].What == "dictionary of named objects missing");
  CHECK(checked.Findings[0].Level == Audit::Severity::Error);
  CHECK(checked.Errors() == 1);
  CHECK(checked.Findings[1].What == "dictionary ACAD_GROUP missing");
  Audit::Repair(rootless);
  CHECK(View::Root(rootless)->Base.Self == Handle{.Value = 0xC});
  CHECK(View::Root(rootless)->Entries.size() == 5);
  CHECK(Audit::Check(rootless).Findings.empty());

  // Any object may sit under any key: a layout under ACAD_GROUP is no finding.
  Drawing odd = Create();
  Objects::Dictionary *groups = GetIf<Objects::Dictionary>(Find(odd, odd.Header.DictionaryAcadGroup));
  groups->Entries.push_back(Objects::Entry{.Name = "Model", .Object = View::Layouts(odd)[0]->Base.Self});
  CHECK(Audit::Check(odd).Findings.empty());
}

TEST_CASE("audit/roots: each record is listed once in its table and non-records are dropped")
{
  Drawing drawing = Create();
  Tables::Layer walls;
  walls.Record.Base.Self = Handle{.Value = 0x2B};
  walls.Record.Name = "Walls";
  walls.Linetype = drawing.Header.LTypeContinuous;
  drawing.Objects.emplace_back(walls);
  const std::expected<Handle, Failure> line = Edit::Add(drawing, Entities::Line{});
  REQUIRE(line);
  Tables::Control *layers = GetIf<Tables::Control>(Find(drawing, drawing.Header.LayerControlObject));
  layers->Entries.push_back(*line);
  Tables::Control *blocks = GetIf<Tables::Control>(Find(drawing, drawing.Header.BlockControlObject));
  blocks->ModelSpace = {};
  Tables::Control *linetypes = GetIf<Tables::Control>(Find(drawing, drawing.Header.LTypeControlObject));
  linetypes->ByBlock = linetypes->ByLayer;
  CHECK(FindingTexts(Audit::Check(drawing)) ==
        std::vector<std::string>{"entry 2C of the layer table is no layer",
                                 "model space of the block table is not the header's",
                                 "ByBlock of the linetype table is not the header's", "layer Walls not listed"});
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(View::Control(drawing, Tables::Table::Layer)->Entries ==
        std::vector<Handle>{drawing.Header.Clayer, Handle{.Value = 0x2B}});
  CHECK(View::Record<Tables::Layer>(drawing, "walls")->Record.Base.Owner == drawing.Header.LayerControlObject);
  CHECK(View::Control(drawing, Tables::Table::Block)->ModelSpace == drawing.Header.BlockRecordModelSpace);
  CHECK(View::Control(drawing, Tables::Table::LType)->ByBlock == drawing.Header.LTypeByBlock);
  CHECK(Audit::Check(drawing).Findings.empty());

  Drawing owned = Create();
  BaseOf(*Find(owned, owned.Header.Clayer)).Owner = {};
  CHECK(FindingTexts(Audit::Check(owned)) == std::vector<std::string>{"owner of layer 0 is not its table"});
  Audit::Repair(owned);
  CHECK(BaseOf(*Find(owned, owned.Header.Clayer)).Owner == owned.Header.LayerControlObject);
}

TEST_CASE("audit/roots: findings on the broken-table drawings match AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  Drawing table = Create();
  Erase(table, table.Header.LayerControlObject);
  CHECK(!PassesAudit(Save(table).value().Bytes, "dwgio-audit-table.dwg"));
  Audit::Repair(table);
  CHECK(PassesAudit(Save(table).value().Bytes, "dwgio-audit-table-repaired.dwg"));
  Drawing space = Create();
  REQUIRE(Edit::Add(space, Entities::Line{}));
  Erase(space, space.Header.BlockRecordModelSpace);
  CHECK(!PassesAudit(Save(space).value().Bytes, "dwgio-audit-space.dwg"));
  Audit::Repair(space);
  CHECK(PassesAudit(Save(space).value().Bytes, "dwgio-audit-space-repaired.dwg"));
  Drawing zero = Create();
  Erase(zero, zero.Header.Clayer);
  CHECK(!PassesAudit(Save(zero).value().Bytes, "dwgio-audit-zero.dwg"));
  Drawing layouts = Create();
  Objects::Dictionary *root = GetIf<Objects::Dictionary>(Find(layouts, layouts.Header.DictionaryNamedObjects));
  std::erase_if(root->Entries, [](const Objects::Entry &entry) -> bool { return entry.Name == "ACAD_LAYOUT"; });
  CHECK(PassesAudit(Save(layouts).value().Bytes, "dwgio-audit-layouts.dwg"));
  Drawing rootless = Create();
  Erase(rootless, rootless.Header.DictionaryNamedObjects);
  CHECK(!PassesAudit(Save(rootless).value().Bytes, "dwgio-audit-rootless.dwg"));
  Drawing ghost = Create();
  GetIf<Objects::Dictionary>(Find(ghost, ghost.Header.DictionaryNamedObjects))
      ->Entries.push_back(Objects::Entry{.Name = "Ghost", .Object = Handle{.Value = 0x999}});
  CHECK(PassesAudit(Save(ghost).value().Bytes, "dwgio-audit-ghost.dwg"));
  Drawing unlisted = Create();
  Tables::Layer walls;
  walls.Record.Base.Self = Handle{.Value = 0x2B};
  walls.Record.Base.Owner = unlisted.Header.LayerControlObject;
  walls.Record.Name = "Walls";
  walls.Linetype = unlisted.Header.LTypeContinuous;
  unlisted.Objects.emplace_back(walls);
  CHECK(!PassesAudit(Save(unlisted).value().Bytes, "dwgio-audit-unlisted.dwg"));
}
} // namespace DwgIO
