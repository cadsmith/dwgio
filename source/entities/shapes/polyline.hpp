/***********************************************************************************************************************
 *  @file      source/entities/shapes/polyline.hpp
 *  @brief     How the 2D, 3D, mesh and polyface polylines lie in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_POLYLINE_HPP
#define DWGIO_ENTITIES_SHAPES_POLYLINE_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/polyline.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  The flag bits that pick a polyline's type, and the curve types of a spline fit.
 */
namespace Poly
{
constexpr unsigned Closed = 1;
constexpr unsigned Splined = 4;
constexpr unsigned Polyline3d = 8;
constexpr unsigned Mesh = 16;
constexpr unsigned Polyface = 64;
constexpr unsigned Typed = Polyline3d | Mesh | Polyface;
constexpr int Quadratic = 5;
constexpr int Cubic = 6;
} // namespace Poly

inline auto Is2d(const Polyline &polyline) -> bool
{
  return (static_cast<unsigned>(polyline.Flags) & (Poly::Polyline3d | Poly::Mesh | Poly::Polyface)) == 0;
}

inline auto Is3d(const Polyline &polyline) -> bool
{
  return (static_cast<unsigned>(polyline.Flags) & Poly::Polyline3d) != 0;
}

inline auto IsMesh(const Polyline &polyline) -> bool
{
  return (static_cast<unsigned>(polyline.Flags) & Poly::Mesh) != 0;
}

inline auto IsPolyface(const Polyline &polyline) -> bool
{
  return (static_cast<unsigned>(polyline.Flags) & Poly::Polyface) != 0;
}

/**
 *  A 2D polyline's or a mesh's flags; a file whose flags disagree with its type reads as its type says.
 */
inline auto ReadPolylineFlags(Polyline &polyline, const Fields::Input &in) -> bool
{
  const auto typed = static_cast<unsigned>(polyline.Flags) & Poly::Typed;
  polyline.Flags = static_cast<int>(typed | (static_cast<unsigned>(in.In->BS()) & ~Poly::Typed));
  return !in.In->HasFailed();
}

inline auto WritePolylineFlags(const Polyline &polyline, const Fields::Output &to) -> bool
{
  to.To->BS(polyline.Flags);
  return true;
}

/**
 *  A 3D polyline's two bytes: the first 1 for a quadratic and 2 for a cubic spline fit, the second 1 when closed; the
 *  flags are 8 for being 3D, 4 when fitted, 1 when closed.
 */
inline auto Read3d(Polyline &polyline, const Fields::Input &in) -> bool
{
  const std::uint8_t curve = in.In->RC();
  const std::uint8_t closed = in.In->RC();
  polyline.Curve = 0;
  if ((curve & 1U) != 0)
  {
    polyline.Curve = Poly::Quadratic;
  }
  else if ((curve & 2U) != 0)
  {
    polyline.Curve = Poly::Cubic;
  }
  polyline.Flags =
      static_cast<int>(Poly::Polyline3d | (polyline.Curve != 0 ? Poly::Splined : 0U) | (closed & Poly::Closed));
  return !in.In->HasFailed();
}

inline auto CurveByteOf(int curve) -> std::uint8_t
{
  if (curve == Poly::Quadratic)
  {
    return 1;
  }
  return curve == Poly::Cubic ? 2 : 0;
}

inline auto Write3d(const Polyline &polyline, const Fields::Output &to) -> bool
{
  to.To->RC(CurveByteOf(polyline.Curve));
  to.To->RC(static_cast<std::uint8_t>(static_cast<unsigned>(polyline.Flags) & Poly::Closed));
  return true;
}

inline auto ReadVertexCount(Polyline &polyline, const Fields::Input &in) -> bool
{
  std::size_t count = 0;
  if (!Fields::ReadHandleCount(in, count))
  {
    return false;
  }
  polyline.Vertices.assign(count, Handle{});
  return true;
}

inline auto WriteVertexCount(const Polyline &polyline, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(polyline.Vertices.size()));
  return true;
}

/**
 *  Hard owners of every form: the text says soft pointers for a mesh and a face mesh, AutoCAD's files say hard owners.
 */
inline auto ReadVertices(Polyline &polyline, const Fields::Input &in) -> bool
{
  return std::ranges::all_of(polyline.Vertices,
                             [&in](Handle &vertex) -> bool { return Fields::ReadLink(vertex, 3, in); });
}

inline auto WriteVertices(const Polyline &polyline, const Fields::Output &to) -> bool
{
  return std::ranges::all_of(polyline.Vertices,
                             [&to](const Handle vertex) -> bool { return Fields::WriteLink(vertex, 3, to); });
}

/**
 *  0 2D, 1 3D, 2 polyface, 3 polygon mesh.
 */
inline auto KindOf(const Polyline &polyline) -> std::size_t
{
  if (IsPolyface(polyline))
  {
    return 2;
  }
  if (IsMesh(polyline))
  {
    return 3;
  }
  return Is3d(polyline) ? 1 : 0;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Polyline>
{
  static constexpr std::array<std::uint16_t, 4> Types{15, 16, 29, 30};
  static constexpr std::string_view Name = "POLYLINE_2D";

  static void Open(Entities::Polyline &polyline, std::uint16_t type)
  {
    using namespace Entities::Shapes;
    static constexpr std::array<unsigned, 4> Bits{0, Poly::Polyline3d, Poly::Polyface, Poly::Mesh};
    for (std::size_t at = 0; at < Types.size(); ++at)
    {
      if (Types.at(at) == type)
      {
        polyline.Flags = static_cast<int>(Bits.at(at));
      }
    }
  }

  static auto Code(const Entities::Polyline &polyline) -> std::uint16_t
  {
    return Types.at(Entities::Shapes::KindOf(polyline));
  }

  static auto NameOf(const Entities::Polyline &polyline) -> std::string_view
  {
    static constexpr std::array<std::string_view, 4> Names{"POLYLINE_2D", "POLYLINE_3D", "POLYLINE_PFACE",
                                                           "POLYLINE_MESH"};
    return Names.at(Entities::Shapes::KindOf(polyline));
  }

  static auto Layout()
  {
    using Entities::Polyline;
    using namespace Entities::Shapes;
    return Fields::Layout(
        Fields::When(Is2d, Fields::Custom(ReadPolylineFlags, WritePolylineFlags), Fields::Short(&Polyline::Curve),
                     Fields::Double(&Polyline::StartWidth), Fields::Double(&Polyline::EndWidth),
                     Fields::Thickness(&Polyline::Thickness), Fields::Double(&Polyline::Elevation),
                     Fields::Extrusion(&Polyline::Extrusion)),
        Fields::When(Is3d, Fields::Custom(Read3d, Write3d)),
        Fields::When(IsPolyface, Fields::Short(&Polyline::Rows), Fields::Short(&Polyline::Columns)),
        Fields::When(IsMesh, Fields::Custom(ReadPolylineFlags, WritePolylineFlags), Fields::Short(&Polyline::Curve),
                     Fields::Short(&Polyline::Rows), Fields::Short(&Polyline::Columns),
                     Fields::Short(&Polyline::Density), Fields::Short(&Polyline::Depth)),
        Fields::Since(Version::R2004, Fields::Custom(ReadVertexCount, WriteVertexCount)),
        Fields::Until(Version::R2000, Fields::Link(&Polyline::First, 4), Fields::Link(&Polyline::Last, 4)),
        Fields::Since(Version::R2004, Fields::Custom(ReadVertices, WriteVertices)), Fields::Link(&Polyline::SeqEnd, 3));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_POLYLINE_HPP */
