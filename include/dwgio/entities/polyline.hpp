/***********************************************************************************************************************
 *  @file      dwgio/entities/polyline.hpp
 *  @brief     The POLYLINE entity: 2D and 3D polylines and polygon and polyface meshes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_POLYLINE_HPP
#define DWGIO_ENTITIES_POLYLINE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/flags.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  The bits of a POLYLINE's flags, group 70, to test with `Has`.
 */
enum class PolylineFlags : std::uint8_t
{
  Closed = 1,     /**< Closed; for a polygon mesh, closed along M. */
  CurveFit = 2,   /**< Curve-fit vertices added.                   */
  SplineFit = 4,  /**< Spline-fit vertices added.                  */
  Polyline3d = 8, /**< A 3D polyline.                              */
  PolygonMesh = 16,
  ClosedN = 32, /**< A polygon mesh closed along N. */
  PolyfaceMesh = 64,
  ContinuousLinetype = 128, /**< The linetype drawn continuously around the vertices. */
};

/**
 *  A POLYLINE: a 2D or 3D polyline, a polygon mesh or a polyface mesh, its vertices following it in the file up to a
 *  SEQEND.
 */
struct DWGIO_API Polyline
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  /**
   *  The bits of PolylineFlags. Group 70. The 8, 16 and 64 bits pick the type the file stores the polyline under.
   */
  int Flags{};
  /**
   *  How the polyline or surface is smoothed: 0 not at all, 5 quadratic B-spline, 6 cubic B-spline, 8 Bezier.
   *
   *  Group 75. A 3D polyline keeps 5, 6 or 0 alone, and its 4 bit of Flags follows it.
   */
  int Curve{};
  double StartWidth{};  /**< The width a 2D polyline's segments start with when their vertex gives none. Group 40. */
  double EndWidth{};    /**< The width a 2D polyline's segments end with when their vertex gives none. Group 41.   */
  double Thickness{};   /**< For a 2D polyline. Group 39.                                                          */
  double Elevation{};   /**< Of a 2D polyline: the Z of group 10.                                                  */
  Vector Extrusion{Up}; /**< For a 2D polyline.                                                                    */
  /**
   *  The vertex count along M of a polygon mesh, or the vertex count of a polyface mesh. Group 71.
   */
  int Rows{};
  /**
   *  The vertex count along N of a polygon mesh, or the face count of a polyface mesh. Group 72.
   */
  int Columns{};
  int Density{}; /**< Along M of a smoothed polygon mesh. Group 73. */
  int Depth{};   /**< Along N of a smoothed polygon mesh. Group 74. */
  Handle First{};
  Handle Last{};
  Handle SeqEnd{};
  std::vector<Handle> Vertices{}; /**< From R2004; First and Last before. */

  auto operator==(const Polyline &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_POLYLINE_HPP */
