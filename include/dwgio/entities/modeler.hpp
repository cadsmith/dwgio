/***********************************************************************************************************************
 *  @file      dwgio/entities/modeler.hpp
 *  @brief     The REGION, 3DSOLID and BODY entities and the seven kinds of surface.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_MODELER_HPP
#define DWGIO_ENTITIES_MODELER_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/entities/geometry.hpp"
#include "dwgio/entities/procedural.hpp"
#include "dwgio/handle.hpp"

#include <cstdint>
#include <variant>

namespace DwgIO::Entities
{
/**
 *  A REGION: a closed flat area, kept as an ACIS body.
 */
struct DWGIO_API Region
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Geometry Geometry{};

  auto operator==(const Region &) const -> bool = default;
};

/**
 *  A BODY: an ACIS body that is neither a solid nor a region.
 */
struct DWGIO_API Body
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Geometry Geometry{};

  auto operator==(const Body &) const -> bool = default;
};

struct DWGIO_API Solid3d
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Geometry Geometry{};
  Handle History{}; /**< From R2007, an ACSH_HISTORY_CLASS object. Group 350. */

  auto operator==(const Solid3d &) const -> bool = default;
};

/**
 *  The kinds of surface entity.
 */
enum class SurfaceKind : std::uint8_t
{
  Generic, /**< A SURFACE, which is how a NURBS surface is stored up to R2010. */
  Planar,
  Nurbs, /**< A NURBSURFACE, from R2013. */
  Extruded,
  Lofted,
  Revolved,
  Swept,
};

/**
 *  What a surface keeps after its isolines: nothing for a plain or planar surface.
 */
template <SurfaceKind KindV>
struct SurfaceDataOf
{
  using Type = std::monostate;
};

template <>
struct SurfaceDataOf<SurfaceKind::Nurbs>
{
  using Type = NurbSurfaceData;
};

template <>
struct SurfaceDataOf<SurfaceKind::Extruded>
{
  using Type = Extrusion;
};

template <>
struct SurfaceDataOf<SurfaceKind::Lofted>
{
  using Type = Loft;
};

template <>
struct SurfaceDataOf<SurfaceKind::Revolved>
{
  using Type = Revolution;
};

template <>
struct SurfaceDataOf<SurfaceKind::Swept>
{
  using Type = Sweep;
};

/**
 *  A surface entity of the given kind: its ACIS body, its isolines, and the kind's own data, which the specification
 *  does not describe.
 */
template <SurfaceKind KindV>
struct SurfaceOf
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Geometry Geometry{};
  int UIsolines{6};                  /**< Isolines along U. Group 71.       */
  int VIsolines{6};                  /**< Isolines along V. Group 72.       */
  SurfaceDataOf<KindV>::Type Data{}; /**< DXF writes it after the isolines. */

  auto operator==(const SurfaceOf &) const -> bool = default;
};

using Surface = SurfaceOf<SurfaceKind::Generic>;
using PlaneSurface = SurfaceOf<SurfaceKind::Planar>;
using NurbSurface = SurfaceOf<SurfaceKind::Nurbs>;
using ExtrudedSurface = SurfaceOf<SurfaceKind::Extruded>;
using LoftedSurface = SurfaceOf<SurfaceKind::Lofted>;
using RevolvedSurface = SurfaceOf<SurfaceKind::Revolved>;
using SweptSurface = SurfaceOf<SurfaceKind::Swept>;
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_MODELER_HPP */
