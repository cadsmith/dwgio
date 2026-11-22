/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/review.cpp
 *  @brief     Inputs to the audit that the other audit tests do not try.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/findings.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/circle.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/vertex.hpp"
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
#include "dwgio/tables/layer.hpp"
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
void Erase(Drawing &drawing, Handle self)
{
  std::erase_if(drawing.Objects, [self](const Object &object) -> bool { return BaseOf(object).Self == self; });
}

void RemoveEntry(Drawing &drawing, const std::string &name)
{
  Objects::Dictionary *root = GetIf<Objects::Dictionary>(Find(drawing, drawing.Header.DictionaryNamedObjects));
  std::erase_if(root->Entries, [&name](const Objects::Entry &entry) -> bool { return entry.Name == name; });
}

} // namespace

TEST_CASE("audit/review: a drawing before R2000 without the R2000 dictionaries is repaired without them")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("empty-r14.dwg"));
  REQUIRE(loaded);
  Drawing drawing = loaded->Drawing;
  Erase(drawing, View::Layouts(drawing)[0]->Base.Self); // An R14 record keeps no layout handle; the layout does.
  RemoveEntry(drawing, "ACAD_PLOTSTYLENAME");
  RemoveEntry(drawing, "ACAD_LAYOUT");
  Erase(drawing, drawing.Header.Clayer);
  Erase(drawing, drawing.Header.BlockRecordModelSpace);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(std::ranges::count(FindingTexts(checked), "layer 0 missing") == 1);
  CHECK(std::ranges::count(FindingTexts(checked), "block *Model_Space missing") == 1);
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(View::Record<Tables::Layer>(drawing, "0")->Style == Handle{});
  CHECK(View::Block(drawing, "*MODEL_SPACE")->Layout == Handle{});
  CHECK(View::Dictionary(drawing, "ACAD_LAYOUT") == nullptr);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/review: a handle above the seed in the header moves the seed past it")
{
  Drawing drawing = Create();
  Erase(drawing, drawing.Header.BlockRecordModelSpace);
  drawing.Header.BlockRecordModelSpace = Handle{.Value = 0xFFFFF};
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(View::Block(drawing, "*Model_Space")->Record.Base.Self == Handle{.Value = 0xFFFFF});
  CHECK(drawing.Header.Handseed.Value > 0xFFFFF);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/review: the header and the controls follow a record dropped for its name")
{
  Drawing drawing = Create();
  Tables::Layer walls;
  walls.Record.Name = "Walls";
  walls.Linetype = drawing.Header.LTypeContinuous;
  const Handle first = *Edit::Add(drawing, walls);
  Tables::Layer twin = walls;
  twin.Record.Name = "walls";
  twin.Record.Base.Self = Handle{.Value = 0x999};
  twin.Record.Base.Owner = drawing.Header.LayerControlObject;
  drawing.Objects.emplace_back(twin);
  Get<Tables::Control>(*Find(drawing, drawing.Header.LayerControlObject)).Entries.push_back(Handle{.Value = 0x999});
  drawing.Header.Clayer = Handle{.Value = 0x999};
  Tables::AppId acad;
  acad.Record.Name = "acad";
  acad.Record.Base.Self = Handle{.Value = 0x998};
  acad.Record.Base.Owner = drawing.Header.AppIdControlObject;
  drawing.Objects.emplace_back(acad);
  Get<Tables::Control>(*Find(drawing, drawing.Header.AppIdControlObject)).Entries.push_back(Handle{.Value = 0x998});
  Entities::Line line;
  line.Base.Extended.push_back(Xdata{.Application = "", .AppId = Handle{.Value = 0x998}, .Tags = {}});
  const Handle drawn = *Edit::Add(drawing, line);
  CHECK(FindingTexts(Audit::Check(drawing)) ==
        std::vector<std::string>{"layer walls repeats Walls", "application acad repeats ACAD"});
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(drawing.Header.Clayer == first);
  const std::vector<Xdata> &extended = BaseOf(*Find(drawing, drawn)).Extended;
  REQUIRE(extended.size() == 1);
  CHECK(extended[0].AppId == View::Record<Tables::AppId>(drawing, "ACAD")->Record.Base.Self);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/review: a sequence whose head names a SEQEND other than its own is reported")
{
  Drawing drawing = Create();
  const Handle polyline = *Edit::Add(drawing, Entities::Polyline{});
  AppendEntity<Entities::Vertex>(drawing, polyline);
  Audit::Repair(drawing);
  const Drawing sound = drawing;
  auto &head = Get<Entities::Polyline>(*Find(drawing, polyline));
  const Handle own = head.SeqEnd;
  head.SeqEnd = Handle{.Value = 0x777};
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) == std::vector<std::string>{"SEQEND of POLYLINE_2D 2B is not its own"});
  CHECK(checked.Warnings() == 1);
  Audit::Repair(drawing);
  CHECK(Get<Entities::Polyline>(*Find(drawing, polyline)).SeqEnd == own);
  CHECK(drawing == sound);
  CHECK(Audit::Repair(drawing).Findings.empty());
  CHECK(drawing == sound);
}

TEST_CASE("audit/review: a record listed twice in its table is listed once")
{
  Drawing drawing = Create();
  Get<Tables::Control>(*Find(drawing, drawing.Header.LayerControlObject)).Entries.push_back(drawing.Header.Clayer);
  CHECK(FindingTexts(Audit::Check(drawing)) == std::vector<std::string>{"layer 0 listed twice"});
  Audit::Repair(drawing);
  CHECK(View::Control(drawing, Tables::Table::Layer)->Entries == std::vector<Handle>{drawing.Header.Clayer});
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/review: the seed is reported before the handles it would have hidden")
{
  Drawing drawing = Create();
  REQUIRE(Edit::Add(drawing, Entities::Line{}));
  Entities::Circle circle;
  circle.Base.Self = Handle{.Value = 0x2B};
  circle.Base.Owner = drawing.Header.BlockRecordModelSpace;
  circle.Common.Layer = drawing.Header.Clayer;
  circle.Common.Space = Space::Model;
  drawing.Objects.emplace_back(circle);
  drawing.Header.Handseed = Handle{.Value = 5};
  const Audit::Report checked = Audit::Check(drawing);
  REQUIRE(checked.Findings.size() >= 2);
  CHECK(checked.Findings[0].What == "handle seed 5 not past 2B");
  CHECK(checked.Findings[1].What == "handle 2B of a CIRCLE used twice");
  Audit::Repair(drawing);
  CHECK(BaseOf(drawing.Objects.back()).Self == Handle{.Value = 0x2C});
  CHECK(drawing.Header.Handseed == Handle{.Value = 0x2D});
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/review: findings on the review drawings match AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  Drawing twice = Create();
  Get<Tables::Control>(*Find(twice, twice.Header.LayerControlObject)).Entries.push_back(twice.Header.Clayer);
  CHECK(!PassesAudit(Save(twice).value().Bytes, "dwgio-audit-twice-listed.dwg"));
}
} // namespace DwgIO
