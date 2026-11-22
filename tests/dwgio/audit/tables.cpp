/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/tables.cpp
 *  @brief     The audit of tables and table styles naming styles or blocks the drawing lacks.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/table.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <expected>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
constexpr Handle Nowhere{.Value = 0x999};

template <typename KindT>
auto First(Drawing &drawing) -> KindT &
{
  for (Object &object : drawing.Objects)
  {
    if (auto *kind = GetIf<KindT>(&object))
    {
      return *kind;
    }
  }
  FAIL("none");
  return Get<KindT>(drawing.Objects.front());
}

auto Has(const Audit::Report &report, std::string_view what) -> bool
{
  return std::ranges::any_of(report.Findings, [what](const Audit::Finding &one) { return one.What == what; });
}

/**
 *  The ACAD_TABLESTYLE dictionary of the drawing.
 */
auto Styles(Drawing &drawing) -> Objects::Dictionary &
{
  const Objects::Dictionary *styles = View::Dictionary(drawing, "ACAD_TABLESTYLE");
  REQUIRE(styles != nullptr);
  return *GetIf<Objects::Dictionary>(Find(drawing, styles->Base.Self));
}
} // namespace

TEST_CASE("audit/tables: a table naming a missing style takes Standard and one of a missing block is dropped")
{
  Drawing drawing = LoadFixture("table-r2000.dwg").Drawing;
  CHECK(Audit::Check(drawing).IsClean());
  Entities::AcadTable &table = First<Entities::AcadTable>(drawing);
  const Handle style = table.Style;
  const Handle self = table.Base.Self;
  table.Style = Nowhere;
  Audit::Report report = Audit::Repair(drawing);
  CHECK(Has(report, "table style 999 of ACAD_TABLE 2CC missing"));
  CHECK(First<Entities::AcadTable>(drawing).Style == style);
  First<Entities::AcadTable>(drawing).Block = Nowhere;
  report = Audit::Repair(drawing);
  CHECK(Has(report, "block 999 of ACAD_TABLE 2CC missing, left out"));
  CHECK(Find(drawing, self) == nullptr);
}

TEST_CASE("audit/tables: a table style's cell text styles fall back to Standard")
{
  for (const std::string_view name : {"table-r2000.dwg", "table-r2018.dwg"})
  {
    CAPTURE(name);
    Drawing drawing = LoadFixture(name).Drawing;
    Objects::TableStyle &style = First<Objects::TableStyle>(drawing);
    Handle *text = style.Cells.empty() ? &style.Styles.back().Style.Content.Style : &style.Cells.back().Style;
    const Handle standard = *text;
    *text = Nowhere;
    const Audit::Report report = Audit::Repair(drawing);
    CHECK(Has(report, "text style 999 of TABLESTYLE 7F missing"));
    Objects::TableStyle &repaired = First<Objects::TableStyle>(drawing);
    CHECK((repaired.Cells.empty() ? repaired.Styles.back().Style.Content.Style : repaired.Cells.back().Style) ==
          standard);
  }
  // A cell style that names no text style is nothing to check.
  Drawing drawing = LoadFixture("table-r2018.dwg").Drawing;
  First<Objects::TableStyle>(drawing).Styles.front().Style.Content.Style = {};
  CHECK(Audit::Check(drawing).IsClean());
}

TEST_CASE("audit/tables: a table's style falls back to none without a Standard table style")
{
  Drawing drawing = LoadFixture("table-r2004.dwg").Drawing;
  First<Entities::AcadTable>(drawing).Style = Nowhere;
  Styles(drawing).Entries.front().Name = "Other";
  CHECK(Has(Audit::Repair(drawing), "table style 999 of ACAD_TABLE 2CC missing"));
  CHECK_FALSE(First<Entities::AcadTable>(drawing).Style);
  // A Standard entry that names no table style, then no dictionary at all.
  Styles(drawing).Entries.front().Name = "Standard";
  Styles(drawing).Entries.front().Object = drawing.Header.Clayer;
  First<Entities::AcadTable>(drawing).Style = Nowhere;
  CHECK(Has(Audit::Repair(drawing), "table style 999 of ACAD_TABLE 2CC missing"));
  CHECK_FALSE(First<Entities::AcadTable>(drawing).Style);
  Drawing made = Create();
  Entities::AcadTable table;
  table.Block = made.Header.BlockRecordModelSpace;
  table.Style = Nowhere;
  const Handle added = *Edit::Add(made, table);
  CHECK(Has(Audit::Repair(made), std::string{"table style 999 of ACAD_TABLE "} + HexOf(added) + " missing"));
}
} // namespace DwgIO
