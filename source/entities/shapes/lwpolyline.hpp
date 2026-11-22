/***********************************************************************************************************************
 *  @file      source/entities/shapes/lwpolyline.hpp
 *  @brief     How an LWPOLYLINE lies in the file, its flags saying what it stores.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_LWPOLYLINE_HPP
#define DWGIO_ENTITIES_SHAPES_LWPOLYLINE_HPP

#include "entities/shapes/count.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/lwpolyline.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  The file's flag bits say what the polyline stores; the last two are group 70's.
 */
namespace Lw
{
constexpr unsigned Extruded = 1;
constexpr unsigned Thick = 2;
constexpr unsigned Wide = 4;
constexpr unsigned Raised = 8;
constexpr unsigned Bulged = 16;
constexpr unsigned Widened = 32;
constexpr unsigned Plinegen = 256;
constexpr unsigned Closed = 512;
constexpr unsigned Identified = 1024;
constexpr unsigned DxfClosed = 1;
constexpr unsigned DxfPlinegen = 128;
} // namespace Lw

inline auto AnyBulge(const LwPolyline &polyline) -> bool
{
  return std::ranges::any_of(polyline.Vertices, [](const LwPolyline::Vertex &one) -> bool { return one.Bulge != 0; });
}

inline auto AnyWidths(const LwPolyline &polyline) -> bool
{
  return std::ranges::any_of(polyline.Vertices,
                             [](const LwPolyline::Vertex &one) -> bool { return one.Widths.has_value(); });
}

inline auto AnyId(const LwPolyline &polyline) -> bool
{
  return std::ranges::any_of(polyline.Vertices,
                             [](const LwPolyline::Vertex &one) -> bool { return one.Id.has_value(); });
}

inline auto FlagOf(const LwPolyline &polyline) -> unsigned
{
  const auto flags = static_cast<unsigned>(polyline.Flags);
  unsigned flag = 0;
  flag |= polyline.Extrusion != Up ? Lw::Extruded : 0U;
  flag |= polyline.Thickness != 0 ? Lw::Thick : 0U;
  flag |= polyline.Width ? Lw::Wide : 0U;
  flag |= polyline.Elevation != 0 ? Lw::Raised : 0U;
  flag |= AnyBulge(polyline) ? Lw::Bulged : 0U;
  flag |= AnyWidths(polyline) ? Lw::Widened : 0U;
  flag |= AnyId(polyline) ? Lw::Identified : 0U;
  flag |= (flags & Lw::DxfPlinegen) != 0 ? Lw::Plinegen : 0U;
  flag |= (flags & Lw::DxfClosed) != 0 ? Lw::Closed : 0U;
  return flag;
}

/**
 *  From R2010, one per vertex; those beyond the vertices are read and dropped.
 */
inline void ReadIds(LwPolyline &polyline, std::size_t count, const Fields::Input &in)
{
  for (std::size_t at = 0; at < count; ++at)
  {
    const std::int32_t identifier = in.In->BL();
    if (at < polyline.Vertices.size())
    {
      polyline.Vertices[at].Id = identifier;
    }
  }
}

/**
 *  Chapter 20.4.85.
 *
 *  The points are pairs up to R14, and from R2000 a pair then deltas against the one before.
 */
inline auto ReadLwPolyline(LwPolyline &polyline, const Fields::Input &in) -> bool
{
  const auto flag = static_cast<unsigned>(in.In->BS());
  polyline.Flags = static_cast<int>(((flag & Lw::Closed) != 0 ? Lw::DxfClosed : 0U) |
                                    ((flag & Lw::Plinegen) != 0 ? Lw::DxfPlinegen : 0U));
  polyline.Width = (flag & Lw::Wide) != 0 ? std::optional{in.In->BD()} : std::nullopt;
  polyline.Elevation = (flag & Lw::Raised) != 0 ? in.In->BD() : 0.0;
  polyline.Thickness = (flag & Lw::Thick) != 0 ? in.In->BD() : 0.0;
  polyline.Extrusion = (flag & Lw::Extruded) != 0 ? in.In->Point3() : Up;
  const std::optional<std::size_t> points = ReadCount(in);
  const std::optional<std::size_t> bulges = (flag & Lw::Bulged) != 0 ? ReadCount(in) : std::optional<std::size_t>{0};
  const bool identified = in.With.Release >= Version::R2010 && (flag & Lw::Identified) != 0;
  const std::optional<std::size_t> ids = identified ? ReadCount(in) : std::optional<std::size_t>{0};
  const std::optional<std::size_t> widths = (flag & Lw::Widened) != 0 ? ReadCount(in) : std::optional<std::size_t>{0};
  if (!points || !bulges || !ids || !widths)
  {
    return false;
  }
  polyline.Vertices.assign(*points, LwPolyline::Vertex{});
  for (std::size_t at = 0; at < *points; ++at)
  {
    Vector &point = polyline.Vertices[at].Position;
    const bool packed = in.With.Release >= Version::R2000 && at > 0;
    point.X = packed ? in.In->DD(polyline.Vertices[at - 1].Position.X) : in.In->RD();
    point.Y = packed ? in.In->DD(polyline.Vertices[at - 1].Position.Y) : in.In->RD();
  }
  for (std::size_t at = 0; at < *bulges; ++at)
  {
    const double bulge = in.In->BD();
    if (at < *points)
    {
      polyline.Vertices[at].Bulge = bulge;
    }
  }
  ReadIds(polyline, *ids, in);
  for (std::size_t at = 0; at < *widths; ++at)
  {
    const DwgIO::Widths pair{.Start = in.In->BD(), .End = in.In->BD()};
    if (at < *points)
    {
      polyline.Vertices[at].Widths = pair;
    }
  }
  return !in.In->HasFailed();
}

inline auto WriteLwPolyline(const LwPolyline &polyline, const Fields::Output &to) -> bool
{
  const unsigned identified = to.With.Release >= Version::R2010 ? Lw::Identified : 0U;
  const unsigned flag = FlagOf(polyline) & ~(Lw::Identified & ~identified);
  const auto count = static_cast<std::int32_t>(polyline.Vertices.size());
  to.To->BS(static_cast<int>(flag));
  if (polyline.Width)
  {
    to.To->BD(*polyline.Width);
  }
  if ((flag & Lw::Raised) != 0)
  {
    to.To->BD(polyline.Elevation);
  }
  if ((flag & Lw::Thick) != 0)
  {
    to.To->BD(polyline.Thickness);
  }
  if ((flag & Lw::Extruded) != 0)
  {
    to.To->Point3(polyline.Extrusion);
  }
  to.To->BL(count);
  if ((flag & Lw::Bulged) != 0)
  {
    to.To->BL(count);
  }
  if ((flag & Lw::Identified) != 0)
  {
    to.To->BL(count);
  }
  if ((flag & Lw::Widened) != 0)
  {
    to.To->BL(count);
  }
  for (std::size_t at = 0; at < polyline.Vertices.size(); ++at)
  {
    const Vector &point = polyline.Vertices[at].Position;
    if (to.With.Release >= Version::R2000 && at > 0)
    {
      to.To->DD(point.X, polyline.Vertices[at - 1].Position.X);
      to.To->DD(point.Y, polyline.Vertices[at - 1].Position.Y);
    }
    else
    {
      to.To->RD(point.X);
      to.To->RD(point.Y);
    }
  }
  if ((flag & Lw::Bulged) != 0)
  {
    for (const LwPolyline::Vertex &segment : polyline.Vertices)
    {
      to.To->BD(segment.Bulge);
    }
  }
  if ((flag & Lw::Identified) != 0)
  {
    for (const LwPolyline::Vertex &segment : polyline.Vertices)
    {
      to.To->BL(segment.Id.value_or(0));
    }
  }
  if ((flag & Lw::Widened) != 0)
  {
    for (const LwPolyline::Vertex &segment : polyline.Vertices)
    {
      const DwgIO::Widths pair = segment.Widths.value_or(DwgIO::Widths{});
      to.To->BD(pair.Start);
      to.To->BD(pair.End);
    }
  }
  return true;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::LwPolyline>
{
  static constexpr std::array<std::uint16_t, 1> Types{77};
  static constexpr Version Fixed = Version::R2000; /**< A class before R2000, which gave it its type code. */
  static constexpr std::string_view Name = "LWPOLYLINE";

  static auto Layout()
  {
    return Fields::Layout(Fields::Custom(Entities::Shapes::ReadLwPolyline, Entities::Shapes::WriteLwPolyline));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_LWPOLYLINE_HPP */
