/***********************************************************************************************************************
 *  @file      source/releases/omission.cpp
 *  @brief     The objects a save to another release leaves out, and the pointers to them it clears.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/omission.hpp"

#include "releases/acis.hpp"
#include "releases/binding.hpp"
#include "releases/untyped.hpp"

#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/entities/raster.hpp"
#include "dwgio/entities/section.hpp"
#include "dwgio/entities/underlay.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/plotsettings.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tables/vport.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The pointers it clears: a page setup's visual style, an image's definition and reactor, an underlay's definition, a
 *  section's settings and a solid's history.
 */
template <typename KindT, typename ClearT>
void ClearKindPointers(KindT &kind, const ClearT &clear)
{
  if constexpr (std::is_same_v<KindT, Objects::PlotSettings>)
  {
    clear(kind.VisualStyle, "visual style");
  }
  if constexpr (requires { kind.Raster.Definition; })
  {
    clear(kind.Raster.Definition, "image definition");
    clear(kind.Raster.Reactor, "image reactor");
  }
  if constexpr (requires { kind.Data.Definition; })
  {
    clear(kind.Data.Definition, "underlay definition");
  }
  if constexpr (std::is_same_v<KindT, Entities::SectionObject>)
  {
    clear(kind.Settings, "settings");
  }
  if constexpr (std::is_same_v<KindT, Entities::Solid3d>)
  {
    clear(kind.History, "history");
  }
}
} // namespace

auto ClearLeftOut(Handle &pointer, std::span<const Handle> leftOut) -> bool
{
  if (pointer == Handle{} || !std::ranges::binary_search(leftOut, pointer.Value, {}, &Handle::Value))
  {
    return false;
  }
  pointer = Handle{};
  return true;
}

auto CollectLeftOut(const Drawing &drawing, Version to) -> std::vector<Handle>
{
  std::vector<Handle> leftOut;
  if (drawing.Release == to)
  {
    return leftOut;
  }
  for (const Object &object : drawing.Objects)
  {
    if (!BindingOf(object, drawing, to).empty() || FindStranding(object, drawing.Release, to))
    {
      leftOut.push_back(BaseOf(object).Self);
    }
  }
  // Those already out among them merge in the sort below.
  const std::vector<Handle> torn = CollectTorn(drawing, leftOut);
  leftOut.insert(leftOut.end(), torn.begin(), torn.end());
  std::ranges::sort(leftOut, {}, &Handle::Value);
  const auto repeated = std::ranges::unique(leftOut);
  leftOut.erase(repeated.begin(), repeated.end());
  return leftOut;
}

auto Refusal(const DwgIO::Object &object, const Drawing &drawing, Version to) -> std::string
{
  const Handle self = BaseOf(object).Self;
  if (const std::string_view bound = BindingOf(object, drawing, to); !bound.empty())
  {
    return std::format("object {} left out, its {} {} cannot be written as {}", HexOf(self), NameOf(drawing.Release),
                       bound, NameOf(to));
  }
  if (std::optional<std::string> stranded = FindStranding(object, drawing.Release, to))
  {
    return std::move(*stranded);
  }
  return std::format("{} {} left out with its {}, which cannot be written whole as {}", TypeOf(object), HexOf(self),
                     KinOf(object, drawing), NameOf(to));
}

void ClearPointers(DwgIO::Object &object, std::span<const Handle> leftOut, std::vector<Issue> &issues)
{
  const Handle self = BaseOf(object).Self;
  const auto clear = [&leftOut, &issues, &object, self](Handle &pointer, std::string_view what) -> bool
  {
    const Handle was = pointer;
    if (!ClearLeftOut(pointer, leftOut))
    {
      return false;
    }
    issues.push_back(
        Issue{.Code = IssueCode::NotKept,
              .Subject = self,
              .What = std::format("{} of {} {} cleared, {} left out", what, TypeOf(object), HexOf(self), HexOf(was))});
    return true;
  };
  if (Common *common = CommonOf(object))
  {
    if (clear(common->Material, "material"))
    {
      common->MaterialOrigin = Origin::ByLayer; // The layer's, as an entity's material is with none of its own.
    }
    clear(common->FullStyle, "visual style");
    clear(common->FaceStyle, "face visual style");
    clear(common->EdgeStyle, "edge visual style");
  }
  Visit(
      [&clear](auto &kind) -> void
      {
        using KindT = std::remove_cvref_t<decltype(kind)>;
        if constexpr (std::is_same_v<KindT, Tables::View> || std::is_same_v<KindT, Tables::VPort> ||
                      std::is_same_v<KindT, Entities::Viewport>)
        {
          clear(kind.VisualStyle, "visual style");
          clear(kind.Background, "background");
          clear(kind.Sun, "sun");
        }
        if constexpr (std::is_same_v<KindT, Entities::Viewport>)
        {
          clear(kind.ShadePlotObject, "shade plot");
        }
        if constexpr (std::is_same_v<KindT, Tables::View>)
        {
          clear(kind.Section, "live section");
        }
        if constexpr (std::is_same_v<KindT, Objects::Layout>)
        {
          clear(kind.VisualStyle, "visual style");
        }
        if constexpr (std::is_same_v<KindT, Tables::Layer>)
        {
          clear(kind.Material, "material");
        }
        ClearKindPointers(kind, clear);
      },
      object);
}
} // namespace DwgIO::Releases
