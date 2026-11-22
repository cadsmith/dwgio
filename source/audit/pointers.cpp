/***********************************************************************************************************************
 *  @file      source/audit/pointers.cpp
 *  @brief     The handles each kind of object names, for the audit.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "audit/pointers.hpp"

#include "dwgio/common.hpp"
#include "dwgio/entities/dimension.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/leader.hpp"
#include "dwgio/entities/mline.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/entities/raster.hpp"
#include "dwgio/entities/shape.hpp"
#include "dwgio/entities/table.hpp"
#include "dwgio/entities/tolerance.hpp"
#include "dwgio/entities/underlay.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/image.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/material.hpp"
#include "dwgio/objects/mleaderstyle.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/placeholder.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/objects/underlay.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tables/vport.hpp"
#include "dwgio/unknown.hpp"

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace DwgIO::Audit
{
namespace
{
constexpr std::array<std::string_view, 13> Words{"layer",           "linetype",    "text style", "dimension style",
                                                 "multiline style", "plot style",  "block",      "multileader style",
                                                 "arrowhead",       "table style", "material",   "visual style",
                                                 "history"};

/**
 *  A handle a multileader or its style names, when it is not null: such a handle may be left null.
 */
void Optional(Handle &target, Wanted kind, std::vector<Referring> &out)
{
  if (target)
  {
    out.push_back(Referring{.Target = &target, .Origin = nullptr, .Kind = kind});
  }
}

/**
 *  What a multileader names: its style, its lines' linetype, its text styles, its block, its arrowheads.
 */
void Callout(Entities::MultiLeaders::MLeader &callout, std::vector<Referring> &out)
{
  Optional(callout.Style, Wanted::MleaderStyle, out);
  Optional(callout.Linetype, Wanted::Linetype, out);
  Optional(callout.TextStyle, Wanted::TextStyle, out);
  Optional(callout.Context.TextStyle, Wanted::TextStyle, out);
  Optional(callout.BlockContent, Wanted::Block, out);
  Optional(callout.Context.BlockContent, Wanted::Block, out);
  Optional(callout.Arrowhead, Wanted::Arrowhead, out);
  for (Entities::MultiLeaders::LeaderRoot &branch : callout.Context.Leaders)
  {
    for (Entities::MultiLeaders::LeaderLine &stem : branch.Lines)
    {
      Optional(stem.Linetype, Wanted::Linetype, out);
      Optional(stem.Arrowhead, Wanted::Arrowhead, out);
    }
  }
  for (Entities::MultiLeaders::Arrowhead &tip : callout.Arrowheads)
  {
    Optional(tip.Block, Wanted::Arrowhead, out);
  }
}

/**
 *  What a multileader style names: its lines' linetype and arrowhead, its text style, its block, which a missing one
 *  leaves null.
 */
void Style(Objects::MLeaderStyle &style, std::vector<Referring> &out)
{
  Optional(style.Leader.Linetype, Wanted::Linetype, out);
  Optional(style.Leader.Arrowhead, Wanted::Arrowhead, out);
  Optional(style.Text.Style, Wanted::TextStyle, out);
  Optional(style.Block.Content, Wanted::Arrowhead, out);
}

constexpr std::array<std::string_view, 5> RasterWords{"image definition", "image reactor", "PDF definition",
                                                      "DWF definition", "DGN definition"};

/**
 *  The definition each kind of underlay names.
 */
constexpr std::array<Wanted, 3> Definitions{Wanted::PdfDefinition, Wanted::DwfDefinition, Wanted::DgnDefinition};

/**
 *  A raster's definition and reactor, each when it names one.
 */
void CollectRaster(Entities::Raster &raster, std::vector<Referring> &out)
{
  if (raster.Definition)
  {
    out.push_back(Referring{.Target = &raster.Definition, .Origin = nullptr, .Kind = Wanted::ImageDef});
  }
  if (raster.Reactor)
  {
    out.push_back(Referring{.Target = &raster.Reactor, .Origin = nullptr, .Kind = Wanted::ImageReactor});
  }
}

/**
 *  Whether an object is the definition a reference wants.
 */
auto Defines(const Object &object, Wanted wanted) -> bool
{
  if (wanted == Wanted::ImageDef)
  {
    return Holds<Objects::ImageDef>(object);
  }
  if (wanted == Wanted::ImageReactor)
  {
    return Holds<Objects::ImageDefReactor>(object);
  }
  if (wanted == Wanted::PdfDefinition)
  {
    return Holds<Objects::PdfDefinition>(object);
  }
  if (wanted == Wanted::DwfDefinition)
  {
    return Holds<Objects::DwfDefinition>(object);
  }
  return Holds<Objects::DgnDefinition>(object);
}

/**
 *  A table style's text styles: its cells' up to R2007, its cell styles' from R2010, each when it names one.
 */
void TextStyles(Objects::TableStyle &style, std::vector<Referring> &out)
{
  for (Objects::TableStyles::RowStyle &cell : style.Cells)
  {
    out.push_back(Referring{.Target = &cell.Style, .Origin = nullptr, .Kind = Wanted::TextStyle});
  }
  for (Objects::TableStyles::NamedCellStyle &named : style.Styles)
  {
    if (named.Style.Content.Style)
    {
      out.push_back(Referring{.Target = &named.Style.Content.Style, .Origin = nullptr, .Kind = Wanted::TextStyle});
    }
  }
}

template <class KindT>
void CollectNamed(KindT &kind, std::vector<Referring> &out)
{
  using Kind = std::remove_cvref_t<KindT>;
  if constexpr (requires { kind.Lettering.Style; })
  {
    out.push_back(Referring{.Target = &kind.Lettering.Style, .Origin = nullptr, .Kind = Wanted::TextStyle});
  }
  else if constexpr (std::is_same_v<Kind, Entities::MText> || std::is_same_v<Kind, Entities::Shape>)
  {
    out.push_back(Referring{.Target = &kind.Style, .Origin = nullptr, .Kind = Wanted::TextStyle});
  }
  else if constexpr (std::is_same_v<Kind, Entities::Dimension> || std::is_same_v<Kind, Entities::Leader> ||
                     std::is_same_v<Kind, Entities::Tolerance> || std::is_same_v<Kind, Entities::ArcDimension> ||
                     std::is_same_v<Kind, Entities::LargeRadialDimension>)
  {
    out.push_back(Referring{.Target = &kind.Style, .Origin = nullptr, .Kind = Wanted::DimStyle});
  }
  else if constexpr (std::is_same_v<Kind, Entities::MLine>)
  {
    out.push_back(Referring{.Target = &kind.Style, .Origin = nullptr, .Kind = Wanted::MlineStyle});
  }
  else if constexpr (std::is_same_v<Kind, Entities::Insert> || std::is_same_v<Kind, Objects::Layout>)
  {
    out.push_back(Referring{.Target = &kind.Block, .Origin = nullptr, .Kind = Wanted::Block});
  }
  else if constexpr (std::is_same_v<Kind, Tables::Layer>)
  {
    out.push_back(Referring{.Target = &kind.Linetype, .Origin = nullptr, .Kind = Wanted::Linetype});
    if (kind.Style)
    {
      out.push_back(Referring{.Target = &kind.Style, .Origin = nullptr, .Kind = Wanted::PlotStyle});
    }
  }
  else if constexpr (std::is_same_v<Kind, Tables::DimStyle>)
  {
    out.push_back(Referring{.Target = &kind.Dimtxsty, .Origin = nullptr, .Kind = Wanted::TextStyle});
  }
  else if constexpr (requires { kind.Raster; })
  {
    CollectRaster(kind.Raster, out);
  }
  else if constexpr (requires { kind.Data.Definition; })
  {
    constexpr Wanted Defined = Definitions.at(static_cast<std::size_t>(Kind::Shows));
    out.push_back(Referring{.Target = &kind.Data.Definition, .Origin = nullptr, .Kind = Defined});
  }
  else if constexpr (std::is_same_v<Kind, Entities::MultiLeader>)
  {
    Callout(kind.MLeader, out);
  }
  else if constexpr (std::is_same_v<Kind, Objects::MLeaderStyle>)
  {
    Style(kind, out);
  }
  else if constexpr (std::is_same_v<Kind, Entities::AcadTable>)
  {
    out.push_back(Referring{.Target = &kind.Block, .Origin = nullptr, .Kind = Wanted::Block});
    out.push_back(Referring{.Target = &kind.Style, .Origin = nullptr, .Kind = Wanted::TableStyle});
  }
  else if constexpr (std::is_same_v<Kind, Objects::TableStyle>)
  {
    TextStyles(kind, out);
  }
  else if constexpr (std::is_same_v<Kind, Entities::Solid3d>)
  {
    if (kind.History)
    {
      out.push_back(Referring{.Target = &kind.History, .Origin = nullptr, .Kind = Wanted::History});
    }
  }
}

/**
 *  The materials and visual styles an object names, each when it names one.
 */
template <class KindT>
void CollectLooks(KindT &kind, std::vector<Referring> &out)
{
  using Kind = std::remove_cvref_t<KindT>;
  if constexpr (std::is_same_v<Kind, Tables::Layer>)
  {
    if (kind.Material)
    {
      out.push_back(Referring{.Target = &kind.Material, .Origin = nullptr, .Kind = Wanted::Material});
    }
  }
  else if constexpr (std::is_same_v<Kind, Tables::View> || std::is_same_v<Kind, Tables::VPort> ||
                     std::is_same_v<Kind, Entities::Viewport> || std::is_same_v<Kind, Objects::Layout>)
  {
    if (kind.VisualStyle)
    {
      out.push_back(Referring{.Target = &kind.VisualStyle, .Origin = nullptr, .Kind = Wanted::VisualStyle});
    }
  }
}

/**
 *  An entity's own material and visual styles, each when it names one.
 */
void CollectRendering(Common &common, std::vector<Referring> &out)
{
  if (common.MaterialOrigin == Origin::Record && common.Material)
  {
    out.push_back(Referring{.Target = &common.Material, .Origin = &common.MaterialOrigin, .Kind = Wanted::Material});
  }
  for (Handle *style : {&common.FullStyle, &common.FaceStyle, &common.EdgeStyle})
  {
    if (*style)
    {
      out.push_back(Referring{.Target = style, .Origin = nullptr, .Kind = Wanted::VisualStyle});
    }
  }
}
} // namespace

auto ReferringOf(Object &object) -> std::vector<Referring>
{
  std::vector<Referring> out;
  if (Common *common = CommonOf(object); common != nullptr)
  {
    out.push_back(Referring{.Target = &common->Layer, .Origin = nullptr, .Kind = Wanted::Layer});
    if (common->LinetypeOrigin == Origin::Record)
    {
      out.push_back(
          Referring{.Target = &common->Linetype, .Origin = &common->LinetypeOrigin, .Kind = Wanted::Linetype});
    }
    // AutoCAD writes a SEQEND with its own plot style and no handle: a null one is nothing to check.
    if (common->PlotstyleOrigin == Origin::Record && common->Plotstyle)
    {
      out.push_back(
          Referring{.Target = &common->Plotstyle, .Origin = &common->PlotstyleOrigin, .Kind = Wanted::PlotStyle});
    }
  }
  Visit([&out](auto &kind) -> void { CollectNamed(kind, out); }, object);
  if (Common *common = CommonOf(object); common != nullptr)
  {
    CollectRendering(*common, out);
  }
  Visit([&out](auto &kind) -> void { CollectLooks(kind, out); }, object);
  return out;
}

auto WordOf(Wanted wanted) -> std::string_view
{
  const auto at = static_cast<std::size_t>(wanted);
  return at < Words.size() ? Words.at(at) : RasterWords.at(at - Words.size());
}

auto Fits(const Object &object, Wanted wanted) -> bool
{
  if (wanted == Wanted::Layer)
  {
    return Holds<Tables::Layer>(object);
  }
  if (wanted == Wanted::Linetype)
  {
    return Holds<Tables::LType>(object);
  }
  if (wanted == Wanted::TextStyle)
  {
    return Holds<Tables::Style>(object);
  }
  if (wanted == Wanted::DimStyle)
  {
    return Holds<Tables::DimStyle>(object);
  }
  if (wanted == Wanted::MlineStyle)
  {
    return Holds<Objects::MLineStyle>(object);
  }
  if (wanted == Wanted::PlotStyle)
  {
    return Holds<Objects::Placeholder>(object);
  }
  if (wanted == Wanted::MleaderStyle)
  {
    return Holds<Objects::MLeaderStyle>(object);
  }
  if (wanted == Wanted::TableStyle)
  {
    return Holds<Objects::TableStyle>(object);
  }
  if (wanted == Wanted::Material)
  {
    return Holds<Objects::Material>(object);
  }
  if (wanted == Wanted::VisualStyle)
  {
    return Holds<Objects::VisualStyle>(object);
  }
  if (wanted == Wanted::Block || wanted == Wanted::Arrowhead)
  {
    return Holds<Tables::BlockRecord>(object);
  }
  if (wanted == Wanted::History)
  {
    const auto *unknown = GetIf<Unknown>(&object);
    return unknown != nullptr && unknown->Class.View() == "ACSH_HISTORY_CLASS";
  }
  return Defines(object, wanted);
}
} // namespace DwgIO::Audit
