/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/table/convert.cpp
 *  @brief     The records a table needs below R2018, made from its content where they are missing.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/table/convert.hpp"

#include "support/fixtures.hpp"
#include "support/pass.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/table.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/cellstylemap.hpp"
#include "dwgio/objects/datatable.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/tablecontent.hpp"
#include "dwgio/objects/tablegeometry.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

namespace DwgIO::Releases::Table
{
namespace
{
template <typename KindT>
auto CountOf(const Drawing &drawing) -> std::size_t
{
  return static_cast<std::size_t>(
      std::ranges::count_if(drawing.Objects, [](const Object &one) { return Holds<KindT>(one); }));
}

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
  throw;
}

/**
 *  The objects under the handles, and all they own, taken away.
 */
void Without(Drawing &drawing, std::unordered_set<std::uint64_t> removed)
{
  for (std::size_t before = 0; before != removed.size();)
  {
    before = removed.size();
    for (const Object &object : drawing.Objects)
    {
      if (removed.contains(BaseOf(object).Owner.Value))
      {
        removed.insert(BaseOf(object).Self.Value);
      }
    }
  }
  std::erase_if(drawing.Objects,
                [&removed](const Object &object) { return removed.contains(BaseOf(object).Self.Value); });
}

/**
 *  A drawing saved as the release and loaded back with no issue.
 */
auto RoundTrip(const Drawing &drawing, Version to) -> Drawing
{
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = to});
  REQUIRE(saved);
  const std::expected<DwgIO::Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  return loaded->Drawing;
}
} // namespace

TEST_CASE("releases/table/convert: nothing is rebuilt within a release or without a table")
{
  const Drawing drawing = LoadFixture("cells-r2018.dwg").Drawing;
  CHECK_FALSE(Pass(ConvertTables, drawing, Version::R2018).has_value());
  CHECK_FALSE(Pass(ConvertTables, Drawing{}, Version::R2004).has_value());
  CHECK(Pass(ConvertTables, drawing, Version::R2013).has_value());
}

TEST_CASE("releases/table/convert: an R2018 table saved below gets its records from its content")
{
  Drawing drawing = LoadFixture("cells-r2018.dwg").Drawing;
  auto &table = First<Entities::AcadTable>(drawing);
  const Handle dictionary = table.Base.Dictionary;
  table.Base.Dictionary = {};
  Without(drawing, {dictionary.Value});
  for (const Version to : {Version::R2007, Version::R2004})
  {
    CAPTURE(NameOf(to));
    Drawing back = RoundTrip(drawing, to);
    CHECK(CountOf<Objects::TableContent>(back) == 1);
    CHECK(CountOf<Objects::TableGeometry>(back) == 1);
    const auto &geometry = First<Objects::TableGeometry>(back);
    CHECK(std::ranges::all_of(geometry.Cells,
                              [](const Objects::TableGeometries::Cell &cell) { return cell.Contents.size() == 1; }));
    CHECK(First<Entities::AcadTable>(back).Cells.size() == 18);
  }
}

TEST_CASE("releases/table/convert: a table's missing records or record objects are rebuilt")
{
  Drawing drawing = LoadFixture("cells-r2018.dwg").Drawing;
  auto &table = First<Entities::AcadTable>(drawing);
  auto &dictionary = Get<Objects::Dictionary>(*Find(drawing, table.Base.Dictionary));
  const Handle record = dictionary.Entries.front().Object;
  dictionary.Entries.front().Name = "OTHER";
  Drawing unnamed = drawing;
  Without(unnamed, {record.Value});
  Drawing back = RoundTrip(unnamed, Version::R2007);
  CHECK(CountOf<Objects::TableContent>(back) == 1);
  auto &named = Get<Objects::Dictionary>(*Find(back, First<Entities::AcadTable>(back).Base.Dictionary));
  CHECK(named.Entries.size() == 2);
  dictionary.Entries.front().Name = "ACAD_XREC_ROUNDTRIP";
  auto &kept = Get<Objects::XRecord>(*Find(drawing, record));
  std::erase_if(kept.Data, [](const Tag &group) { return group.Code == 361; });
  back = RoundTrip(drawing, Version::R2004);
  CHECK(CountOf<Objects::TableGeometry>(back) == 2); // The one the record no longer names stays, as it was.
  CHECK(CountOf<Objects::DataTable>(back) == 1);
}

TEST_CASE("releases/table/convert: a table style gets a dictionary for its map only where it has none")
{
  Drawing drawing = LoadFixture("cells-r2018.dwg").Drawing;
  auto &style = First<Objects::TableStyle>(drawing);
  const Handle dictionary = style.Base.Dictionary;
  style.Base.Dictionary = {};
  style.Styles.pop_back();
  Without(drawing, {dictionary.Value});
  Drawing back = RoundTrip(drawing, Version::R2004);
  CHECK(CountOf<Objects::CellStyleMap>(back) == 1);
  auto &made = Get<Objects::Dictionary>(*Find(back, First<Objects::TableStyle>(back).Base.Dictionary));
  CHECK(made.Entries.size() == 2);

  Drawing mapped = LoadFixture("cells-r2018.dwg").Drawing;
  auto &entries = Get<Objects::Dictionary>(*Find(mapped, First<Objects::TableStyle>(mapped).Base.Dictionary));
  entries.Entries.push_back(Objects::Entry{.Name = "ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP",
                                           .Object = First<Entities::AcadTable>(mapped).Style});
  CHECK(CountOf<Objects::CellStyleMap>(*Pass(ConvertTables, mapped, Version::R2007)) == 0);
  entries.Entries.push_back(Objects::Entry{.Name = "ACAD_XREC_ROUNDTRIP", .Object = entries.Entries.back().Object});
  const std::size_t records = CountOf<Objects::XRecord>(mapped);
  const Drawing kept = *Pass(ConvertTables, mapped, Version::R2004);
  CHECK(CountOf<Objects::XRecord>(kept) == records + 18); // The data table's cells; the style's record was there.

  Drawing older = LoadFixture("cells-r2007.dwg").Drawing;
  auto &bare = First<Objects::TableStyle>(older);
  const Handle owned = bare.Base.Dictionary;
  bare.Base.Dictionary = {};
  Without(older, {owned.Value});
  CHECK(CountOf<Objects::CellStyleMap>(*Pass(ConvertTables, older, Version::R2010)) == 0);
}

TEST_CASE("releases/table/convert: a table or content or style without a handle or with a repeated one stays")
{
  Drawing drawing = LoadFixture("cells-r2018.dwg").Drawing;
  std::vector<Object> broken{First<Entities::AcadTable>(drawing), First<Objects::TableContent>(drawing),
                             First<Objects::TableStyle>(drawing)};
  BaseOf(broken[0]).Self = {};
  BaseOf(broken[1]).Self = drawing.Header.DictionaryNamedObjects; // A dictionary bears it first.
  BaseOf(broken[2]).Self = {};
  const std::size_t count = drawing.Objects.size();
  drawing.Objects.insert(drawing.Objects.end(), broken.begin(), broken.end());
  const std::optional<Drawing> made = Pass(ConvertTables, drawing, Version::R2004);
  REQUIRE(made);
  for (const Object &object : broken)
  {
    CHECK(std::ranges::contains(made->Objects, object));
  }
  CHECK(made->Objects.size() > count + broken.size());
}
} // namespace DwgIO::Releases::Table
