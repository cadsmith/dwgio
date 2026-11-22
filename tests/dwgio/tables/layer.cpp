/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/layer.cpp
 *  @brief     The LAYER against its DXF twin, and R14 layers written as R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/layer.hpp"

#include "support/fixtures.hpp"
#include "support/twin.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Tables
{
namespace
{
auto Layers(const Drawing &drawing) -> std::vector<const Layer *>
{
  std::vector<const Layer *> layers;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *layer = GetIf<Layer>(&object))
    {
      layers.push_back(layer);
    }
  }
  return layers;
}

auto FindByName(const std::vector<const Layer *> &layers, std::string_view name) -> const Layer &
{
  const auto found = std::ranges::find(layers, name, [](const Layer *one) { return one->Record.Name; });
  REQUIRE(found != layers.end());
  return **found;
}
} // namespace

TEST_CASE("tables/layer: the fixture's layers carry their colors and linetypes and states")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  const auto ltype = [&twin](std::string_view name) -> Handle
  {
    const std::optional<std::size_t> opened = FindOpening(twin, "LTYPE", name);
    REQUIRE(opened);
    return Self(twin, *opened);
  };
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const bool modern = loaded->Drawing.Release >= Version::R2000;
    const std::vector<const Layer *> layers = Layers(loaded->Drawing);
    REQUIRE(layers.size() == 4);
    const Layer &walls = FindByName(layers, modern ? "Walls" : "WALLS");
    CHECK(walls.Color.Index == 1);
    CHECK(walls.Lineweight == (modern ? Lineweight::Mm050 : Lineweight::Default));
    CHECK(walls.Plot);
    CHECK(walls.On);
    CHECK_FALSE(walls.Frozen);
    CHECK_FALSE(walls.Locked);
    CHECK(walls.Linetype == ltype("Continuous"));
    const Layer &doors = FindByName(layers, modern ? "Doors" : "DOORS");
    CHECK(doors.Color.Index == 30);
    CHECK_FALSE(doors.On);
    CHECK(doors.Locked);
    CHECK(doors.Plot == !modern);
    CHECK(doors.Linetype == ltype("HIDDEN"));
    const Layer &hidden = FindByName(layers, modern ? "Hidden" : "HIDDEN");
    CHECK(hidden.Frozen);
    CHECK_FALSE(hidden.FrozenNew);
    CHECK(hidden.Linetype == ltype("DASHED"));
    for (const Layer *layer : layers)
    {
      CAPTURE(layer->Record.Name);
      CHECK(layer->Record.XrefIndex == -1);
      CHECK(layer->Record.Source == Handle{});
      CHECK(layer->Material == Handle{});
      CHECK(TypeOf(Object{*layer}) == "LAYER");
      if (!modern)
      {
        CHECK(layer->Style == Handle{});
        continue;
      }
      const std::optional<std::size_t> opened = FindOpening(twin, "LAYER", layer->Record.Name);
      REQUIRE(opened);
      CHECK(layer->Record.Base.Self == Self(twin, *opened));
      const int color = std::stoi(GroupOf(twin, *opened, 62));
      CHECK(layer->Color.Index == std::abs(color));
      CHECK(layer->On == (color > 0));
      const int flags = std::stoi(GroupOf(twin, *opened, 70));
      CHECK(layer->Frozen == ((flags & 1) != 0));
      CHECK(layer->FrozenNew == ((flags & 2) != 0));
      CHECK(layer->Locked == ((flags & 4) != 0));
      const std::string plot = GroupOf(twin, *opened, 290);
      CHECK(layer->Plot == (plot.empty() || plot == "1"));
      const int width = std::stoi(GroupOf(twin, *opened, 370));
      if (layer->Lineweight == Lineweight::Default)
      {
        CHECK(width == -3);
      }
      else
      {
        CHECK(std::lround(*MillimetersOf(layer->Lineweight) * 100) == width);
      }
      CHECK(layer->Style == ParseHandle(GroupOf(twin, *opened, 390)));
      const std::optional<std::size_t> linetype = FindOpening(twin, "LTYPE", GroupOf(twin, *opened, 6));
      REQUIRE(linetype);
      CHECK(layer->Linetype == Self(twin, *linetype));
    }
  }
}

TEST_CASE("tables/layer: an R14 drawing's layers written as R2000 read back unchanged with their names restored")
{
  const std::expected<Loaded, Failure> older = Load(ReadFixture("tables-r14.dwg"));
  REQUIRE(older);
  const std::expected<Saved, Failure> saved = Save(older->Drawing, SaveOptions{.Release = Version::R2000});
  REQUIRE(saved);
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  const std::vector<const Layer *> before = Layers(older->Drawing);
  const std::vector<const Layer *> after = Layers(back->Drawing);
  REQUIRE(before.size() == 4);
  REQUIRE(after.size() == 4);
  for (std::size_t at = 0; at < before.size(); ++at)
  {
    // R14 holds the names in upper case and the names they were in its EXTNAMES records, which AutoCAD reads.
    Layer expected = *before[at];
    CHECK(DwgIO::View::Matches(expected.Record.Name, after[at]->Record.Name));
    expected.Record.Name = after[at]->Record.Name;
    // The extension dictionary that held the record of the name alone is removed with it.
    if (after[at]->Record.Base.Dictionary.Value == 0)
    {
      expected.Record.Base.Dictionary = {};
    }
    CHECK(expected == *after[at]);
  }
  CHECK(std::ranges::contains(after, std::string{"Walls"}, [](const Layer *layer) { return layer->Record.Name; }));
}
} // namespace DwgIO::Tables
