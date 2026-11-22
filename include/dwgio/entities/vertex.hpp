/***********************************************************************************************************************
 *  @file      dwgio/entities/vertex.hpp
 *  @brief     The VERTEX entity: a point of a POLYLINE or a face of a polyface mesh.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_VERTEX_HPP
#define DWGIO_ENTITIES_VERTEX_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/flags.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>

namespace DwgIO::Entities
{
/**
 *  The bits of a VERTEX's flags, group 70, to test with `Has`.
 */
enum class VertexFlags : std::uint8_t
{
  CurveFitExtra = 1, /**< Added by curve fitting.      */
  Tangent = 2,       /**< Has a curve-fit tangent.     */
  SplineFit = 8,     /**< Added by spline fitting.     */
  ControlPoint = 16, /**< A point of the spline frame. */
  Polyline3d = 32,
  PolygonMesh = 64,   /**< Of a polygon mesh; with PolyfaceMesh, a polyface mesh's vertex. */
  PolyfaceMesh = 128, /**< Alone, a face of a polyface mesh.                               */
};

/**
 *  A VERTEX: a point of a POLYLINE, or a face of a polyface mesh.
 */
struct DWGIO_API Vertex
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  /**
   *  The bits of VertexFlags. Group 70. The 32, 64 and 128 bits pick the type the file stores the vertex under.
   */
  int Flags{};
  /**
   *  In the polyline's own coordinates for a 2D polyline, whose Z the file keeps as AutoCAD left it, the elevation
   *  being the polyline's; in world coordinates otherwise.
   */
  Vector Position{};
  double StartWidth{};
  double EndWidth{};
  /**
   *  The tangent of a quarter of the arc's angle to the next vertex; 0 when straight. Group 42.
   */
  double Bulge{};
  std::int32_t Id{}; /**< From R2010, for a 2D vertex. Group 91.                                           */
  double Tangent{};  /**< A 2D polyline's curve-fit tangent direction, in radians; group 50 gives degrees. */
  /**
   *  A polyface mesh face's vertex, counted from 1; negative when the edge from it is invisible. Group 71.
   */
  int First{};
  int Second{}; /**< Counted the same way. Group 72.                   */
  int Third{};  /**< Counted the same way. Group 73.                   */
  int Fourth{}; /**< Counted the same way; 0 for a triangle. Group 74. */

  auto operator==(const Vertex &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_VERTEX_HPP */
