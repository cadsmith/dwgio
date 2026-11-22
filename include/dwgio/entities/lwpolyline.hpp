/***********************************************************************************************************************
 *  @file      dwgio/entities/lwpolyline.hpp
 *  @brief     The LWPOLYLINE entity and its vertices.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_LWPOLYLINE_HPP
#define DWGIO_ENTITIES_LWPOLYLINE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/flags.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/widths.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  The bits of an LWPOLYLINE's flags, group 70, to test with `Has`.
 */
enum class LwPolylineFlags : std::uint8_t
{
  Closed = 1,
  ContinuousLinetype = 128, /**< The linetype drawn continuously around the vertices. */
};

struct DWGIO_API LwPolyline
{
  /**
   *  A vertex of a lightweight polyline, with the widths and bulge of the segment that starts at it.
   */
  struct Vertex
  {
    Vector Position{}; /**< In the polyline's own coordinates. */
    /**
     *  The tangent of a quarter of the arc's angle to the next vertex; 0 when straight. Group 42.
     */
    double Bulge{};
    std::optional<DwgIO::Widths> Widths{}; /**< When the polyline keeps a pair per vertex. Groups 40, 41.     */
    std::optional<std::int32_t> Id{};      /**< From R2010, when the polyline keeps one per vertex. Group 91. */

    auto operator==(const Vertex &) const -> bool = default;
  };

  DwgIO::Base Base{};
  DwgIO::Common Common{};
  int Flags{};                   /**< The bits of LwPolylineFlags. Group 70.                                 */
  std::optional<double> Width{}; /**< The width every segment shares, when the polyline keeps one. Group 43. */
  double Elevation{};            /**< Along the normal. Group 38.                                            */
  double Thickness{};
  Vector Extrusion{Up};
  /**
   *  The file keeps a bulge per vertex when any is not zero, and a width pair per vertex when any vertex has one.
   */
  std::vector<Vertex> Vertices{};

  auto operator==(const LwPolyline &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_LWPOLYLINE_HPP */
