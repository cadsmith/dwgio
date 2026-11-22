/***********************************************************************************************************************
 *  @file      tests/dwgio/appearance.cpp
 *  @brief     What entities are drawn with, ByLayer and ByBlock resolved through inserts, and an insert's attributes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/appearance.hpp"

#include "support/fixtures.hpp"

#include "dwgio/edit.hpp"
#include "dwgio/entities/circle.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/index.hpp"
#include "dwgio/lineweight.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/origin.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <vector>

namespace DwgIO
{
namespace
{
auto Self(const Drawing &drawing, const char *name) -> Handle
{
  const auto *layer = View::Record<Tables::Layer>(drawing, name);
  if (layer != nullptr)
  {
    return layer->Record.Base.Self;
  }
  return View::Record<Tables::LType>(drawing, name)->Record.Base.Self;
}
} // namespace

TEST_CASE("appearance: ByLayer follows the layer and ByBlock the insert with layer 0 taking the insert's layer")
{
  Drawing drawing = LoadFixture("tables-r2004.dwg").Drawing;
  const Handle zero = Self(drawing, "0");
  const Handle walls = Self(drawing, "Walls");
  const Handle dashed = Self(drawing, "DASHED");
  Entities::Line stroke;
  stroke.Common.Color.Index = 0;
  stroke.Common.LinetypeOrigin = Origin::ByBlock;
  stroke.Common.Lineweight = Lineweight::ByBlock;
  Entities::Circle ring;
  ring.Common.Layer = zero;
  const Handle mark = *Edit::AddBlock(drawing, "Mark");
  REQUIRE(Edit::Add(drawing, Object{stroke}, "Mark"));
  REQUIRE(Edit::Add(drawing, Object{ring}, "Mark"));
  Entities::Insert onWalls;
  onWalls.Common.Layer = walls;
  onWalls.Block = mark;
  Entities::Insert green;
  green.Common.Layer = zero;
  green.Common.Color.Index = 3;
  green.Common.LinetypeOrigin = Origin::Record;
  green.Common.Linetype = dashed;
  green.Common.Lineweight = Lineweight::Mm018;
  green.Block = mark;
  const Handle first = *Edit::Add(drawing, Object{onWalls});
  const Handle second = *Edit::Add(drawing, Object{green});
  const Index index{drawing};
  const Tables::BlockRecord &block = *View::Block(index, "Mark");
  const std::vector<const Object *> parts = View::Entities(index, block);
  REQUIRE(parts.size() == 2);
  const Appearance placed = AppearanceOf(index, *CommonOf(*index.Find(first)), nullptr);
  const Appearance line = AppearanceOf(index, *CommonOf(*parts[0]), &placed);
  const Appearance circle = AppearanceOf(index, *CommonOf(*parts[1]), &placed);
  const Handle continuous = drawing.Header.LTypeContinuous;
  CHECK(
      line ==
      Appearance{.Layer = walls, .Color = Color{.Index = 1}, .Linetype = continuous, .Lineweight = Lineweight::Mm050});
  CHECK(circle == line);
  Common own;
  own.Layer = walls;
  CHECK(AppearanceOf(index, own, &placed).Layer == walls);
  const Appearance colored = AppearanceOf(index, *CommonOf(*index.Find(second)), nullptr);
  CHECK(AppearanceOf(index, *CommonOf(*parts[0]), &colored) ==
        Appearance{.Layer = zero, .Color = Color{.Index = 3}, .Linetype = dashed, .Lineweight = Lineweight::Mm018});
  CHECK(
      AppearanceOf(index, *CommonOf(*parts[1]), &colored) ==
      Appearance{.Layer = zero, .Color = Color{.Index = 7}, .Linetype = continuous, .Lineweight = Lineweight::Default});
  // Outside any block, by block draws white with Continuous at the default lineweight.
  CHECK(
      AppearanceOf(index, *CommonOf(*parts[0]), nullptr) ==
      Appearance{.Layer = zero, .Color = Color{.Index = 7}, .Linetype = continuous, .Lineweight = Lineweight::Default});
}

TEST_CASE("appearance: an R14 entity's ByLayer and ByBlock records resolve to the R2000 forms")
{
  const Drawing drawing = LoadFixture("tables-r14.dwg").Drawing;
  const Index index{drawing};
  const Handle walls = Self(drawing, "Walls");
  const auto &layer = Get<Tables::Layer>(*index.Find(walls));
  Common named;
  named.Layer = walls;
  named.LinetypeOrigin = Origin::Record;
  named.Linetype = drawing.Header.LTypeByLayer;
  CHECK(AppearanceOf(index, named, nullptr).Linetype == layer.Linetype);
  named.Linetype = drawing.Header.LTypeByBlock;
  CHECK(AppearanceOf(index, named, nullptr).Linetype == drawing.Header.LTypeContinuous);
  named.LinetypeOrigin = Origin::Continuous;
  CHECK(AppearanceOf(index, named, nullptr).Linetype == drawing.Header.LTypeContinuous);
  named.Color = Color{.Index = 2, .Rgb = static_cast<int>(0xC2102030U)};
  CHECK(AppearanceOf(index, named, nullptr).Color == named.Color);
  Common lost;
  lost.Layer = Handle{.Value = 0xFFFFF};
  CHECK(AppearanceOf(index, lost, nullptr).Color == Color{.Index = 7});
}

TEST_CASE("appearance: an entity with no layer 0 and no layer found takes the insert's layer and white")
{
  const Drawing bare;
  const Index index{bare};
  const Appearance outer{.Layer = Handle{.Value = 0x50}, .Color = Color{.Index = 3}, .Linetype = {}};
  const Appearance look = AppearanceOf(index, Common{}, &outer);
  CHECK(look.Layer == outer.Layer);
  CHECK(look.Color == Color{.Index = 7});
  CHECK(look.Lineweight == Lineweight::Default);
}
} // namespace DwgIO
