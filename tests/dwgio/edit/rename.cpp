/***********************************************************************************************************************
 *  @file      tests/dwgio/edit/rename.cpp
 *  @brief     Renaming records and blocks, and appending vertices to a polyline, in each release's form.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/edit/editor.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <expected>
#include <format>

namespace DwgIO
{
TEST_CASE("edit/rename: a layer and a block take their new names with the BLOCK following its record")
{
  for (const Version release : {Version::R14, Version::R2000, Version::R2018})
  {
    CAPTURE(NameOf(release));
    Drawing drawing = Create(release);
    const Handle walls =
        *Edit::Add(drawing, Tables::Layer{.Record = {.Name = "Walls"}, .Linetype = drawing.Header.LTypeContinuous});
    const Handle chair = *Edit::AddBlock(drawing, "Chair");
    Entities::Line line;
    line.Common.Layer = walls;
    const Handle drawn = *Edit::Add(drawing, Object{line});
    Entities::Insert placed;
    placed.Block = chair;
    REQUIRE(Edit::Add(drawing, Object{placed}));
    REQUIRE(Edit::Rename(drawing, walls, "Partitions"));
    REQUIRE(Edit::Rename(drawing, chair, "Seat"));
    CHECK(View::Record<Tables::Layer>(std::as_const(drawing), "Partitions")->Record.Base.Self == walls);
    CHECK(View::Record<Tables::Layer>(std::as_const(drawing), "Walls") == nullptr);
    CHECK(CommonOf(*Find(drawing, drawn))->Layer == walls);
    const Tables::BlockRecord *seat = View::Block(drawing, "seat");
    REQUIRE(seat != nullptr);
    CHECK(Get<Entities::Block>(*Find(drawing, seat->Block)).Name == "Seat");
    CHECK(Edit::Rename(drawing, walls, "partitions")); // Its own name in another case.
    for (const Audit::Finding &finding : Audit::Check(drawing).Findings)
    {
      FAIL_CHECK(finding.What);
    }
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(View::Block(back->Drawing, "Seat") != nullptr);
    QueueAudit(saved->Bytes, std::format("dwgio-rename-{}.dwg", NameOf(release)));
  }
}

TEST_CASE("edit/rename: required or anonymous or missing records and taken or forbidden names are refused")
{
  Drawing drawing = Create();
  const Drawing before = drawing;
  const Handle zero = View::Record<Tables::Layer>(std::as_const(drawing), "0")->Record.Base.Self;
  const Handle model = drawing.Header.BlockRecordModelSpace;
  CHECK(Edit::Rename(drawing, zero, "Base").error() ==
        Failure{.Code = FailureCode::Refused, .What = "layer 0 not renamed"});
  CHECK(Edit::Rename(drawing, model, "Main").error().Code == FailureCode::Refused);
  CHECK(Edit::Rename(drawing, Handle{.Value = 0xFFFFF}, "X").error() ==
        Failure{.Code = FailureCode::Missing, .What = "handle FFFFF missing"});
  CHECK(Edit::Rename(drawing, drawing.Header.DictionaryNamedObjects, "X").error().Code == FailureCode::Refused);
  const Handle walls = *Edit::Add(drawing, Tables::Layer{.Record = {.Name = "Walls"}});
  const Handle doors = *Edit::Add(drawing, Tables::Layer{.Record = {.Name = "Doors"}});
  CHECK(Edit::Rename(drawing, walls, "DOORS").error() ==
        Failure{.Code = FailureCode::Exists, .What = "layer DOORS exists"});
  CHECK(Edit::Rename(drawing, walls, "").error().Code == FailureCode::Refused);
  CHECK(Edit::Rename(drawing, walls, "*U1").error().Code == FailureCode::Refused);
  CHECK(Edit::Rename(drawing, walls, "a|b").error() ==
        Failure{.Code = FailureCode::Refused, .What = "layer name \"a|b\" not allowed"});
  const Handle anonymous = *Edit::AddBlock(drawing, "*U1");
  CHECK(Edit::Rename(drawing, anonymous, "Named").error().Code == FailureCode::Refused);
  const Handle bare = *Edit::AddBlock(drawing, "Bare");
  GetIf<Tables::BlockRecord>(Find(drawing, bare))->Block = Handle{};
  CHECK(Edit::Rename(drawing, bare, "Stripped"));
  GetIf<Tables::Layer>(Find(drawing, doors))->Record.Dependent = true;
  CHECK(Edit::Rename(drawing, doors, "Plan").error().Code == FailureCode::Refused);
}

TEST_CASE("edit/rename: an Editor renames and finds the record by its new name")
{
  Drawing drawing = Create();
  Edit::Editor edits{drawing};
  const Handle walls = *edits.Add(Tables::Layer{.Record = {.Name = "Walls"}});
  REQUIRE(edits.Rename(walls, "Partitions"));
  CHECK(edits.Add(Tables::Layer{.Record = {.Name = "partitions"}}).error().Code == FailureCode::Exists);
  CHECK(edits.Add(Tables::Layer{.Record = {.Name = "Walls"}}));
  CHECK_FALSE(edits.Rename(walls, "walls"));
  Object *found = edits.Find(walls);
  REQUIRE(found != nullptr);
  Get<Tables::Layer>(*found).Locked = true;
  CHECK(View::Record<Tables::Layer>(std::as_const(drawing), "Partitions")->Locked);
}

TEST_CASE("edit/rename: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
