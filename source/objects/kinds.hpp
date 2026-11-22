/***********************************************************************************************************************
 *  @file      source/objects/kinds.hpp
 *  @brief     Every kind the library types, and the type or class each is stored under.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_KINDS_HPP
#define DWGIO_OBJECTS_KINDS_HPP

#include "entities/shapes/arc.hpp"
#include "entities/shapes/attdef.hpp"
#include "entities/shapes/attrib.hpp"
#include "entities/shapes/block.hpp"
#include "entities/shapes/circle.hpp"
#include "entities/shapes/dimension.hpp"
#include "entities/shapes/ellipse.hpp"
#include "entities/shapes/endblk.hpp"
#include "entities/shapes/face.hpp"
#include "entities/shapes/hatch.hpp"
#include "entities/shapes/helix.hpp"
#include "entities/shapes/insert.hpp"
#include "entities/shapes/leader.hpp"
#include "entities/shapes/light.hpp"
#include "entities/shapes/line.hpp"
#include "entities/shapes/lwpolyline.hpp"
#include "entities/shapes/mesh.hpp"
#include "entities/shapes/mline.hpp"
#include "entities/shapes/modeler.hpp"
#include "entities/shapes/mtext.hpp"
#include "entities/shapes/multileader.hpp"
#include "entities/shapes/ole2frame.hpp"
#include "entities/shapes/point.hpp"
#include "entities/shapes/polyline.hpp"
#include "entities/shapes/raster.hpp"
#include "entities/shapes/ray.hpp"
#include "entities/shapes/section.hpp"
#include "entities/shapes/seqend.hpp"
#include "entities/shapes/shape.hpp"
#include "entities/shapes/solid.hpp"
#include "entities/shapes/spline.hpp"
#include "entities/shapes/table.hpp"
#include "entities/shapes/text.hpp"
#include "entities/shapes/tolerance.hpp"
#include "entities/shapes/trace.hpp"
#include "entities/shapes/underlay.hpp"
#include "entities/shapes/vertex.hpp"
#include "entities/shapes/viewport.hpp"
#include "entities/shapes/xline.hpp"
#include "fields/context.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/datatable.hpp"
#include "objects/shapes/dictionary.hpp"
#include "objects/shapes/dictionaryvar.hpp"
#include "objects/shapes/group.hpp"
#include "objects/shapes/image.hpp"
#include "objects/shapes/layout.hpp"
#include "objects/shapes/material.hpp"
#include "objects/shapes/mleaderstyle.hpp"
#include "objects/shapes/mlinestyle.hpp"
#include "objects/shapes/placeholder.hpp"
#include "objects/shapes/plotsettings.hpp"
#include "objects/shapes/scale.hpp"
#include "objects/shapes/sortents.hpp"
#include "objects/shapes/table/content.hpp"
#include "objects/shapes/table/geometry.hpp"
#include "objects/shapes/table/map.hpp"
#include "objects/shapes/table/style.hpp"
#include "objects/shapes/underlay.hpp"
#include "objects/shapes/visualstyle.hpp"
#include "objects/shapes/wipeout.hpp"
#include "objects/shapes/xrecord.hpp"
#include "tables/shapes/appid.hpp"
#include "tables/shapes/block.hpp"
#include "tables/shapes/control.hpp"
#include "tables/shapes/dimstyle.hpp"
#include "tables/shapes/layer.hpp"
#include "tables/shapes/ltype.hpp"
#include "tables/shapes/style.hpp"
#include "tables/shapes/ucs.hpp"
#include "tables/shapes/view.hpp"
#include "tables/shapes/viewportheader.hpp"
#include "tables/shapes/vport.hpp"

#include "dwgio/issue.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace DwgIO::Objects
{
constexpr std::uint16_t FirstClass = 500;

/**
 *  In the order the reader tries them.
 */
using Kinds = std::tuple<
    Tables::AppId, Tables::Control, Tables::Layer, Tables::Style, Tables::LType, Tables::View, Tables::Ucs,
    Tables::VPort, Tables::DimStyle, Tables::BlockRecord, Entities::Line, Entities::Point, Entities::Circle,
    Entities::Arc, Entities::Ellipse, Entities::Ray, Entities::XLine, Entities::Face3d, Entities::Solid,
    Entities::Trace, Entities::Shape, Entities::Text, Entities::AttDef, Entities::Attrib, Entities::Block,
    Entities::EndBlk, Entities::SeqEnd, Entities::Insert, Entities::MText, Entities::Polyline, Entities::Vertex,
    Entities::LwPolyline, Entities::Spline, Entities::Hatch, Entities::Dimension, Entities::Leader, Entities::Tolerance,
    Entities::MLine, Entities::Viewport, Objects::Dictionary, Objects::Placeholder, Objects::DictionaryVar,
    Objects::XRecord, Objects::Group, Objects::MLineStyle, Objects::Scale, Objects::SortEntsTable, Objects::Layout,
    Tables::ViewportEntityHeader, Entities::ArcDimension, Entities::LargeRadialDimension, Objects::PlotSettings,
    Entities::Image, Entities::Wipeout, Objects::ImageDef, Objects::ImageDefReactor, Objects::RasterVariables,
    Objects::WipeoutVariables, Entities::PdfUnderlay, Entities::DwfUnderlay, Entities::DgnUnderlay,
    Objects::PdfDefinition, Objects::DwfDefinition, Objects::DgnDefinition, Entities::Ole2Frame, Entities::MultiLeader,
    Objects::MLeaderStyle, Entities::AcadTable, Objects::TableStyle, Objects::DataTable, Entities::Helix,
    Entities::Mesh, Entities::Light, Entities::SectionObject, Objects::Material, Objects::VisualStyle, Entities::Region,
    Entities::Solid3d, Entities::Body, Entities::Surface, Entities::PlaneSurface, Entities::NurbSurface,
    Entities::ExtrudedSurface, Entities::LoftedSurface, Entities::RevolvedSurface, Entities::SweptSurface,
    Objects::TableContent, Objects::TableGeometry, Objects::CellStyleMap>;

template <typename KindT>
auto NameOf([[maybe_unused]] const KindT &kind) -> std::string_view
{
  if constexpr (requires { Shape<KindT>::NameOf(kind); })
  {
    return Shape<KindT>::NameOf(kind);
  }
  else
  {
    return Shape<KindT>::Name;
  }
}

/**
 *  Its shape's list, or its name alone.
 */
template <typename KindT>
auto ClassNames() -> std::span<const std::string_view>
{
  if constexpr (requires { Shape<KindT>::Classes; })
  {
    return Shape<KindT>::Classes;
  }
  else
  {
    static constexpr std::array<std::string_view, 1> Own{Shape<KindT>::Name};
    return Own;
  }
}

/**
 *  500 plus the class's index among the context's classes; 0 and an issue when the drawing declares no such class.
 */
inline auto ClassCode(std::string_view name, const Fields::Context &with) -> std::uint16_t
{
  if (with.Classes != nullptr)
  {
    for (std::size_t at = 0; at < with.Classes->size(); ++at)
    {
      if ((*with.Classes)[at].DxfName == name)
      {
        return static_cast<std::uint16_t>(FirstClass + at);
      }
    }
  }
  if (with.Issues != nullptr)
  {
    with.Issues->push_back(Issue{.Code = IssueCode::Damaged,
                                 .Offset = with.Offset,
                                 .Subject = with.Record,
                                 .What = std::format("class {} not declared", name)});
  }
  return 0;
}

/**
 *  Its fixed type, or its class when it has no fixed type, its shape says this object takes the class, or the release
 *  written is before the one its shape says gave it a type.
 */
template <typename KindT>
auto CodeOf(const KindT &kind, const Fields::Context &with) -> std::uint16_t
{
  if constexpr (requires { Shape<KindT>::Fixed; })
  {
    if (with.Release < Shape<KindT>::Fixed)
    {
      return ClassCode(NameOf(kind), with); // A class before the release that gave the kind its type code.
    }
  }
  if constexpr (Shape<KindT>::Types.empty())
  {
    return ClassCode(NameOf(kind), with);
  }
  else if constexpr (requires { Shape<KindT>::IsClassed(kind); })
  {
    return Shape<KindT>::IsClassed(kind) ? ClassCode(NameOf(kind), with) : Shape<KindT>::Types.front();
  }
  else if constexpr (requires { Shape<KindT>::Code(kind); })
  {
    return Shape<KindT>::Code(kind);
  }
  else
  {
    return Shape<KindT>::Types.front();
  }
}

/**
 *  The kinds a file stores under a class rather than a fixed type: those chapter 20.4 marks as varying, and the
 *  dictionary with a default.
 */
using Varying =
    std::tuple<Entities::LwPolyline, Entities::Hatch, Objects::Dictionary, Objects::Placeholder, Objects::DictionaryVar,
               Objects::XRecord, Objects::Scale, Objects::SortEntsTable, Objects::Layout, Entities::ArcDimension,
               Entities::LargeRadialDimension, Objects::PlotSettings, Entities::Image, Entities::Wipeout,
               Objects::ImageDef, Objects::ImageDefReactor, Objects::RasterVariables, Objects::WipeoutVariables,
               Entities::PdfUnderlay, Entities::DwfUnderlay, Entities::DgnUnderlay, Objects::PdfDefinition,
               Objects::DwfDefinition, Objects::DgnDefinition, Entities::MultiLeader, Objects::MLeaderStyle,
               Entities::AcadTable, Objects::TableStyle, Objects::DataTable, Entities::Helix, Entities::Mesh,
               Entities::Light, Entities::SectionObject, Objects::Material, Objects::VisualStyle, Entities::Surface,
               Entities::PlaneSurface, Entities::NurbSurface, Entities::ExtrudedSurface, Entities::LoftedSurface,
               Entities::RevolvedSurface, Entities::SweptSurface, Objects::TableContent, Objects::TableGeometry,
               Objects::CellStyleMap>;

/**
 *  False when no varying kind bears the class's DXF name.
 */
template <typename FunctionT>
auto VisitClass(std::string_view name, const FunctionT &function) -> bool
{
  const auto one = [&function, name]<typename KindT>(std::type_identity<KindT>) -> bool
  {
    if (!std::ranges::contains(ClassNames<KindT>(), name))
    {
      return false;
    }
    function(std::type_identity<KindT>{});
    return true;
  };
  return [&one]<typename... KindsT>(std::type_identity<std::tuple<KindsT...>>) -> bool
  {
    return (one(std::type_identity<KindsT>{}) || ...);
  }(std::type_identity<Varying>{});
}

/**
 *  The kinds with a fixed type code, in the order the reader tries them.
 */
template <typename... KindsT>
auto FixedKinds(std::type_identity<std::tuple<KindsT...>>) -> decltype(std::tuple_cat(
    std::conditional_t<Shape<KindsT>::Types.empty(), std::tuple<>, std::tuple<KindsT>>{}...));

using Fixed = decltype(FixedKinds(std::type_identity<Kinds>{}));

/**
 *  False when no kind is stored under that type code.
 */
template <typename FunctionT>
auto VisitType(std::uint16_t type, const FunctionT &function) -> bool
{
  const auto one = [&function, type]<typename KindT>(std::type_identity<KindT>) -> bool
  {
    if (!std::ranges::contains(Shape<KindT>::Types, type))
    {
      return false;
    }
    function(std::type_identity<KindT>{});
    return true;
  };
  return [&one]<typename... KindsT>(std::type_identity<std::tuple<KindsT...>>) -> bool
  {
    return (one(std::type_identity<KindsT>{}) || ...);
  }(std::type_identity<Fixed>{});
}
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_KINDS_HPP */
