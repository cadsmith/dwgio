/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/names.cpp
 *  @brief     The audit of table record names: duplicates in any case and records with none.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/findings.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <expected>
#include <string>
#include <vector>

namespace DwgIO
{
namespace
{
auto LayerNamed(Drawing &drawing, std::string name) -> Handle
{
  Tables::Layer layer;
  layer.Record.Name = std::move(name);
  layer.Linetype = drawing.Header.LTypeContinuous;
  return *Edit::Add(drawing, layer);
}

auto LineOn(Drawing &drawing, Handle layer) -> Handle
{
  Entities::Line line;
  line.Common.Layer = layer;
  return *Edit::Add(drawing, line);
}
} // namespace

TEST_CASE("audit/names: a record whose name another record of its table has in any case is dropped")
{
  Drawing drawing = Create();
  const Handle walls = LayerNamed(drawing, "Walls");
  const Handle line = LineOn(drawing, walls);
  Tables::Layer twin;
  twin.Record.Name = "walls";
  twin.Linetype = drawing.Header.LTypeContinuous;
  const Handle again = *Edit::Add(
      drawing,
      Tables::LType{
          .Record = {
              .Base = {}, .Name = "Dashed", .Referenced = true, .XrefIndex = -1, .Dependent = false, .Source = {}}});
  twin.Record.Base.Self = Handle{.Value = 0x999};
  twin.Record.Base.Owner = drawing.Header.LayerControlObject;
  drawing.Objects.emplace_back(twin);
  Get<Tables::Control>(*Find(drawing, drawing.Header.LayerControlObject)).Entries.push_back(twin.Record.Base.Self);
  const Handle moved = LineOn(drawing, twin.Record.Base.Self);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) == std::vector<std::string>{"layer walls repeats Walls"});
  CHECK(checked.Findings[0].Level == Audit::Severity::Error);
  CHECK(checked.Findings[0].Subject == Handle{.Value = 0x999});
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(Find(drawing, Handle{.Value = 0x999}) == nullptr);
  CHECK(View::Records<Tables::Layer>(drawing).size() == 2);
  CHECK(CommonOf(*Find(drawing, moved))->Layer == walls);
  CHECK(CommonOf(*Find(drawing, line))->Layer == walls);
  CHECK(View::Record<Tables::LType>(drawing, "dashed")->Record.Base.Self == again);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/names: a nameless record is dropped and its references move to the table's default")
{
  Drawing drawing = Create();
  const Handle nameless = LayerNamed(drawing, "");
  const Handle line = LineOn(drawing, nameless);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) == std::vector<std::string>{"layer 2B has no name"});
  CHECK(checked.Findings[0].Level == Audit::Severity::Error);
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(Find(drawing, nameless) == nullptr);
  CHECK(View::Control(drawing, Tables::Table::Layer)->Entries == std::vector<Handle>{drawing.Header.Clayer});
  CHECK(CommonOf(*Find(drawing, line))->Layer == drawing.Header.Clayer);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/names: findings on the broken-name drawings match AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  Drawing twins = Create();
  LineOn(twins, LayerNamed(twins, "Walls"));
  Tables::Layer twin;
  twin.Record.Name = "walls";
  twin.Record.Base.Self = Handle{.Value = 0x999};
  twin.Record.Base.Owner = twins.Header.LayerControlObject;
  twin.Linetype = twins.Header.LTypeContinuous;
  twins.Objects.emplace_back(twin);
  Get<Tables::Control>(*Find(twins, twins.Header.LayerControlObject)).Entries.push_back(twin.Record.Base.Self);
  CHECK(!PassesAudit(Save(twins).value().Bytes, "dwgio-audit-twins.dwg"));
  Audit::Repair(twins);
  CHECK(PassesAudit(Save(twins).value().Bytes, "dwgio-audit-twins-repaired.dwg"));
  Drawing nameless = Create();
  LineOn(nameless, LayerNamed(nameless, ""));
  CHECK(!PassesAudit(Save(nameless).value().Bytes, "dwgio-audit-nameless.dwg"));
}
} // namespace DwgIO
