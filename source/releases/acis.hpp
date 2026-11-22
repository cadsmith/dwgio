/***********************************************************************************************************************
 *  @file      source/releases/acis.hpp
 *  @brief     The ACIS entities as the release a save writes holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_ACIS_HPP
#define DWGIO_RELEASES_ACIS_HPP

#include "releases/object.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/geometry.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <optional>
#include <string>

namespace DwgIO::Releases
{
/**
 *  The ACIS entity of a translation, from the drawing's release, as the release holds it: its body as AutoCAD writes it
 *  there, SAT before R2004, SAB from it and none in the object from R2013, where the data store holds it; the
 *  geometry's longs and data store fields of R2007 and R2013 dropped when downgrading below them and set as AutoCAD
 *  writes them when upgrading, a NURBS surface's second stream dropped across R2013, a solid's history dropped below
 *  R2007, an extruded or swept surface and the history node of an extrusion or a sweep given ACAD's saved-by-version
 *  extended data below R2007 and none from it, a NURBS surface a SURFACE below R2013 and a SURFACE a NURBS surface from
 *  it, as AutoCAD saves them. Any other object stays as it is.
 */
void ConvertAcis(Translation &translation, const Drawing &drawing, Version to);

/**
 *  Why an object of a release cannot be written as another, or nothing when it can: an ACIS entity whose body holds a
 *  record or value AutoCAD's files never hold, whose form in another release is unknown, or whose body is in a data
 *  store the drawing could not give it from.
 */
[[nodiscard]] auto FindStranding(const DwgIO::Object &object, Version from, Version to) -> std::optional<std::string>;

/**
 *  The ACIS geometry of a region, solid, body or surface, or null for any other object.
 */
[[nodiscard]] auto GeometryOf(DwgIO::Object &object) -> Entities::Geometry *;
[[nodiscard]] auto GeometryOf(const DwgIO::Object &object) -> const Entities::Geometry *;

/**
 *  Whether AutoCAD remakes an entity's body when it reads it from an older release: a procedural surface's, an
 *  extruded, revolved, lofted or swept one.
 */
[[nodiscard]] auto IsProcedural(const DwgIO::Object &object) -> bool;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_ACIS_HPP */
