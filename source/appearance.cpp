/***********************************************************************************************************************
 *  @file      source/appearance.cpp
 *  @brief     The layer, color, linetype and lineweight an entity is drawn with, ByLayer and ByBlock resolved.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/appearance.hpp"

#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/origin.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/view.hpp"

namespace DwgIO
{
namespace
{
constexpr int White = 7;

/**
 *  An R14 entity names the ByLayer and ByBlock records where a later one says so in its origin.
 */
auto OriginOf(const Common &common, const Header &header) -> Origin
{
  if (common.LinetypeOrigin == Origin::Record && common.Linetype == header.LTypeByLayer)
  {
    return Origin::ByLayer;
  }
  if (common.LinetypeOrigin == Origin::Record && common.Linetype == header.LTypeByBlock)
  {
    return Origin::ByBlock;
  }
  return common.LinetypeOrigin;
}
} // namespace

auto AppearanceOf(const Index &index, const Common &common, const Appearance *outer) -> Appearance
{
  const Header &header = index.Source().Header;
  const Handle zero = [&index] -> Handle
  {
    const auto *layer = View::Record<Tables::Layer>(index, "0");
    return layer == nullptr ? Handle{} : layer->Record.Base.Self;
  }();
  Appearance look;
  look.Layer = outer != nullptr && common.Layer == zero ? outer->Layer : common.Layer;
  const Tables::Layer fallback{.Record = {}, .Color = Color{.Index = White}, .Linetype = header.LTypeContinuous};
  const auto *found = GetIf<Tables::Layer>(index.Find(look.Layer));
  const Tables::Layer &layer = found == nullptr ? fallback : *found;
  switch (MethodOf(common.Color))
  {
  case ColorMethod::ByLayer:
    look.Color = layer.Color;
    break;
  case ColorMethod::ByBlock:
    look.Color = outer != nullptr ? outer->Color : Color{.Index = White};
    break;
  default:
    look.Color = common.Color;
    break;
  }
  switch (OriginOf(common, header))
  {
  case Origin::ByLayer:
    look.Linetype = layer.Linetype;
    break;
  case Origin::ByBlock:
    look.Linetype = outer != nullptr ? outer->Linetype : header.LTypeContinuous;
    break;
  case Origin::Continuous:
    look.Linetype = header.LTypeContinuous;
    break;
  default:
    look.Linetype = common.Linetype;
    break;
  }
  const Lineweight own = common.Lineweight.value_or(Lineweight::ByLayer);
  if (own == Lineweight::ByLayer)
  {
    look.Lineweight = layer.Lineweight;
  }
  else if (own == Lineweight::ByBlock)
  {
    look.Lineweight = outer != nullptr ? outer->Lineweight : Lineweight::Default;
  }
  else
  {
    look.Lineweight = own;
  }
  return look;
}
} // namespace DwgIO
