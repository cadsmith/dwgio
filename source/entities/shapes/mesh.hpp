/***********************************************************************************************************************
 *  @file      source/entities/shapes/mesh.hpp
 *  @brief     How a MESH lies in the file, its overrides kept as bits.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_MESH_HPP
#define DWGIO_ENTITIES_SHAPES_MESH_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/field.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/opaque.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/mesh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  A BL count of pairs, then each pair's two vertex indices as BL. A count beyond what is left stops the read.
 */
inline auto ReadEdges(Mesh &mesh, const Fields::Input &in) -> bool
{
  const std::int32_t pairs = in.In->BL();
  mesh.Edges.clear();
  if (in.In->HasFailed() || pairs < 0 || static_cast<std::size_t>(pairs) > Fields::Room(in))
  {
    in.In->Seek(in.In->Size() + 1);
    return false;
  }
  mesh.Edges.resize(static_cast<std::size_t>(pairs) * 2);
  for (int &index : mesh.Edges)
  {
    index = in.In->BL();
  }
  return !in.In->HasFailed();
}

inline auto WriteEdges(const Mesh &mesh, const Fields::Output &to) -> bool
{
  const std::size_t pairs = mesh.Edges.size() / 2;
  if (mesh.Edges.size() % 2 != 0)
  {
    Fields::Note(to, IssueCode::NotKept,
                 std::format("mesh edge index {} without its pair not written", mesh.Edges.back()));
  }
  to.To->BL(static_cast<std::int32_t>(pairs));
  for (std::size_t at = 0; at < pairs * 2; ++at)
  {
    to.To->BL(mesh.Edges[at]);
  }
  return true;
}

inline auto IsOverridden(const Mesh &mesh) -> bool
{
  return mesh.Overridden != 0;
}

inline auto ReadOverrides(Mesh &mesh, const Fields::Input &in) -> bool
{
  return Fields::ReadOpaque(mesh.Overrides, in);
}

inline auto WriteOverrides(const Mesh &mesh, const Fields::Output &to) -> bool
{
  return Fields::WriteOpaque(mesh.Overrides, to);
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
/**
 *  No chapter; the same in every release and stored under its class. The overrides are kept as bits.
 */
template <>
struct Shape<Entities::Mesh>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "MESH";

  static auto Layout()
  {
    using Entities::Mesh;
    namespace Shapes = Entities::Shapes;
    return Fields::Layout(
        Fields::Short(&Mesh::Version), Fields::Bit(&Mesh::Blend), Fields::Long(&Mesh::Level),
        Fields::List(&Mesh::Vertices, Fields::Counted::Long, Fields::Point3()),
        Fields::List(&Mesh::Faces, Fields::Counted::Long, Fields::Self<int, Fields::LongCode>{}),
        Fields::Custom(Shapes::ReadEdges, Shapes::WriteEdges),
        Fields::List(&Mesh::Creases, Fields::Counted::Long, Fields::Self<double, Fields::DoubleCode>{}),
        Fields::Long(&Mesh::Overridden),
        Fields::When(Shapes::IsOverridden, Fields::Custom(Shapes::ReadOverrides, Shapes::WriteOverrides)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_MESH_HPP */
