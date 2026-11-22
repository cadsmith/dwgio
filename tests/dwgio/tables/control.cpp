/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/control.cpp
 *  @brief     The table controls and the records they list.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/control.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/table.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <expected>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Tables
{
namespace
{
auto ControlOf(const Drawing &drawing, Table table) -> const Control &
{
  for (const Object &object : drawing.Objects)
  {
    if (const auto *control = GetIf<Control>(&object); control != nullptr && control->Table == table)
    {
      return *control;
    }
  }
  FAIL("no such control");
  static const Control none{};
  return none;
}

auto Handles(const Drawing &drawing, std::string_view type) -> std::vector<Handle>
{
  std::vector<Handle> handles;
  for (const Object &object : drawing.Objects)
  {
    if (TypeOf(object) == type)
    {
      handles.push_back(BaseOf(object).Self);
    }
  }
  return handles;
}
} // namespace

TEST_CASE("tables/control: each table's control lists its records")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  const auto record = [&twin](std::string_view name) -> Handle
  {
    const std::optional<std::size_t> opened = FindOpening(twin, "BLOCK_RECORD", name);
    REQUIRE(opened);
    return Self(twin, *opened);
  };
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    const Control &layers = ControlOf(drawing, Table::Layer);
    CHECK(std::ranges::is_permutation(layers.Entries, Handles(drawing, "LAYER")));
    CHECK(layers.Entries.size() == 4);
    CHECK(layers.ModelSpace == Handle{});
    CHECK(layers.RepeatedEntries.empty());
    const Control &blocks = ControlOf(drawing, Table::Block);
    CHECK(std::ranges::contains(blocks.Entries, record("Chair")));
    CHECK(std::ranges::contains(blocks.Entries, record("_Dot")));
    CHECK(std::ranges::contains(blocks.Entries, record("*Paper_Space0")));
    CHECK_FALSE(std::ranges::contains(blocks.Entries, record("*Model_Space")));
    CHECK(blocks.ModelSpace == record("*Model_Space"));
    CHECK(blocks.PaperSpace == record("*Paper_Space"));
    const Control &dimstyles = ControlOf(drawing, Table::DimStyle);
    CHECK(dimstyles.Entries.size() == 4);
    if (drawing.Release == Version::R2000)
    {
      REQUIRE(dimstyles.RepeatedEntries.size() == 3);
      for (const Handle extra : dimstyles.RepeatedEntries)
      {
        CHECK(std::ranges::contains(dimstyles.Entries, extra));
      }
    }
    else
    {
      CHECK(dimstyles.RepeatedEntries.empty());
    }
    CHECK(ControlOf(drawing, Table::AppId).Entries.size() == 9);
    const Control &ltypes = ControlOf(drawing, Table::LType);
    const auto ltype = [&twin](std::string_view wanted) -> Handle
    {
      const std::optional<std::size_t> opened = FindOpening(twin, "LTYPE", wanted);
      REQUIRE(opened);
      return Self(twin, *opened);
    };
    CHECK(ltypes.ByBlock == ltype("ByBlock"));
    CHECK(ltypes.ByLayer == ltype("ByLayer"));
    CHECK(ltypes.Entries.size() == 6);
    CHECK_FALSE(std::ranges::contains(ltypes.Entries, ltypes.ByLayer));
    CHECK(ControlOf(drawing, Table::Ucs).Entries.size() == 1);
    CHECK(TypeOf(Object{layers}) == "LAYER_CONTROL");
    CHECK(TypeOf(Object{dimstyles}) == "DIMSTYLE_CONTROL");
    std::size_t controls = 0;
    for (const Object &object : drawing.Objects)
    {
      controls += Holds<Control>(object) ? 1U : 0U;
    }
    CHECK(controls == 10);
  }
}

TEST_CASE("tables/control: the foundation's view control lists five empty slots")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("foundation-r2000.dwg"));
  REQUIRE(loaded);
  const Control &views = ControlOf(loaded->Drawing, Table::View);
  CHECK(views.Entries == std::vector<Handle>(5));
  CHECK(NameOf(Table::View) == "VIEW_CONTROL");
  CHECK(NameOf(Table::ViewportEntityHeader) == "VP_ENT_HDR_CONTROL");
}

TEST_CASE("tables/control: a control written with one more empty slot reads back with it")
{
  Control control;
  control.Base.Self = Handle{0x2};
  control.Base.Dictionary = Handle{0x1FF};
  control.Base.Reactors = {Handle{0x9}};
  control.Table = Table::Layer;
  control.Entries = {Handle{0x10}, Handle{}};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const std::vector<std::byte> bytes = Objects::WriteObject(Object{control}, with);
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> read = Objects::ReadObject(bytes, 0, reading);
  REQUIRE(read);
  CHECK(*read == Object{control});
  CHECK(issues.empty());
}
} // namespace DwgIO::Tables
