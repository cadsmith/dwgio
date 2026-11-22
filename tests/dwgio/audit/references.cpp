/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/references.cpp
 *  @brief     The audit of references to layers, linetypes, styles and blocks the drawing lacks.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/findings.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/dimension.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/mline.hpp"
#include "dwgio/entities/text.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
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
constexpr Handle Nowhere{.Value = 0x999};

auto AllErrors(const Audit::Report &report) -> bool
{
  return std::ranges::all_of(report.Findings, [](const Audit::Finding &finding) -> bool
                             { return finding.Level == Audit::Severity::Error; });
}
} // namespace

TEST_CASE("audit/references: a reference to a missing layer or linetype or style falls back")
{
  Drawing drawing = Create();
  Entities::Line line;
  line.Common.Layer = Nowhere;
  line.Common.LinetypeOrigin = Origin::Record;
  line.Common.Linetype = Nowhere;
  line.Common.PlotstyleOrigin = Origin::Record;
  line.Common.Plotstyle = Nowhere;
  const Handle drawn = *Edit::Add(drawing, line);
  Entities::Text text;
  text.Lettering.Style = Nowhere;
  const Handle lettered = *Edit::Add(drawing, text);
  Entities::Dimension dimension;
  dimension.Style = Nowhere;
  const Handle measured = *Edit::Add(drawing, dimension);
  Entities::MLine mline;
  mline.Style = Nowhere;
  const Handle lined = *Edit::Add(drawing, mline);
  Tables::Layer walls;
  walls.Record.Name = "Walls";
  walls.Linetype = Nowhere;
  walls.Style = Nowhere;
  const Handle layered = *Edit::Add(drawing, walls);
  Tables::DimStyle style;
  style.Record.Name = "Arrows";
  style.Dimtxsty = Nowhere;
  const Handle styled = *Edit::Add(drawing, style);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) ==
        std::vector<std::string>{"layer 999 of LINE 2B missing", "linetype 999 of LINE 2B missing",
                                 "plot style 999 of LINE 2B missing", "text style 999 of TEXT 2C missing",
                                 "dimension style 999 of DIMENSION_ORDINATE 2D missing",
                                 "multiline style 999 of MLINE 2E missing", "linetype 999 of LAYER 2F missing",
                                 "plot style 999 of LAYER 2F missing", "text style 999 of DIMSTYLE 30 missing"});
  CHECK(AllErrors(checked));
  CHECK(checked.Findings[0].Subject == drawn);
  CHECK(Audit::Repair(drawing).IsClean());
  const Common &common = *CommonOf(*Find(drawing, drawn));
  CHECK(common.Layer == drawing.Header.Clayer);
  CHECK(common.LinetypeOrigin == Origin::ByLayer);
  CHECK(common.Linetype == Handle{});
  CHECK(common.PlotstyleOrigin == Origin::ByLayer);
  CHECK(common.Plotstyle == Handle{});
  CHECK(Get<Entities::Text>(*Find(drawing, lettered)).Lettering.Style == drawing.Header.Textstyle);
  CHECK(Get<Entities::Dimension>(*Find(drawing, measured)).Style ==
        View::Record<Tables::DimStyle>(drawing, "Standard")->Record.Base.Self);
  CHECK(Get<Entities::MLine>(*Find(drawing, lined)).Style == drawing.Header.Cmlstyle);
  CHECK(Get<Tables::Layer>(*Find(drawing, layered)).Linetype == drawing.Header.LTypeContinuous);
  CHECK(Get<Tables::Layer>(*Find(drawing, layered)).Style == Handle{});
  CHECK(Get<Tables::DimStyle>(*Find(drawing, styled)).Dimtxsty == drawing.Header.Textstyle);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/references: an insert of a missing block is dropped and a block gets its BLOCK and ENDBLK")
{
  Drawing drawing = Create();
  const Handle first = *Edit::Add(drawing, Entities::Line{});
  Entities::Insert insert;
  insert.Block = Nowhere;
  const Handle placed = *Edit::Add(drawing, insert);
  const Handle last = *Edit::Add(drawing, Entities::Line{});
  const Handle chair = *Edit::AddBlock(drawing, "Chair");
  Tables::BlockRecord *record = GetIf<Tables::BlockRecord>(Find(drawing, chair));
  const Handle end = record->EndBlk;
  record->EndBlk = Nowhere;
  std::erase_if(drawing.Objects, [end](const Object &object) -> bool { return BaseOf(object).Self == end; });
  Objects::Layout *layout = GetIf<Objects::Layout>(Find(drawing, View::Layouts(drawing)[1]->Base.Self));
  const Handle orphan = layout->Base.Self;
  layout->Block = Nowhere;
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) ==
        std::vector<std::string>{"block 999 of LAYOUT 26 missing, left out", "block 999 of INSERT 2C missing, left out",
                                 "layout of block *Paper_Space missing", "ENDBLK of block Chair missing"});
  CHECK(checked.Errors() == 1);
  CHECK(checked.Findings[1].Level == Audit::Severity::Error);
  CHECK(checked.Warnings() == 3);
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(Find(drawing, placed) == nullptr);
  CHECK(Find(drawing, orphan) == nullptr);
  const std::vector<const Object *> entities = View::Entities(drawing, *View::Block(drawing, "*Model_Space"));
  REQUIRE(entities.size() == 2);
  CHECK(BaseOf(*entities[0]).Self == first);
  CHECK(BaseOf(*entities[1]).Self == last);
  record = GetIf<Tables::BlockRecord>(Find(drawing, chair));
  CHECK(Holds<Entities::EndBlk>(*Find(drawing, record->EndBlk)));
  CHECK(BaseOf(*Find(drawing, record->EndBlk)).Owner == chair);
  const std::vector<const Objects::Layout *> layouts = View::Layouts(drawing);
  REQUIRE(layouts.size() == 2);
  CHECK(layouts[1]->Name == "Layout1");
  CHECK(layouts[1]->Block == drawing.Header.BlockRecordPaperSpace);
  CHECK(View::Block(drawing, "*Paper_Space")->Layout == layouts[1]->Base.Self);
  CHECK(Audit::Check(drawing).Findings.empty());

  Drawing table = Create();
  const Handle desk = *Edit::AddBlock(table, "Desk");
  record = GetIf<Tables::BlockRecord>(Find(table, desk));
  record->Layout = Nowhere;
  const Handle block = record->Block;
  record->Block = {};
  std::erase_if(table.Objects, [block](const Object &object) -> bool { return BaseOf(object).Self == block; });
  CHECK(FindingTexts(Audit::Check(table)) ==
        std::vector<std::string>{"BLOCK of block Desk missing", "layout 999 of block Desk is no layout"});
  Audit::Repair(table);
  record = GetIf<Tables::BlockRecord>(Find(table, desk));
  CHECK(Get<Entities::Block>(*Find(table, record->Block)).Name == "Desk");
  CHECK(record->Layout == Handle{});
  CHECK(Audit::Check(table).Findings.empty());
}

TEST_CASE("audit/references: findings on the broken-reference drawings match AutoCAD's AUDIT" *
          doctest::skip(!IsVerifying()))
{
  Drawing layer = Create();
  Entities::Line line;
  line.Common.Layer = Nowhere;
  REQUIRE(Edit::Add(layer, line));
  CHECK(!PassesAudit(Save(layer).value().Bytes, "dwgio-audit-layer.dwg"));
  Audit::Repair(layer);
  CHECK(PassesAudit(Save(layer).value().Bytes, "dwgio-audit-layer-repaired.dwg"));
  Drawing linetype = Create();
  Entities::Line dashed;
  dashed.Common.LinetypeOrigin = Origin::Record;
  dashed.Common.Linetype = Nowhere;
  REQUIRE(Edit::Add(linetype, dashed));
  CHECK(!PassesAudit(Save(linetype).value().Bytes, "dwgio-audit-linetype.dwg"));
  Drawing style = Create();
  Entities::Text text;
  text.Lettering.Style = Nowhere;
  REQUIRE(Edit::Add(style, text));
  CHECK(!PassesAudit(Save(style).value().Bytes, "dwgio-audit-style.dwg"));
  Drawing insert = Create();
  Entities::Insert placed;
  placed.Block = Nowhere;
  REQUIRE(Edit::Add(insert, placed));
  CHECK(!PassesAudit(Save(insert).value().Bytes, "dwgio-audit-insert.dwg"));
  Drawing framing = Create();
  const Handle chair = *Edit::AddBlock(framing, "Chair");
  Tables::BlockRecord *record = GetIf<Tables::BlockRecord>(Find(framing, chair));
  const Handle end = record->EndBlk;
  std::erase_if(framing.Objects, [end](const Object &object) -> bool { return BaseOf(object).Self == end; });
  CHECK(PassesAudit(Save(framing).value().Bytes, "dwgio-audit-framing.dwg"));
  Drawing layout = Create();
  GetIf<Objects::Layout>(Find(layout, View::Layouts(layout)[1]->Base.Self))->Block = Nowhere;
  CHECK(PassesAudit(Save(layout).value().Bytes, "dwgio-audit-layout.dwg"));
}
} // namespace DwgIO
