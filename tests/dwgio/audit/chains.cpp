/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/chains.cpp
 *  @brief     The audit of block chains and sequences: loops, cuts, orphans, as AutoCAD grades them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/findings.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <expected>
#include <format>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
constexpr Handle Nowhere{.Value = 0x999};

auto Model(Drawing &drawing) -> Tables::BlockRecord &
{
  return *GetIf<Tables::BlockRecord>(Find(drawing, drawing.Header.BlockRecordModelSpace));
}

auto Handles(const std::vector<const Object *> &entities) -> std::vector<Handle>
{
  std::vector<Handle> handles;
  for (const Object *entity : entities)
  {
    handles.push_back(BaseOf(*entity).Self);
  }
  return handles;
}
} // namespace

TEST_CASE("audit/chains: a block whose chain loops or is cut is rebuilt in file order")
{
  Drawing drawing = Create();
  const Handle first = *Edit::Add(drawing, Entities::Line{});
  const Handle second = *Edit::Add(drawing, Entities::Line{});
  const Handle third = *Edit::Add(drawing, Entities::Line{});
  CommonOf(*Find(drawing, second))->Next = first;
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) == std::vector<std::string>{"chain of block *Model_Space broken"});
  CHECK(checked.Findings[0].Level == Audit::Severity::Error);
  CHECK(checked.Findings[0].Subject == drawing.Header.BlockRecordModelSpace);
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(Handles(View::Entities(drawing, Model(drawing))) == std::vector<Handle>{first, second, third});
  CHECK(Model(drawing).First == first);
  CHECK(Model(drawing).Last == third);
  CHECK(CommonOf(*Find(drawing, second))->Next == third);
  CHECK(CommonOf(*Find(drawing, third))->Previous == second);
  CHECK(CommonOf(*Find(drawing, third))->Next == Handle{});
  CHECK(Audit::Check(drawing).Findings.empty());

  Drawing cut = Create();
  REQUIRE(Edit::Add(cut, Entities::Line{}));
  Model(cut).Last = Nowhere;
  CHECK(FindingTexts(Audit::Check(cut)) == std::vector<std::string>{"chain of block *Model_Space broken"});
  Audit::Repair(cut);
  CHECK(Model(cut).Last == Model(cut).First);
  CHECK(Audit::Check(cut).Findings.empty());
}

TEST_CASE("audit/chains: an entity takes its block's space and one with no block goes to model space")
{
  Drawing drawing = Create();
  const Handle first = *Edit::Add(drawing, Entities::Line{});
  const Handle wrong = *Edit::Add(drawing, Entities::Line{});
  CommonOf(*Find(drawing, wrong))->Space = Space::Paper;
  const Handle orphan = AppendEntity<Entities::Line>(drawing, Nowhere);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) ==
        std::vector<std::string>{"owner 999 of LINE 2D is no block record", "space of LINE 2C is not its block's"});
  CHECK(checked.Warnings() == 2);
  CHECK(checked.IsClean());
  CHECK(checked.Findings[0].Subject == orphan);
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(BaseOf(*Find(drawing, orphan)).Owner == drawing.Header.BlockRecordModelSpace);
  CHECK(CommonOf(*Find(drawing, wrong))->Space == Space::Model);
  CHECK(Handles(View::Entities(drawing, Model(drawing))) == std::vector<Handle>{first, wrong, orphan});
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/chains: a polyline or insert gets its missing sequence and a sequence orphan is dropped")
{
  Drawing drawing = Create();
  const Handle polyline = *Edit::Add(drawing, Entities::Polyline{});
  const Handle one = AppendEntity<Entities::Vertex>(drawing, polyline);
  const Handle two = AppendEntity<Entities::Vertex>(drawing, polyline);
  Entities::Insert insert;
  insert.Block = drawing.Header.BlockRecordPaperSpace;
  insert.Follows = true;
  const Handle placed = *Edit::Add(drawing, insert);
  const Handle line = *Edit::Add(drawing, Entities::Line{});
  const Handle lost = AppendEntity<Entities::Vertex>(drawing, line);
  const Handle stray = AppendEntity<Entities::Attrib>(drawing, polyline);
  Get<Entities::Attrib>(*Find(drawing, stray)).Lettering.Style = drawing.Header.Textstyle;
  const Handle ended = AppendEntity<Entities::SeqEnd>(drawing, line);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) ==
        std::vector<std::string>{"VERTEX_2D 30 owned by no polyline, left out",
                                 "ATTRIB 31 owned by no insert, left out", "SEQEND 32 owned by no sequence, left out",
                                 "sequence of POLYLINE_2D 2B broken", "POLYLINE_2D 2B lacks its SEQEND",
                                 "INSERT 2E has no attribute"});
  CHECK(checked.Errors() == 0);
  CHECK(checked.IsClean());
  CHECK(Audit::Repair(drawing).IsClean());
  for (const Handle removed : {lost, stray, ended})
  {
    CHECK(Find(drawing, removed) == nullptr);
  }
  const Entities::Polyline &chained = Get<Entities::Polyline>(*Find(drawing, polyline));
  CHECK(chained.First == one);
  CHECK(chained.Last == two);
  CHECK(CommonOf(*Find(drawing, one))->Next == two);
  CHECK(CommonOf(*Find(drawing, two))->Previous == one);
  const Object *seqend = Find(drawing, chained.SeqEnd);
  REQUIRE(seqend);
  CHECK(Holds<Entities::SeqEnd>(*seqend));
  CHECK(BaseOf(*seqend).Owner == polyline);
  CHECK(CommonOf(*seqend)->Layer == drawing.Header.Clayer);
  const Entities::Insert &bare = Get<Entities::Insert>(*Find(drawing, placed));
  CHECK(!bare.Follows);
  CHECK(bare.First == Handle{});
  CHECK(bare.SeqEnd == Handle{});
  CHECK(Handles(View::Entities(drawing, Model(drawing))) == std::vector<Handle>{polyline, placed, line});
  CHECK(Audit::Check(drawing).Findings.empty());

  Drawing tagged = Create();
  Entities::Insert quiet;
  quiet.Block = tagged.Header.BlockRecordPaperSpace;
  const Handle silent = *Edit::Add(tagged, quiet);
  const Handle tag = AppendEntity<Entities::Attrib>(tagged, silent);
  Get<Entities::Attrib>(*Find(tagged, tag)).Lettering.Style = tagged.Header.Textstyle;
  CHECK(FindingTexts(Audit::Check(tagged)) == std::vector<std::string>{"INSERT 2B has attributes it does not say",
                                                                       "sequence of INSERT 2B broken",
                                                                       "INSERT 2B lacks its SEQEND"});
  Audit::Repair(tagged);
  const Entities::Insert &said = Get<Entities::Insert>(*Find(tagged, silent));
  CHECK(said.Follows);
  CHECK(said.First == tag);
  CHECK(said.Last == tag);
  CHECK(Holds<Entities::SeqEnd>(*Find(tagged, said.SeqEnd)));
  CHECK(Audit::Check(tagged).Findings.empty());
}

TEST_CASE("audit/chains: findings on the broken-chain drawings match AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  Drawing loop = Create();
  const Handle first = *Edit::Add(loop, Entities::Line{});
  const Handle second = *Edit::Add(loop, Entities::Line{});
  CommonOf(*Find(loop, second))->Next = first;
  CHECK(!PassesAudit(Save(loop).value().Bytes, "dwgio-audit-loop.dwg"));
  Audit::Repair(loop);
  CHECK(PassesAudit(Save(loop).value().Bytes, "dwgio-audit-loop-repaired.dwg"));
  Drawing space = Create();
  CommonOf(*Find(space, *Edit::Add(space, Entities::Line{})))->Space = Space::Paper;
  CHECK(PassesAudit(Save(space).value().Bytes, "dwgio-audit-space-code.dwg"));
  Drawing orphan = Create();
  AppendEntity<Entities::Line>(orphan, Nowhere);
  CHECK(PassesAudit(Save(orphan).value().Bytes, "dwgio-audit-orphan.dwg"));
  Drawing polyline = Create();
  const Handle drawn = *Edit::Add(polyline, Entities::Polyline{});
  AppendEntity<Entities::Vertex>(polyline, drawn);
  // AutoCAD's audit is quiet, and its own save keeps the polyline and closes it with a SEQEND of its own.
  CHECK(PassesAudit(Save(polyline).value().Bytes, "dwgio-audit-polyline.dwg"));
  const std::expected<Loaded, Failure> theirs = Load(Resave(Save(polyline).value().Bytes, "dwgio-audit-polyline"));
  REQUIRE(theirs);
  CHECK(std::ranges::count_if(theirs->Drawing.Objects, [drawn](const Object &object) -> bool
                              { return Holds<Entities::SeqEnd>(object) && BaseOf(object).Owner == drawn; }) == 1);
  Drawing insert = Create();
  Entities::Insert placed;
  placed.Block = insert.Header.BlockRecordPaperSpace;
  placed.Follows = true;
  REQUIRE(Edit::Add(insert, placed));
  CHECK(PassesAudit(Save(insert).value().Bytes, "dwgio-audit-attributes.dwg"));
  Drawing vertex = Create();
  AppendEntity<Entities::Vertex>(vertex, *Edit::Add(vertex, Entities::Line{}));
  CHECK(PassesAudit(Save(vertex).value().Bytes, "dwgio-audit-vertex.dwg"));
}

TEST_CASE("audit/chains: an R2004 block whose list names a foreign entity or misses one is rebuilt")
{
  Drawing drawing = Create(Version::R2004);
  const Handle drawn = *Edit::Add(drawing, Entities::Line{});
  const Handle model = drawing.Header.BlockRecordModelSpace;
  Get<Tables::BlockRecord>(*Find(drawing, model)).Entities.push_back(Handle{.Value = 0x999});
  const Audit::Report report = Audit::Repair(drawing);
  REQUIRE(report.Findings.size() == 1);
  CHECK(report.Findings[0].What == "entities of block *Model_Space broken");
  CHECK(report.Findings[0].Level == Audit::Severity::Warning); // AutoCAD silently opens such a drawing.
  CHECK(Get<Tables::BlockRecord>(*Find(drawing, model)).Entities == std::vector<Handle>{drawn});
  CHECK(Audit::Check(drawing).IsClean());
  // An entity the list misses is listed again, in file order.
  Get<Tables::BlockRecord>(*Find(drawing, model)).Entities.clear();
  CHECK(FindingTexts(Audit::Check(drawing)) == std::vector<std::string>{"entities of block *Model_Space broken"});
  Audit::Repair(drawing);
  CHECK(Get<Tables::BlockRecord>(*Find(drawing, model)).Entities == std::vector<Handle>{drawn});
  // An entity named twice is listed once.
  Get<Tables::BlockRecord>(*Find(drawing, model)).Entities.push_back(drawn);
  CHECK(FindingTexts(Audit::Check(drawing)) == std::vector<std::string>{"entities of block *Model_Space broken"});
  Audit::Repair(drawing);
  CHECK(Get<Tables::BlockRecord>(*Find(drawing, model)).Entities == std::vector<Handle>{drawn});
  // No chain is asked of an R2004 drawing, whose links are all null.
  CHECK(Audit::Check(Create(Version::R2004)).IsClean());
  // An entity owned by no block record goes to model space's list there, as it goes to its chain before.
  Drawing stray = Create(Version::R2004);
  const Handle lost = *Edit::Add(stray, Entities::Line{});
  BaseOf(*Find(stray, lost)).Owner = stray.Header.Clayer;
  Get<Tables::BlockRecord>(*Find(stray, stray.Header.BlockRecordModelSpace)).Entities.clear();
  const Audit::Report owned = Audit::Repair(stray);
  CHECK(std::ranges::any_of(owned.Findings,
                            [](const Audit::Finding &one) { return one.What.contains("is no block record"); }));
  CHECK(Get<Tables::BlockRecord>(*Find(stray, stray.Header.BlockRecordModelSpace)).Entities ==
        std::vector<Handle>{lost});
  CHECK(Audit::Check(stray).IsClean());
}

TEST_CASE("audit/chains: an R2004 drawing lists a polyline's vertices and a rebuilt model space's entities")
{
  Drawing drawing = Create(Version::R2004);
  Entities::Polyline polyline;
  const Handle head = *Edit::Add(drawing, polyline);
  std::vector<Handle> vertices;
  for (int at = 0; at < 2; ++at)
  {
    Entities::Vertex vertex;
    vertex.Base.Self = Edit::MakeHandle(drawing);
    vertex.Base.Owner = head;
    vertex.Common.Layer = drawing.Header.Clayer;
    vertex.Common.Lineweight = Lineweight::ByLayer;
    vertices.push_back(vertex.Base.Self);
    drawing.Objects.emplace_back(std::move(vertex));
  }
  Entities::SeqEnd seqend;
  seqend.Base.Self = Edit::MakeHandle(drawing);
  seqend.Base.Owner = head;
  seqend.Common.Layer = drawing.Header.Clayer;
  seqend.Common.Lineweight = Lineweight::ByLayer;
  drawing.Objects.emplace_back(std::move(seqend));
  auto &listed = Get<Entities::Polyline>(*Find(drawing, head));
  listed.SeqEnd = drawing.Objects.back().index() == 0 ? Handle{} : BaseOf(drawing.Objects.back()).Self;
  listed.Vertices = {vertices[1]};
  CHECK(FindingTexts(Audit::Check(drawing)) ==
        std::vector<std::string>{std::format("sequence of POLYLINE_2D {} broken", HexOf(head))});
  Audit::Repair(drawing);
  CHECK(Get<Entities::Polyline>(*Find(drawing, head)).Vertices == vertices);
  CHECK(Audit::Check(drawing).IsClean());
  // The model space record erased comes back with its list.
  const Handle model = drawing.Header.BlockRecordModelSpace;
  std::erase_if(drawing.Objects, [model](const Object &object) { return BaseOf(object).Self == model; });
  Audit::Repair(drawing);
  REQUIRE(Find(drawing, drawing.Header.BlockRecordModelSpace) != nullptr);
  CHECK(Get<Tables::BlockRecord>(*Find(drawing, drawing.Header.BlockRecordModelSpace)).Entities ==
        std::vector<Handle>{head});
  CHECK(Audit::Check(drawing).IsClean());
}
} // namespace DwgIO
