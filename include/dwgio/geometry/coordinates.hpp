/***********************************************************************************************************************
 *  @file      dwgio/geometry/coordinates.hpp
 *  @brief     Points between an entity's object coordinate system and the world.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_GEOMETRY_COORDINATES_HPP
#define DWGIO_GEOMETRY_COORDINATES_HPP

#include "dwgio/api.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Geometry
{
/**
 *  A point of the object coordinate system an extrusion sets, in world coordinates.
 *
 *  The axes follow the DXF reference's arbitrary axis algorithm: when the normal's X and Y are both under 1/64, the X
 *  axis is the world Y axis crossed with the normal, otherwise the world Z axis crossed with it; Y is the normal
 *  crossed with X. A zero extrusion counts as the world Z axis.
 */
[[nodiscard]] DWGIO_API auto ToWorld(Vector point, Vector extrusion) -> Vector;

/**
 *  A point in world coordinates, in the object coordinate system an extrusion sets: ToWorld undone.
 */
[[nodiscard]] DWGIO_API auto ToObject(Vector point, Vector extrusion) -> Vector;
} // namespace DwgIO::Geometry

#endif /* DWGIO_GEOMETRY_COORDINATES_HPP */
