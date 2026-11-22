/***********************************************************************************************************************
 *  @file      source/entities/shapes/vertex.hpp
 *  @brief     How each kind of VERTEX lies in the file, its flags picking its type.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_VERTEX_HPP
#define DWGIO_ENTITIES_SHAPES_VERTEX_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/vertex.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  The flag bits that pick a vertex's type.
 */
namespace Vert
{
constexpr unsigned Vertex3d = 32;
constexpr unsigned Mesh = 64;
constexpr unsigned Polyface = 128;
constexpr unsigned Typed = Vertex3d | Mesh | Polyface;
} // namespace Vert

inline auto IsFace(const Vertex &vertex) -> bool
{
  return (static_cast<unsigned>(vertex.Flags) & (Vert::Polyface | Vert::Mesh)) == Vert::Polyface;
}

inline auto IsPoint(const Vertex &vertex) -> bool
{
  return !IsFace(vertex);
}

inline auto IsPlanar(const Vertex &vertex) -> bool
{
  return (static_cast<unsigned>(vertex.Flags) & (Vert::Vertex3d | Vert::Mesh | Vert::Polyface)) == 0;
}

/**
 *  A file whose flags byte disagrees with its type reads as its type says.
 */
inline auto ReadVertexFlags(Vertex &vertex, const Fields::Input &in) -> bool
{
  const auto typed = static_cast<unsigned>(vertex.Flags) & Vert::Typed;
  vertex.Flags = static_cast<int>(typed | (in.In->RC() & ~Vert::Typed));
  return !in.In->HasFailed();
}

inline auto WriteVertexFlags(const Vertex &vertex, const Fields::Output &to) -> bool
{
  to.To->RC(static_cast<std::uint8_t>(vertex.Flags));
  return true;
}

/**
 *  One negative value stands for both widths when they are equal and not zero.
 */
inline auto ReadWidths(Vertex &vertex, const Fields::Input &in) -> bool
{
  vertex.StartWidth = in.In->BD();
  if (vertex.StartWidth < 0)
  {
    vertex.StartWidth = -vertex.StartWidth;
    vertex.EndWidth = vertex.StartWidth;
  }
  else
  {
    vertex.EndWidth = in.In->BD();
  }
  return !in.In->HasFailed();
}

inline auto WriteWidths(const Vertex &vertex, const Fields::Output &to) -> bool
{
  if (vertex.StartWidth == vertex.EndWidth && vertex.StartWidth != 0)
  {
    to.To->BD(-vertex.StartWidth);
    return true;
  }
  to.To->BD(vertex.StartWidth);
  to.To->BD(vertex.EndWidth);
  return true;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Vertex>
{
  static constexpr std::array<std::uint16_t, 5> Types{10, 11, 12, 13, 14};
  static constexpr std::string_view Name = "VERTEX_2D";

  /**
   *  The type's bits, which the flags byte then joins: 32 for a 3D polyline's vertex, 64 for a mesh's, 192 for a
   *  polyface mesh's, 128 for one of its faces, which stores no flags.
   */
  static void Open(Entities::Vertex &vertex, std::uint16_t type)
  {
    using namespace Entities::Shapes;
    static constexpr std::array<unsigned, 5> Bits{0, Vert::Vertex3d, Vert::Mesh, Vert::Mesh | Vert::Polyface,
                                                  Vert::Polyface};
    for (std::size_t at = 0; at < Types.size(); ++at)
    {
      if (Types.at(at) == type)
      {
        vertex.Flags = static_cast<int>(Bits.at(at));
      }
    }
  }

  static auto Code(const Entities::Vertex &vertex) -> std::uint16_t
  {
    using namespace Entities::Shapes;
    const auto flags = static_cast<unsigned>(vertex.Flags);
    if ((flags & Vert::Polyface) != 0)
    {
      return (flags & Vert::Mesh) != 0 ? 13 : 14;
    }
    if ((flags & Vert::Mesh) != 0)
    {
      return 12;
    }
    return (flags & Vert::Vertex3d) != 0 ? 11 : 10;
  }

  static auto NameOf(const Entities::Vertex &vertex) -> std::string_view
  {
    static constexpr std::array<std::string_view, 5> Names{"VERTEX_2D", "VERTEX_3D", "VERTEX_MESH", "VERTEX_PFACE",
                                                           "VERTEX_PFACE_FACE"};
    return Names.at(static_cast<std::size_t>(Code(vertex) - Types.front()));
  }

  static auto Layout()
  {
    using Entities::Vertex;
    using namespace Entities::Shapes;
    return Fields::Layout(
        Fields::When(IsFace, Fields::Short(&Vertex::First), Fields::Short(&Vertex::Second),
                     Fields::Short(&Vertex::Third), Fields::Short(&Vertex::Fourth)),
        Fields::When(IsPoint, Fields::Custom(ReadVertexFlags, WriteVertexFlags), Fields::Point3(&Vertex::Position)),
        Fields::When(IsPlanar, Fields::Custom(ReadWidths, WriteWidths), Fields::Double(&Vertex::Bulge),
                     Fields::Since(Version::R2010, Fields::Long(&Vertex::Id)), Fields::Double(&Vertex::Tangent)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_VERTEX_HPP */
