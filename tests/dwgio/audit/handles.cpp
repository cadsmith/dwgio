/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/handles.cpp
 *  @brief     The audit of handles: duplicates, nulls and a seed below a handle in use.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/circle.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"

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
auto Highest(const Drawing &drawing) -> Handle
{
  Handle highest;
  for (const Object &object : drawing.Objects)
  {
    highest = std::max(highest, BaseOf(object).Self);
  }
  return highest;
}

/**
 *  A circle appended by hand under a handle of the caller's choosing, past Edit.
 */
void Slip(Drawing &drawing, Handle self)
{
  Entities::Circle circle;
  circle.Base.Self = self;
  circle.Base.Owner = drawing.Header.BlockRecordModelSpace;
  circle.Common.Layer = drawing.Header.Clayer;
  circle.Radius = 1;
  drawing.Objects.emplace_back(circle);
}
} // namespace

TEST_CASE("audit/handles: a new drawing audits clean")
{
  Drawing drawing = Create();
  const Drawing before = drawing;
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(checked.Findings.empty());
  CHECK(checked.IsClean());
  CHECK(checked.Errors() == 0);
  CHECK(checked.Warnings() == 0);
  CHECK(Audit::Repair(drawing).Findings.empty());
  CHECK(drawing == before);
}

TEST_CASE("audit/handles: a handle used twice goes to the later object")
{
  Drawing drawing = Create();
  Entities::Line line;
  const std::expected<Handle, Failure> first = Edit::Add(drawing, line);
  REQUIRE(first);
  Slip(drawing, *first);
  const Drawing broken = drawing;
  // The circle slipped in is also off its block's chain and space, which the later checks note.
  const Audit::Report checked = Audit::Check(drawing);
  REQUIRE(!checked.Findings.empty());
  CHECK(checked.Findings[0].Level == Audit::Severity::Error);
  CHECK(checked.Findings[0].What == "handle 2B of a CIRCLE used twice");
  CHECK(checked.Findings[0].Subject == *first);
  CHECK(!checked.Findings[0].Fixed);
  CHECK(!checked.IsClean());
  CHECK(checked.Errors() >= 1);
  CHECK(drawing == broken);
  const Audit::Report repaired = Audit::Repair(drawing);
  REQUIRE(!repaired.Findings.empty());
  CHECK(repaired.Findings[0].Fixed);
  CHECK(repaired.IsClean());
  const Handle given = BaseOf(drawing.Objects.back()).Self;
  CHECK(given != *first);
  CHECK(given == Highest(drawing));
  CHECK(drawing.Header.Handseed > given);
  CHECK(BaseOf(*Find(drawing, *first)).Self == *first);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/handles: a null handle is given one")
{
  Drawing drawing = Create();
  Slip(drawing, Handle{});
  const Audit::Report checked = Audit::Check(drawing);
  REQUIRE(!checked.Findings.empty());
  CHECK(checked.Findings[0].Level == Audit::Severity::Error);
  CHECK(checked.Findings[0].What == "CIRCLE has no handle");
  CHECK(checked.Findings[0].Subject == Handle{});
  Audit::Repair(drawing);
  CHECK(BaseOf(drawing.Objects.back()).Self == Handle{.Value = 0x2B});
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/handles: a seed below a handle in use is moved past it")
{
  Drawing drawing = Create();
  drawing.Header.Handseed = Handle{.Value = 5};
  const Audit::Report checked = Audit::Check(drawing);
  REQUIRE(checked.Findings.size() == 1);
  CHECK(checked.Findings[0].Level == Audit::Severity::Error);
  CHECK(checked.Findings[0].What == "handle seed 5 not past 2A");
  CHECK(checked.Findings[0].Subject == Handle{});
  CHECK(drawing.Header.Handseed == Handle{.Value = 5});
  CHECK(Audit::Repair(drawing).IsClean());
  CHECK(drawing.Header.Handseed == Handle{.Value = 0x2B});
  CHECK(Audit::Check(drawing).Findings.empty());

  // A seed below the handles and a handle used twice: the handle given comes past every handle and the old seed.
  Drawing both = Create();
  REQUIRE(Edit::Add(both, Entities::Line{}));
  Slip(both, Handle{.Value = 0x2B});
  both.Header.Handseed = Handle{.Value = 5};
  const Audit::Report repaired = Audit::Repair(both);
  CHECK(repaired.IsClean());
  CHECK(BaseOf(both.Objects.back()).Self == Handle{.Value = 0x2C});
  CHECK(both.Header.Handseed == Handle{.Value = 0x2D});
  CHECK(Audit::Check(both).Findings.empty());
}

TEST_CASE("audit/handles: findings on the broken-handle drawings match AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  // A handle used twice: AutoCAD's audit is quiet, and its own save holds the circle alone; the line is gone.
  Drawing twice = Create();
  REQUIRE(Edit::Add(twice, Entities::Line{}));
  Slip(twice, Handle{.Value = 0x2B});
  CHECK(PassesAudit(Save(twice).value().Bytes, "dwgio-audit-twice.dwg"));
  const std::expected<Loaded, Failure> theirs = Load(Resave(Save(twice).value().Bytes, "dwgio-audit-twice"));
  REQUIRE(theirs);
  CHECK(std::ranges::count_if(theirs->Drawing.Objects,
                              [](const Object &object) -> bool { return Holds<Entities::Line>(object); }) == 0);
  CHECK(std::ranges::count_if(theirs->Drawing.Objects,
                              [](const Object &object) -> bool { return Holds<Entities::Circle>(object); }) == 1);
  Audit::Repair(twice);
  const std::expected<Loaded, Failure> ours = Load(Resave(Save(twice).value().Bytes, "dwgio-audit-twice-repaired"));
  REQUIRE(ours);
  const Handle model = ours->Drawing.Header.BlockRecordModelSpace;
  CHECK(std::ranges::count_if(ours->Drawing.Objects, [model](const Object &object) -> bool
                              { return CommonOf(object) != nullptr && BaseOf(object).Owner == model; }) == 4);
  // A seed below the handles in use: AutoCAD does not open the drawing at all.
  Drawing seed = Create();
  seed.Header.Handseed = Handle{.Value = 5};
  CHECK(!PassesAudit(Save(seed).value().Bytes, "dwgio-audit-seed.dwg"));
  CHECK(Resave(Save(seed).value().Bytes, "dwgio-audit-seed").empty());
}
} // namespace DwgIO
