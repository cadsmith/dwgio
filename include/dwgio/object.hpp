/***********************************************************************************************************************
 *  @file      dwgio/object.hpp
 *  @brief     Any object of a drawing, and access to it through its kind.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECT_HPP
#define DWGIO_OBJECT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/boxed.hpp"
#include "dwgio/common.hpp"
#include "dwgio/entities/arc.hpp"
#include "dwgio/entities/attdef.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/circle.hpp"
#include "dwgio/entities/dimension.hpp"
#include "dwgio/entities/ellipse.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/face.hpp"
#include "dwgio/entities/hatch.hpp"
#include "dwgio/entities/helix.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/leader.hpp"
#include "dwgio/entities/light.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/lwpolyline.hpp"
#include "dwgio/entities/mesh.hpp"
#include "dwgio/entities/mline.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/entities/ole2frame.hpp"
#include "dwgio/entities/point.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/raster.hpp"
#include "dwgio/entities/ray.hpp"
#include "dwgio/entities/section.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/shape.hpp"
#include "dwgio/entities/solid.hpp"
#include "dwgio/entities/spline.hpp"
#include "dwgio/entities/table.hpp"
#include "dwgio/entities/text.hpp"
#include "dwgio/entities/tolerance.hpp"
#include "dwgio/entities/trace.hpp"
#include "dwgio/entities/underlay.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/entities/xline.hpp"
#include "dwgio/objects/cellstylemap.hpp"
#include "dwgio/objects/datatable.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/dictionaryvar.hpp"
#include "dwgio/objects/group.hpp"
#include "dwgio/objects/image.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/material.hpp"
#include "dwgio/objects/mleaderstyle.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/placeholder.hpp"
#include "dwgio/objects/plotsettings.hpp"
#include "dwgio/objects/scale.hpp"
#include "dwgio/objects/sortents.hpp"
#include "dwgio/objects/tablecontent.hpp"
#include "dwgio/objects/tablegeometry.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/objects/underlay.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/objects/wipeout.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/ucs.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tables/viewportheader.hpp"
#include "dwgio/tables/vport.hpp"
#include "dwgio/unknown.hpp"

#include <string_view>
#include <utility>
#include <variant>

namespace DwgIO
{
/**
 *  Any object of a drawing: one of the typed kinds, or Unknown for the rest, each in a box of its own so an object
 *  takes the size of its own kind.
 *
 *  Read it with Holds, Get, GetIf and Visit, which see the kind through the box.
 */
using Object = std::variant<
    Boxed<Unknown>, Boxed<Tables::AppId>, Boxed<Tables::Control>, Boxed<Tables::Layer>, Boxed<Tables::Style>,
    Boxed<Tables::LType>, Boxed<Tables::View>, Boxed<Tables::Ucs>, Boxed<Tables::VPort>, Boxed<Tables::DimStyle>,
    Boxed<Tables::BlockRecord>, Boxed<Entities::Line>, Boxed<Entities::Point>, Boxed<Entities::Circle>,
    Boxed<Entities::Arc>, Boxed<Entities::Ellipse>, Boxed<Entities::Ray>, Boxed<Entities::XLine>,
    Boxed<Entities::Face3d>, Boxed<Entities::Solid>, Boxed<Entities::Trace>, Boxed<Entities::Shape>,
    Boxed<Entities::Text>, Boxed<Entities::AttDef>, Boxed<Entities::Attrib>, Boxed<Entities::Block>,
    Boxed<Entities::EndBlk>, Boxed<Entities::SeqEnd>, Boxed<Entities::Insert>, Boxed<Entities::MText>,
    Boxed<Entities::Polyline>, Boxed<Entities::Vertex>, Boxed<Entities::LwPolyline>, Boxed<Entities::Spline>,
    Boxed<Entities::Hatch>, Boxed<Entities::Dimension>, Boxed<Entities::Leader>, Boxed<Entities::Tolerance>,
    Boxed<Entities::MLine>, Boxed<Entities::Viewport>, Boxed<Objects::Dictionary>, Boxed<Objects::Placeholder>,
    Boxed<Objects::DictionaryVar>, Boxed<Objects::XRecord>, Boxed<Objects::Group>, Boxed<Objects::MLineStyle>,
    Boxed<Objects::Scale>, Boxed<Objects::SortEntsTable>, Boxed<Objects::Layout>, Boxed<Tables::ViewportEntityHeader>,
    Boxed<Entities::ArcDimension>, Boxed<Entities::LargeRadialDimension>, Boxed<Objects::PlotSettings>,
    Boxed<Entities::Image>, Boxed<Entities::Wipeout>, Boxed<Objects::ImageDef>, Boxed<Objects::ImageDefReactor>,
    Boxed<Objects::RasterVariables>, Boxed<Objects::WipeoutVariables>, Boxed<Entities::PdfUnderlay>,
    Boxed<Entities::DwfUnderlay>, Boxed<Entities::DgnUnderlay>, Boxed<Objects::PdfDefinition>,
    Boxed<Objects::DwfDefinition>, Boxed<Objects::DgnDefinition>, Boxed<Entities::Ole2Frame>,
    Boxed<Entities::MultiLeader>, Boxed<Objects::MLeaderStyle>, Boxed<Entities::AcadTable>, Boxed<Objects::TableStyle>,
    Boxed<Objects::DataTable>, Boxed<Entities::Helix>, Boxed<Entities::Mesh>, Boxed<Entities::Light>,
    Boxed<Entities::SectionObject>, Boxed<Objects::Material>, Boxed<Objects::VisualStyle>, Boxed<Entities::Region>,
    Boxed<Entities::Solid3d>, Boxed<Entities::Body>, Boxed<Entities::Surface>, Boxed<Entities::PlaneSurface>,
    Boxed<Entities::NurbSurface>, Boxed<Entities::ExtrudedSurface>, Boxed<Entities::LoftedSurface>,
    Boxed<Entities::RevolvedSurface>, Boxed<Entities::SweptSurface>, Boxed<Objects::TableContent>,
    Boxed<Objects::TableGeometry>, Boxed<Objects::CellStyleMap>>;

template <class KindT>
[[nodiscard]] auto Holds(const Object &object) -> bool
{
  return std::holds_alternative<Boxed<KindT>>(object);
}

/**
 *  Throws std::bad_variant_access when the object is of another kind.
 */
template <class KindT>
[[nodiscard]] auto Get(Object &object) -> KindT &
{
  return *std::get<Boxed<KindT>>(object);
}

/**
 *  Throws std::bad_variant_access when the object is of another kind.
 */
template <class KindT>
[[nodiscard]] auto Get(const Object &object) -> const KindT &
{
  return *std::get<Boxed<KindT>>(object);
}

/**
 *  @return Null when the object is of another kind or there is none.
 */
template <class KindT>
[[nodiscard]] auto GetIf(Object *object) -> KindT *
{
  auto *box = std::get_if<Boxed<KindT>>(object);
  return box == nullptr ? nullptr : box->operator->();
}

/**
 *  @return Null when the object is of another kind or there is none.
 */
template <class KindT>
[[nodiscard]] auto GetIf(const Object *object) -> const KindT *
{
  const auto *box = std::get_if<Boxed<KindT>>(object);
  return box == nullptr ? nullptr : box->operator->();
}

/**
 *  As std::visit would, were the kind not boxed.
 */
template <class FunctionT>
auto Visit(FunctionT &&function, Object &object) -> decltype(auto)
{
  return std::visit([&function](auto &box) -> decltype(auto) { return std::forward<FunctionT>(function)(*box); },
                    object);
}

template <class FunctionT>
auto Visit(FunctionT &&function, const Object &object) -> decltype(auto)
{
  return std::visit([&function](const auto &box) -> decltype(auto) { return std::forward<FunctionT>(function)(*box); },
                    object);
}

/**
 *  The fixed name, such as "LINE", or the class's DXF name.
 */
[[nodiscard]] DWGIO_API auto TypeOf(const Object &object) -> std::string_view;

[[nodiscard]] DWGIO_API auto BaseOf(const Object &object) -> const Base &;

[[nodiscard]] DWGIO_API auto BaseOf(Object &object) -> Base &;

/**
 *  @return Null for an object that is no entity; untyped entities included.
 */
[[nodiscard]] DWGIO_API auto CommonOf(const Object &object) -> const Common *;

/**
 *  @return Null for an object that is no entity.
 */
[[nodiscard]] DWGIO_API auto CommonOf(Object &object) -> Common *;
} // namespace DwgIO

#endif /* DWGIO_OBJECT_HPP */
