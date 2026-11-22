/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/datatable.cpp
 *  @brief     The DATATABLE against its DXF twin, written back, and columns the library does not read.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/datatable.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/shapes/datatable.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/class.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
using Cell = DwgIO::Tag;
using DataTables::Column;

/**
 *  The classes a data table written alone needs.
 */
auto MakeClasses() -> std::vector<Class>
{
  return {Class{.Number = 500, .CppName = "AcDbDataTable", .DxfName = "DATATABLE", .ItemClass = 0x1F3}};
}

/**
 *  A data table written and read back as that release.
 */
auto RoundTrip(const DataTable &table, Version release, std::vector<Issue> &issues) -> std::optional<Object>
{
  const std::vector<Class> classes = MakeClasses();
  const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = classes, .Issues = &issues};
  return ReadObject(WriteObject(Object{table}, with), 0, reading);
}
} // namespace

TEST_CASE("objects/datatable: the data table of each fixture up to R2004 matches its DXF twin")
{
  for (const auto &[name, twinName, kind] :
       {std::tuple{std::string_view{"table-r14.dwg"}, std::string_view{"table-r2000.dxf"}, "ACDBDATATABLE"},
        std::tuple{std::string_view{"table-r2000.dwg"}, std::string_view{"table-r2000.dxf"}, "ACDBDATATABLE"},
        std::tuple{std::string_view{"table-r2004.dwg"}, std::string_view{"table-r2004.dxf"}, "DATATABLE"},
        std::tuple{std::string_view{"cells-r2000.dwg"}, std::string_view{"cells-r2000.dxf"}, "ACDBDATATABLE"},
        std::tuple{std::string_view{"cells-r2004.dwg"}, std::string_view{"cells-r2004.dxf"}, "DATATABLE"}})
  {
    CAPTURE(name);
    const std::vector<Pair> twin = ReadTwin(twinName);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *table = GetIf<DataTable>(&object);
      if (table == nullptr)
      {
        continue;
      }
      ++count;
      CHECK(TypeOf(object) == "DATATABLE");
      // DXFOUT gives the table and its records handles of their own, so the twin's is found by its kind.
      const std::optional<std::size_t> opened = FindOpening(twin, kind);
      REQUIRE(opened);
      CHECK(table->Version == ReadWhole(twin, *opened, 70));
      CHECK(static_cast<int>(table->Columns.size()) == ReadWhole(twin, *opened, 90));
      CHECK(table->Rows == ReadWhole(twin, *opened, 91));
      CHECK(table->Name == GroupOf(twin, *opened, 1));
      const std::vector<std::string> kinds = Every(twin, *opened, 92);
      REQUIRE(kinds.size() == table->Columns.size());
      for (std::size_t at = 0; at < kinds.size(); ++at)
      {
        const Column &column = table->Columns[at];
        CHECK(column.Kind == std::stoi(kinds[at]));
        CHECK(column.Name.empty());
        REQUIRE(column.Cells.size() == static_cast<std::size_t>(table->Rows));
        for (const Cell &cell : column.Cells)
        {
          CHECK(cell.Code == (column.Kind == 1 ? 93 : column.Kind == 2 ? 40 : 360));
        }
      }
      CHECK(Every(twin, *opened, 40).size() == static_cast<std::size_t>(table->Rows));
      CHECK(Every(twin, *opened, 93).size() == 2 * static_cast<std::size_t>(table->Rows));
      CHECK(Every(twin, *opened, 360).size() == static_cast<std::size_t>(table->Rows));
      // Each hard owner names an xrecord of the drawing the table owns.
      for (const Cell &cell : table->Columns.back().Cells)
      {
        const Object *owned = FindObject(loaded->Drawing, std::get<Handle>(cell.Value));
        REQUIRE(owned != nullptr);
        CHECK(TypeOf(*owned) == "XRECORD");
        CHECK(BaseOf(*owned).Owner == table->Base.Self);
      }
    }
    CHECK(count == 1);
  }
}

TEST_CASE("objects/datatable: a table of every column type dwgio reads round-trips in each release")
{
  DataTable table;
  table.Base.Self = Handle{0x100};
  table.Base.Owner = Handle{0x50};
  table.Rows = 2;
  table.Name = "parts";
  table.Columns = {
      Column{.Kind = 1,
             .Name = "count",
             .Cells = {Cell{.Code = 93, .Value = std::int64_t{3}}, Cell{.Code = 93, .Value = std::int64_t{-4}}}},
      Column{.Kind = 2, .Name = "size", .Cells = {Cell{.Code = 40, .Value = 1.5}, Cell{.Code = 40, .Value = 0.0}}},
      Column{.Kind = 6,
             .Name = "record",
             .Cells = {Cell{.Code = 360, .Value = Handle{0x101}}, Cell{.Code = 360, .Value = Handle{}}}}};
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2010})
  {
    CAPTURE(static_cast<int>(release));
    std::vector<Issue> issues;
    const std::optional<Object> back = RoundTrip(table, release, issues);
    REQUIRE(back);
    CHECK(*back == Object{table});
    CHECK(issues.empty());
  }
}

TEST_CASE("objects/datatable: a missing cell or a cell of another type is written as its column's zero")
{
  DataTable table;
  table.Base.Self = Handle{0x100};
  table.Rows = 1;
  table.Columns = {Column{.Kind = 1, .Name = {}, .Cells = {Cell{.Code = 1, .Value = std::string{"no"}}}},
                   Column{.Kind = 2, .Name = {}, .Cells = {}},
                   Column{.Kind = 6, .Name = {}, .Cells = {Cell{.Code = 40, .Value = 2.0}}}};
  std::vector<Issue> issues;
  const std::optional<Object> back = RoundTrip(table, Version::R2000, issues);
  REQUIRE(back);
  const auto &read = Get<DataTable>(*back);
  CHECK(read.Columns[0].Cells == std::vector<Cell>{Cell{.Code = 93, .Value = std::int64_t{0}}});
  CHECK(read.Columns[1].Cells == std::vector<Cell>{Cell{.Code = 40, .Value = 0.0}});
  CHECK(read.Columns[2].Cells == std::vector<Cell>{Cell{.Code = 360, .Value = Handle{}}});
  CHECK(issues.empty());
  table.Rows = -1; // No cell written, and the count refused when read back.
  const std::optional<Object> none = RoundTrip(table, Version::R2000, issues);
  REQUIRE(none);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What.ends_with("data cut short"));
}

TEST_CASE("objects/datatable: a column of a type dwgio does not read keeps the object as bytes")
{
  DataTable table;
  table.Base.Self = Handle{0x100};
  table.Rows = 1;
  table.Columns = {Column{.Kind = 3, .Name = "text", .Cells = {}}};
  std::vector<Issue> issues;
  const std::optional<Object> back = RoundTrip(table, Version::R2000, issues);
  REQUIRE(back);
  CHECK(Holds<Unknown>(*back));
  CHECK(TypeOf(*back) == "DATATABLE");
}

TEST_CASE("objects/datatable: negative or oversized counts stop the read")
{
  const auto read = [](std::int32_t columns, std::int32_t rows) -> bool
  {
    Bits::Writer data;
    data.BL(columns);
    data.BL(rows);
    data.TV("", Version::R2000, Latin);
    const std::vector<std::byte> bytes = data.Take();
    Bits::Reader in{bytes};
    const Fields::Input reading{.In = &in, .With = Fields::Context{.Release = Version::R2000, .Text = &Latin}};
    DataTable table;
    const bool done = Shapes::ReadDataTable(table, reading);
    CHECK(table.Columns.empty());
    return done || !in.HasFailed();
  };
  CHECK_FALSE(read(-1, 0));
  CHECK_FALSE(read(0, -1));
  CHECK_FALSE(read(100000, 0));
  CHECK_FALSE(read(0, 100000));
  CHECK(read(0, 0));
  const std::vector<std::byte> nothing;
  Bits::Reader empty{nothing};
  DataTable none;
  CHECK_FALSE(Shapes::ReadDataTable(
      none, Fields::Input{.In = &empty, .With = Fields::Context{.Release = Version::R2000, .Text = &Latin}}));
}
} // namespace DwgIO::Objects
