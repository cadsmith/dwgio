/***********************************************************************************************************************
 *  @file      source/edit/usage.cpp
 *  @brief     What still needs an object a removal would take.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "edit/usage.hpp"

#include "ascii.hpp"
#include "audit/pointers.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/dimension.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/plotsettings.hpp"
#include "dwgio/objects/sortents.hpp"
#include "dwgio/tables/axes.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/ucs.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tables/vport.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

namespace DwgIO::Edit
{
namespace
{
/**
 *  Every handle the header keeps but the seed.
 */
constexpr std::array<Handle Header::*, 50> Kept{
    &Header::CurrentViewportEntityHeader,
    &Header::Clayer,
    &Header::Textstyle,
    &Header::Celtype,
    &Header::Dimstyle,
    &Header::Cmlstyle,
    &Header::Pucsname,
    &Header::Pucsorthoref,
    &Header::Pucsbase,
    &Header::Ucsname,
    &Header::Ucsorthoref,
    &Header::Ucsbase,
    &Header::Dimtxsty,
    &Header::Dimldrblk,
    &Header::Dimblk,
    &Header::Dimblk1,
    &Header::Dimblk2,
    &Header::BlockControlObject,
    &Header::LayerControlObject,
    &Header::StyleControlObject,
    &Header::LTypeControlObject,
    &Header::ViewControlObject,
    &Header::UcsControlObject,
    &Header::VPortControlObject,
    &Header::AppIdControlObject,
    &Header::DimStyleControlObject,
    &Header::ViewportEntityHeaderControlObject,
    &Header::DictionaryAcadGroup,
    &Header::DictionaryAcadMLineStyle,
    &Header::DictionaryNamedObjects,
    &Header::DictionaryLayouts,
    &Header::DictionaryPlotSettings,
    &Header::DictionaryPlotStyles,
    &Header::Cpsnid,
    &Header::BlockRecordPaperSpace,
    &Header::BlockRecordModelSpace,
    &Header::LTypeByLayer,
    &Header::LTypeByBlock,
    &Header::LTypeContinuous,
    &Header::DictionaryMaterials,
    &Header::DictionaryColors,
    &Header::Cmaterial,
    &Header::Dimltype,
    &Header::Dimltex1,
    &Header::Dimltex2,
    &Header::DictionaryVisualStyles,
    &Header::DictionaryUnknown,
    &Header::Interfereobjvs,
    &Header::Interferevpvs,
    &Header::Dragvs,
};

void CollectAxes(const Tables::Axes &axes, std::vector<Handle> &out)
{
  out.insert(out.end(), {axes.Named, axes.Basis});
}

/**
 *  What a kind names that the audit's references leave out; nothing for most.
 */
template <class KindT>
void Hard(const KindT & /*kind*/, std::vector<Handle> & /*out*/)
{
}

void Hard(const Entities::Dimension &dimension, std::vector<Handle> &out)
{
  out.push_back(dimension.Block);
}

void Hard(const Entities::ArcDimension &dimension, std::vector<Handle> &out)
{
  out.push_back(dimension.Block);
}

void Hard(const Entities::LargeRadialDimension &dimension, std::vector<Handle> &out)
{
  out.push_back(dimension.Block);
}

void Hard(const Entities::MText &text, std::vector<Handle> &out)
{
  out.push_back(text.Application);
}

void Hard(const Entities::Viewport &viewport, std::vector<Handle> &out)
{
  out.insert(out.end(), {viewport.Clip, viewport.Header, viewport.Background, viewport.ShadePlotObject});
  CollectAxes(viewport.Axes, out);
}

void Hard(const Tables::DimStyle &style, std::vector<Handle> &out)
{
  out.insert(out.end(), {style.Dimldrblk, style.Dimblk, style.Dimblk1, style.Dimblk2, style.Dimltype, style.Dimltex1,
                         style.Dimltex2});
}

void Hard(const Tables::LType &linetype, std::vector<Handle> &out)
{
  for (const Tables::Dash &dash : linetype.Dashes)
  {
    out.push_back(dash.Style);
  }
}

void Hard(const Tables::Ucs &ucs, std::vector<Handle> &out)
{
  out.insert(out.end(), {ucs.Basis, ucs.Named});
}

void Hard(const Tables::View &view, std::vector<Handle> &out)
{
  out.insert(out.end(), {view.Background, view.Section});
  CollectAxes(view.Axes, out);
}

void Hard(const Tables::VPort &vport, std::vector<Handle> &out)
{
  out.push_back(vport.Background);
  CollectAxes(vport.Axes, out);
}

void Hard(const Tables::BlockRecord &record, std::vector<Handle> &out)
{
  out.push_back(record.Layout);
}

void Hard(const Tables::Control &control, std::vector<Handle> &out)
{
  out.insert(out.end(), {control.ModelSpace, control.PaperSpace, control.ByBlock, control.ByLayer});
}

void Hard(const Objects::MLineStyle &style, std::vector<Handle> &out)
{
  for (const Objects::MLineStyles::Element &element : style.Elements)
  {
    out.push_back(element.Record);
  }
}

void Hard(const Objects::Layout &layout, std::vector<Handle> &out)
{
  out.push_back(layout.PlotView);
  CollectAxes(layout.Axes, out);
}

void Hard(const Objects::PlotSettings &settings, std::vector<Handle> &out)
{
  out.insert(out.end(), {settings.PlotView, settings.VisualStyle});
}

void Hard(const Objects::Dictionary &dictionary, std::vector<Handle> &out)
{
  out.push_back(dictionary.Default.value_or(Handle{}));
}

void Hard(const Objects::SortEntsTable &table, std::vector<Handle> &out)
{
  out.push_back(table.Block);
}

/**
 *  Whether a dimension style or the header names a block as an arrow, as a release before R2000 does: by name, without
 *  the underscore AutoCAD's own arrow blocks bear.
 */
auto IsArrow(const Drawing &drawing, std::string_view name) -> bool
{
  const std::string_view bare = name.starts_with('_') ? name.substr(1) : name;
  const Header &header = drawing.Header;
  std::vector<std::string_view> arrows{header.DimblkName, header.Dimblk1Name, header.Dimblk2Name};
  for (const Object &object : drawing.Objects)
  {
    if (const auto *style = GetIf<Tables::DimStyle>(&object))
    {
      arrows.insert(arrows.end(), {style->Arrows.Dimblk, style->Arrows.Dimblk1, style->Arrows.Dimblk2});
    }
  }
  return std::ranges::any_of(arrows, [bare](std::string_view arrow) -> bool { return Ascii::Same(arrow, bare); });
}

/**
 *  Every handle an object names that a removal cannot follow.
 */
auto HardOf(Object &object) -> std::vector<Handle>
{
  std::vector<Handle> out;
  for (const Audit::Referring &referring : Audit::ReferringOf(object))
  {
    out.push_back(*referring.Target);
  }
  for (const Xdata &xdata : BaseOf(object).Extended)
  {
    out.push_back(xdata.AppId);
  }
  Visit([&out](const auto &kind) -> void { Hard(kind, out); }, object);
  return out;
}
} // namespace

auto FindUsed(Drawing &drawing, const std::unordered_set<std::uint64_t> &removed) -> Handle
{
  for (const auto member : Kept)
  {
    if (removed.contains((drawing.Header.*member).Value))
    {
      return drawing.Header.*member;
    }
  }
  for (Object &object : drawing.Objects)
  {
    const Handle self = BaseOf(object).Self;
    if (removed.contains(self.Value))
    {
      const auto *record = GetIf<Tables::BlockRecord>(&object);
      if (record != nullptr && IsArrow(drawing, record->Record.Name))
      {
        return self;
      }
      continue;
    }
    for (const Handle one : HardOf(object))
    {
      if (removed.contains(one.Value))
      {
        return one;
      }
    }
  }
  return {};
}
} // namespace DwgIO::Edit
