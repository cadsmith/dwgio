/***********************************************************************************************************************
 *  @file      dwgio/entities/mesh.hpp
 *  @brief     The MESH entity: a subdivision surface.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_MESH_HPP
#define DWGIO_ENTITIES_MESH_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  A MESH: a subdivision surface, stored under its class.
 */
struct DWGIO_API Mesh
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  int Version{2}; /**< 2 as AutoCAD writes it. */
  bool Blend{};
  std::int32_t Level{};
  std::vector<Vector> Vertices{};
  std::vector<int> Faces{};      /**< Each face is its vertex count, then the index of each vertex.      */
  std::vector<int> Edges{};      /**< Pairs of vertex indices; an index without its pair is not written. */
  std::vector<double> Creases{}; /**< One per edge.                                                      */
  std::int32_t Overridden{};     /**< How many parts of the mesh have their properties overridden.       */
  /**
   *  The overrides that follow when Overridden is not zero, kept as stored since their layout is not known. A save to
   *  another release leaves out a mesh that has them.
   */
  Opaque Overrides{};

  auto operator==(const Mesh &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_MESH_HPP */
