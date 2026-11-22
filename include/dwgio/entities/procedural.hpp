/***********************************************************************************************************************
 *  @file      dwgio/entities/procedural.hpp
 *  @brief     What extruded, swept, revolved, lofted and NURBS surfaces keep after their isolines.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_PROCEDURAL_HPP
#define DWGIO_ENTITIES_PROCEDURAL_HPP

#include "dwgio/api.hpp"
#include "dwgio/matrix.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/vector.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  An entity a procedural surface was made from, a profile, path, cross section or guide, as the file holds it: the
 *  entity's type code and its data bits, which DXF writes as group 90 (the type), 90 (the bits) and 310.
 */
struct DWGIO_API Profile
{
  std::int32_t Type{}; /**< 19 for a LINE. */
  BitString Data{};

  auto operator==(const Profile &) const -> bool = default;
};

/**
 *  How a profile is swept along a direction or a path: the options an extruded and a swept surface share.
 */
struct DWGIO_API SweepOptions
{
  double Draft{};
  double StartDraft{};
  double EndDraft{};
  double Twist{};
  double Scale{1};
  double Align{};
  bool Solid{};
  int Alignment{}; /**< 0 none, 1 aligned, 2 translated to the path, 3 the path translated. Group 70. */
  int Miter{2};    /**< Unnamed in DXF's reference; 2 as AutoCAD writes it. Group 71.                 */
  bool AlignStart{};
  bool Bank{};
  bool BasePoint{};
  Vector Reference{}; /**< Controls the twist. Group 11. */
  bool ProfileComputed{};
  bool PathComputed{};
  Matrix ProfileTransform{Identity};
  Matrix PathTransform{Identity};

  auto operator==(const SweepOptions &) const -> bool = default;
};

/**
 *  What an EXTRUDEDSURFACE keeps after its isolines: its options, its direction, its transform and its profile.
 */
struct DWGIO_API Extrusion
{
  SweepOptions Options{};
  Vector Direction{}; /**< The sweep vector. Group 10. */
  Matrix Transform{Identity};
  Entities::Profile Profile{};

  auto operator==(const Extrusion &) const -> bool = default;
};

/**
 *  What a SWEPTSURFACE keeps after its isolines: its options, two transforms, its profile and its path.
 */
struct DWGIO_API Sweep
{
  SweepOptions Options{};
  Matrix ProfileTransform{Identity};
  Matrix PathTransform{Identity};
  Entities::Profile Profile{};
  Entities::Profile Path{};

  auto operator==(const Sweep &) const -> bool = default;
};

/**
 *  What a REVOLVEDSURFACE keeps after its isolines: its drafts, its axis, its angles, its transform and its profile.
 */
struct DWGIO_API Revolution
{
  double Draft{};
  double StartDraft{};
  double EndDraft{};
  double Twist{};
  bool Solid{};
  bool Close{};
  Vector Point{}; /**< A point of the axis. Group 10. */
  Vector Axis{Right};
  double Angle{Turn}; /**< The angle revolved through. Group 40. */
  double Start{};
  Matrix Transform{Identity};
  Entities::Profile Profile{};

  auto operator==(const Revolution &) const -> bool = default;
};

/**
 *  What a LOFTEDSURFACE keeps after its isolines: its transform, its options, and the entities it is lofted through.
 */
struct DWGIO_API Loft
{
  Matrix Transform{Identity};
  double StartDraft{};
  double EndDraft{};
  double StartMagnitude{};
  double EndMagnitude{};
  bool ArcLength{};
  bool NoTwist{};
  bool AlignDirection{};
  bool Simple{};
  bool Closed{};
  bool Solid{};
  bool Ruled{};
  bool VirtualGuide{};
  int Normal{}; /**< 0 none, the others the start, end or both normals. Group 70. */
  std::vector<Entities::Profile> Sections{};
  std::vector<Entities::Profile> Guides{}; /**< After the sections in the file; empty in AutoCAD's own lofts. */
  std::optional<Entities::Profile> Path{}; /**< After the guides in the file.                                 */

  auto operator==(const Loft &) const -> bool = default;
};

/**
 *  What a NURBSURFACE keeps after its isolines, from R2013: groups DXF writes and its reference does not name.
 */
struct DWGIO_API NurbSurfaceData
{
  int Hull{};                     /**< 0 as AutoCAD writes it. Group 170.                                   */
  bool Unknown1{};                /**< Unnamed in the specification; clear as AutoCAD writes it. Group 290. */
  std::array<Vector, 4> Points{}; /**< Zero as AutoCAD writes them. Groups 10 to 13.                        */

  auto operator==(const NurbSurfaceData &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_PROCEDURAL_HPP */
