/***********************************************************************************************************************
 *  @file      dwgio/entities/geometry.hpp
 *  @brief     The ACIS body and wireframe that regions, solids, bodies and surfaces share.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_GEOMETRY_HPP
#define DWGIO_ENTITIES_GEOMETRY_HPP

#include "dwgio/api.hpp"
#include "dwgio/vector.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  How a wire of an ACIS entity's wireframe is placed.
 */
struct DWGIO_API Transform
{
  Vector XAxis{Right};
  Vector YAxis{.X = 0, .Y = 1, .Z = 0};
  Vector ZAxis{Up};
  Vector Translation{};
  double Scale{1};
  bool Rotation{};
  bool Reflection{};
  bool Shear{};

  auto operator==(const Transform &) const -> bool = default;
};

/**
 *  One wire of an ACIS entity's wireframe or silhouette: a polyline AutoCAD draws the body with.
 */
struct DWGIO_API Wire
{
  int Type{};             /**< 1 a line, 2 or 3 a curve, as AutoCAD writes it.           */
  std::int32_t Marker{};  /**< The selection marker, negative as AutoCAD writes it.      */
  int Color{256};         /**< 256 for ByLayer.                                          */
  std::int32_t Index{-1}; /**< The ACIS entity the wire traces; -1 as AutoCAD writes it. */
  std::vector<Vector> Points{};
  std::optional<Entities::Transform> Transform{}; /**< When the file gives it. */

  auto operator==(const Wire &) const -> bool = default;
};

/**
 *  The outline of an ACIS entity as seen from one viewport.
 */
struct DWGIO_API Silhouette
{
  std::int32_t Viewport{}; /**< The viewport's id. */
  Vector Target{};
  Vector Direction{Up}; /**< From the target. */
  Vector Upward{.X = 0, .Y = 1, .Z = 0};
  bool Perspective{};
  std::vector<Wire> Wires{};

  auto operator==(const Silhouette &) const -> bool = default;
};

/**
 *  An ACIS stream as an object holds it: SAT text, SAB binary, or none.
 */
struct DWGIO_API Acis
{
  /**
   *  The format: 1 SAT text, 2 SAB binary, 0 none, the empty bit set. DXF writes SAT, group 70 at 1, in either case.
   */
  int Version{};
  bool Textual{}; /**< Set with SAT and clear with SAB, as AutoCAD writes it; the specification leaves it unnamed. */
  /**
   *  For version 1 the SAT text decrypted, its lines ended by newlines, which DXF's groups 1 and 3 carry one line each,
   *  encrypted, a tab as a space, its second header line without the date.
   *
   *  For version 2 the SAB bytes, from `ACIS BinaryFile` through the end marker.
   */
  std::vector<std::byte> Data{};

  auto operator==(const Acis &) const -> bool = default;
};

/**
 *  The AcDbModelerGeometry part of a REGION, 3DSOLID, BODY or surface: its ACIS body and the wires AutoCAD draws it
 *  with.
 *
 *  From R2013 AutoCAD keeps the body in the data store, which the drawing keeps as bytes.
 */
struct DWGIO_API Geometry
{
  /**
   *  The body, before the wires, up to R2010; from R2013 the object has no place for it.
   */
  Entities::Acis Body{};
  bool Framed{true};             /**< Whether the wireframe data follows: the point, the isolines and the wires. */
  std::optional<Vector> Point{}; /**< When the file gives one.                                                   */
  std::int32_t Isolines{};
  bool Wired{}; /**< Whether the wires are present, even none. */
  std::vector<Wire> Wires{};
  std::vector<Silhouette> Silhouettes{}; /**< By viewport. */
  /**
   *  A second ACIS stream after the wires, which the specification has follow a second empty bit: AutoCAD writes one
   *  for a NURBS surface from R2013, a body for each isoline, and keeps it in an R2000 save of one.
   */
  Entities::Acis Second{};
  std::int32_t Unknown1{}; /**< Unnamed in the specification; 0 as AutoCAD writes it. From R2007. */
  bool Stored{};           /**< From R2013. Group 290.                                            */
  /**
   *  From R2013, the body's GUID, as DXF writes it: `{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}`. Group 2.
   */
  std::string Guid{};
  std::int32_t Unknown2{}; /**< Unnamed in the specification; 0 as AutoCAD writes it. From R2013. */

  auto operator==(const Geometry &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_GEOMETRY_HPP */
