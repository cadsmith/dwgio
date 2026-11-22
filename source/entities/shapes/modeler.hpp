/***********************************************************************************************************************
 *  @file      source/entities/shapes/modeler.hpp
 *  @brief     How the ACIS entities lie in the file: regions, solids, bodies and surfaces.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_MODELER_HPP
#define DWGIO_ENTITIES_SHAPES_MODELER_HPP

#include "entities/shapes/geometry.hpp"
#include "entities/shapes/procedural.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/modeler.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  A solid's history is a hard owner.
 */
constexpr int Owner = 3;

template <typename KindT>
auto GeometryField()
{
  return Fields::Part(&KindT::Geometry, Fields::Layout(Fields::Custom(ReadGeometry, WriteGeometry)));
}

/**
 *  Each surface kind's DXF name, class, and what follows its isolines.
 */
template <SurfaceKind KindV>
struct Sheeted;

template <>
struct Sheeted<SurfaceKind::Generic>
{
  static constexpr std::string_view Name = "SURFACE";

  static auto Layout() { return Fields::Layout(); }
};

template <>
struct Sheeted<SurfaceKind::Planar>
{
  static constexpr std::string_view Name = "PLANESURFACE";

  static auto Layout() { return Fields::Layout(); }
};

template <>
struct Sheeted<SurfaceKind::Nurbs>
{
  static constexpr std::string_view Name = "NURBSURFACE";

  static auto Layout() { return Fields::Layout(Fields::Since(Version::R2013, Fields::Custom(ReadNurbs, WriteNurbs))); }
};

template <>
struct Sheeted<SurfaceKind::Extruded>
{
  static constexpr std::string_view Name = "EXTRUDEDSURFACE";

  static auto Layout() { return ExtrusionLayout(); }
};

template <>
struct Sheeted<SurfaceKind::Lofted>
{
  static constexpr std::string_view Name = "LOFTEDSURFACE";

  static auto Layout() { return Fields::Layout(Fields::Custom(ReadLoft, WriteLoft)); }
};

template <>
struct Sheeted<SurfaceKind::Revolved>
{
  static constexpr std::string_view Name = "REVOLVEDSURFACE";

  static auto Layout() { return RevolutionLayout(); }
};

template <>
struct Sheeted<SurfaceKind::Swept>
{
  static constexpr std::string_view Name = "SWEPTSURFACE";

  static auto Layout() { return SweepLayout(); }
};

/**
 *  AutoCAD stores a NURBS surface as SURFACE up to R2010 and as NURBSURFACE from R2013.
 */
template <SurfaceKind KindV>
auto SheetClass(Version release) -> std::string_view
{
  if constexpr (KindV == SurfaceKind::Generic || KindV == SurfaceKind::Nurbs)
  {
    return release >= Version::R2013 ? Sheeted<SurfaceKind::Nurbs>::Name : Sheeted<SurfaceKind::Generic>::Name;
  }
  else
  {
    static_cast<void>(release);
    return Sheeted<KindV>::Name;
  }
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Region>
{
  static constexpr std::array<std::uint16_t, 1> Types{37};
  static constexpr std::string_view Name = "REGION";

  static auto IsReadable(const Entities::Region &region) -> bool { return Entities::Shapes::IsWhole(region.Geometry); }

  static auto Layout() { return Fields::Layout(Entities::Shapes::GeometryField<Entities::Region>()); }
};

/**
 *  Chapter 20.4.41; from R2007 the history in the handles.
 */
template <>
struct Shape<Entities::Solid3d>
{
  static constexpr std::array<std::uint16_t, 1> Types{38};
  static constexpr std::string_view Name = "3DSOLID";

  static auto IsReadable(const Entities::Solid3d &solid) -> bool { return Entities::Shapes::IsWhole(solid.Geometry); }

  static auto Layout()
  {
    using Entities::Solid3d;
    return Fields::Layout(Entities::Shapes::GeometryField<Solid3d>(),
                          Fields::Since(Version::R2007, Fields::Link(&Solid3d::History, Entities::Shapes::Owner)));
  }
};

template <>
struct Shape<Entities::Body>
{
  static constexpr std::array<std::uint16_t, 1> Types{39};
  static constexpr std::string_view Name = "BODY";

  static auto IsReadable(const Entities::Body &body) -> bool { return Entities::Shapes::IsWhole(body.Geometry); }

  static auto Layout() { return Fields::Layout(Entities::Shapes::GeometryField<Entities::Body>()); }
};

/**
 *  Chapter 20.4.41's geometry, the two isoline counts as BS and the kind's own data; stored under its class.
 */
template <Entities::SurfaceKind KindV>
struct Shape<Entities::SurfaceOf<KindV>>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = Entities::Shapes::Sheeted<KindV>::Name;

  static auto IsReadable(const Entities::SurfaceOf<KindV> &surface) -> bool
  {
    return Entities::Shapes::IsWhole(surface.Geometry);
  }

  /**
   *  A save to the release declares the class.
   */
  static auto ClassAt(Version release) -> std::string_view { return Entities::Shapes::SheetClass<KindV>(release); }

  static auto Layout()
  {
    using Surface = Entities::SurfaceOf<KindV>;
    return Fields::Layout(Entities::Shapes::GeometryField<Surface>(), Fields::Short(&Surface::UIsolines),
                          Fields::Short(&Surface::VIsolines),
                          Fields::Part(&Surface::Data, Entities::Shapes::Sheeted<KindV>::Layout()));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_MODELER_HPP */
