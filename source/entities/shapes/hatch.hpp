/***********************************************************************************************************************
 *  @file      source/entities/shapes/hatch.hpp
 *  @brief     How a HATCH lies in the file: its paths, edges, pattern and gradient.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_HATCH_HPP
#define DWGIO_ENTITIES_SHAPES_HATCH_HPP

#include "entities/shapes/count.hpp"
#include "entities/shapes/lettering.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/hatch.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>
#include <variant>

namespace DwgIO::Entities::Shapes
{
/**
 *  The path flags the file reads; a derived boundary brings the pixel size.
 */
namespace Bound
{
constexpr unsigned Polyline = 2;
constexpr unsigned Derived = 4;
} // namespace Bound

inline auto IsPolyline(const Path &path) -> bool
{
  return (static_cast<unsigned>(path.Flags) & Bound::Polyline) != 0;
}

inline auto HasDerived(const Hatch &hatch) -> bool
{
  return std::ranges::any_of(hatch.Paths, [](const Path &path) -> bool
                             { return (static_cast<unsigned>(path.Flags) & Bound::Derived) != 0; });
}

inline auto ReadSplineEdge(Edges::Spline &spline, const Fields::Input &in) -> bool
{
  spline.Degree = static_cast<int>(in.In->BL());
  spline.Rational = in.In->B();
  spline.Periodic = in.In->B();
  const std::optional<std::size_t> knots = ReadCount(in);
  const std::optional<std::size_t> poles = ReadCount(in);
  if (!knots || !poles)
  {
    return false;
  }
  spline.Knots.assign(*knots, 0.0);
  for (double &knot : spline.Knots)
  {
    knot = in.In->BD();
  }
  spline.ControlPoints.assign(*poles, Edges::Node{});
  for (Edges::Node &pole : spline.ControlPoints)
  {
    pole.Position = ReadPair(in);
    pole.Weight = spline.Rational ? in.In->BD() : 0.0;
  }
  if (in.With.Release >= Version::R2010)
  {
    const std::optional<std::size_t> fits = ReadCount(in);
    if (!fits)
    {
      return false;
    }
    spline.FitPoints.assign(*fits, Vector{});
    for (Vector &fit : spline.FitPoints)
    {
      fit = ReadPair(in);
    }
    spline.StartTangent = Vector{};
    spline.EndTangent = Vector{};
    if (!spline.FitPoints.empty())
    {
      spline.StartTangent = ReadPair(in);
      spline.EndTangent = ReadPair(in);
    }
  }
  return !in.In->HasFailed();
}

inline void WriteSplineEdge(const Edges::Spline &spline, const Fields::Output &to)
{
  to.To->BL(spline.Degree);
  to.To->B(spline.Rational);
  to.To->B(spline.Periodic);
  to.To->BL(static_cast<std::int32_t>(spline.Knots.size()));
  to.To->BL(static_cast<std::int32_t>(spline.ControlPoints.size()));
  for (const double knot : spline.Knots)
  {
    to.To->BD(knot);
  }
  for (const Edges::Node &pole : spline.ControlPoints)
  {
    WritePair(pole.Position, to);
    if (spline.Rational)
    {
      to.To->BD(pole.Weight);
    }
  }
  if (to.With.Release >= Version::R2010)
  {
    to.To->BL(static_cast<std::int32_t>(spline.FitPoints.size()));
    for (const Vector &fit : spline.FitPoints)
    {
      WritePair(fit, to);
    }
    if (!spline.FitPoints.empty())
    {
      WritePair(spline.StartTangent, to);
      WritePair(spline.EndTangent, to);
    }
  }
}

/**
 *  The type byte: 1 line, 2 arc, 3 ellipse, 4 spline.
 */
inline auto ReadEdge(Edges::Edge &edge, const Fields::Input &in) -> bool
{
  const std::uint8_t type = in.In->RC();
  if (type == 1)
  {
    Edges::Line line;
    line.Start = ReadPair(in);
    line.End = ReadPair(in);
    edge = line;
  }
  else if (type == 2)
  {
    Edges::Arc arc;
    arc.Center = ReadPair(in);
    arc.Radius = in.In->BD();
    arc.Start = in.In->BD();
    arc.End = in.In->BD();
    arc.CounterClockwise = in.In->B();
    edge = arc;
  }
  else if (type == 3)
  {
    Edges::Ellipse ellipse;
    ellipse.Center = ReadPair(in);
    ellipse.Major = ReadPair(in);
    ellipse.Ratio = in.In->BD();
    ellipse.Start = in.In->BD();
    ellipse.End = in.In->BD();
    ellipse.CounterClockwise = in.In->B();
    edge = ellipse;
  }
  else if (type == 4)
  {
    Edges::Spline spline;
    if (!ReadSplineEdge(spline, in))
    {
      return false;
    }
    edge = std::move(spline);
  }
  else
  {
    Fields::Note(in, IssueCode::Unknown, std::format("hatch edge type {} unknown", type));
    in.In->Seek(in.In->Size() + 1);
    return false;
  }
  return !in.In->HasFailed();
}

inline void WriteEdge(const Edges::Edge &edge, const Fields::Output &to)
{
  to.To->RC(static_cast<std::uint8_t>(edge.index() + 1));
  std::visit(
      [&to](const auto &one) -> void
      {
        using EdgeT = std::remove_cvref_t<decltype(one)>;
        if constexpr (std::is_same_v<EdgeT, Edges::Line>)
        {
          WritePair(one.Start, to);
          WritePair(one.End, to);
        }
        else if constexpr (std::is_same_v<EdgeT, Edges::Arc>)
        {
          WritePair(one.Center, to);
          to.To->BD(one.Radius);
          to.To->BD(one.Start);
          to.To->BD(one.End);
          to.To->B(one.CounterClockwise);
        }
        else if constexpr (std::is_same_v<EdgeT, Edges::Ellipse>)
        {
          WritePair(one.Center, to);
          WritePair(one.Major, to);
          to.To->BD(one.Ratio);
          to.To->BD(one.Start);
          to.To->BD(one.End);
          to.To->B(one.CounterClockwise);
        }
        else
        {
          WriteSplineEdge(one, to);
        }
      },
      edge);
}

/**
 *  The flags, the edges or the polyline, then the source count; the sources' handles come later.
 */
inline auto ReadPath(Path &path, const Fields::Input &in) -> bool
{
  path.Flags = static_cast<int>(in.In->BL());
  if (!IsPolyline(path))
  {
    const std::optional<std::size_t> edges = ReadCount(in);
    if (!edges)
    {
      return false;
    }
    path.Edges.assign(*edges, Edges::Edge{});
    for (Edges::Edge &edge : path.Edges)
    {
      if (!ReadEdge(edge, in))
      {
        return false;
      }
    }
  }
  else
  {
    path.Bulged = in.In->B();
    path.Closed = in.In->B();
    const std::optional<std::size_t> vertices = ReadCount(in);
    if (!vertices)
    {
      return false;
    }
    path.Vertices.assign(*vertices, Edges::Node{});
    for (Edges::Node &vertex : path.Vertices)
    {
      vertex.Position = ReadPair(in);
      vertex.Weight = path.Bulged ? in.In->BD() : 0.0;
    }
  }
  const std::optional<std::size_t> sources = ReadCount(in);
  if (!sources)
  {
    return false;
  }
  path.Sources.assign(*sources, Handle{});
  return !in.In->HasFailed();
}

inline void WritePath(const Path &path, const Fields::Output &to)
{
  to.To->BL(path.Flags);
  if (!IsPolyline(path))
  {
    to.To->BL(static_cast<std::int32_t>(path.Edges.size()));
    for (const Edges::Edge &edge : path.Edges)
    {
      WriteEdge(edge, to);
    }
  }
  else
  {
    to.To->B(path.Bulged);
    to.To->B(path.Closed);
    to.To->BL(static_cast<std::int32_t>(path.Vertices.size()));
    for (const Edges::Node &vertex : path.Vertices)
    {
      WritePair(vertex.Position, to);
      if (path.Bulged)
      {
        to.To->BD(vertex.Weight);
      }
    }
  }
  to.To->BL(static_cast<std::int32_t>(path.Sources.size()));
}

inline auto ReadPattern(Hatch &hatch, const Fields::Input &in) -> bool
{
  hatch.Angle = in.In->BD();
  hatch.Scale = in.In->BD();
  hatch.Doubled = in.In->B();
  const std::optional<std::size_t> stripes = ReadShortCount(in);
  if (!stripes)
  {
    return false;
  }
  hatch.PatternLines.assign(*stripes, PatternLine{});
  for (PatternLine &stripe : hatch.PatternLines)
  {
    stripe.Angle = in.In->BD();
    stripe.X = in.In->BD();
    stripe.Y = in.In->BD();
    stripe.Run = in.In->BD();
    stripe.Rise = in.In->BD();
    const std::optional<std::size_t> dashes = ReadShortCount(in);
    if (!dashes)
    {
      return false;
    }
    stripe.Dashes.assign(*dashes, 0.0);
    for (double &dash : stripe.Dashes)
    {
      dash = in.In->BD();
    }
  }
  return !in.In->HasFailed();
}

inline void WritePattern(const Hatch &hatch, const Fields::Output &to)
{
  to.To->BD(hatch.Angle);
  to.To->BD(hatch.Scale);
  to.To->B(hatch.Doubled);
  to.To->BS(static_cast<int>(hatch.PatternLines.size()));
  for (const PatternLine &stripe : hatch.PatternLines)
  {
    to.To->BD(stripe.Angle);
    to.To->BD(stripe.X);
    to.To->BD(stripe.Y);
    to.To->BD(stripe.Run);
    to.To->BD(stripe.Rise);
    to.To->BS(static_cast<int>(stripe.Dashes.size()));
    for (const double dash : stripe.Dashes)
    {
      to.To->BD(dash);
    }
  }
}

inline auto ReadGradient(Gradient &gradient, const Fields::Input &in) -> bool
{
  gradient.Shown = in.In->BL();
  gradient.Unknown1 = in.In->BL();
  gradient.Angle = in.In->BD();
  gradient.Shift = in.In->BD();
  gradient.Single = in.In->BL();
  gradient.Tint = in.In->BD();
  const std::optional<std::size_t> shades = ReadCount(in);
  if (!shades)
  {
    return false;
  }
  gradient.Colors.assign(*shades, GradientColor{});
  for (GradientColor &shade : gradient.Colors)
  {
    shade.Place = in.In->BD();
    shade.Unknown1 = in.In->BS();
    shade.Rgb = in.In->BL();
    shade.Unknown2 = in.In->RC();
  }
  gradient.Name = Fields::TextCode::Read(in);
  return !in.In->HasFailed();
}

inline void WriteGradient(const Gradient &gradient, const Fields::Output &to)
{
  to.To->BL(gradient.Shown);
  to.To->BL(gradient.Unknown1);
  to.To->BD(gradient.Angle);
  to.To->BD(gradient.Shift);
  to.To->BL(gradient.Single);
  to.To->BD(gradient.Tint);
  to.To->BL(static_cast<std::int32_t>(gradient.Colors.size()));
  for (const GradientColor &shade : gradient.Colors)
  {
    to.To->BD(shade.Place);
    to.To->BS(shade.Unknown1);
    to.To->BL(shade.Rgb);
    to.To->RC(static_cast<std::uint8_t>(shade.Unknown2));
  }
  Fields::TextCode::Write(gradient.Name, to);
}

/**
 *  Chapter 20.4.75, without the sources' handles.
 */
inline auto ReadHatch(Hatch &hatch, const Fields::Input &in) -> bool
{
  if (in.With.Release >= Version::R2004 && !ReadGradient(hatch.Gradient, in))
  {
    return false;
  }
  hatch.Elevation = in.In->BD();
  hatch.Normal = in.In->Point3();
  hatch.Name = Fields::TextCode::Read(in);
  hatch.Solid = in.In->B();
  hatch.Associative = in.In->B();
  const std::optional<std::size_t> paths = ReadCount(in);
  if (!paths)
  {
    return false;
  }
  hatch.Paths.assign(*paths, Path{});
  for (Path &path : hatch.Paths)
  {
    if (!ReadPath(path, in))
    {
      return false;
    }
  }
  hatch.Style = in.In->BS();
  hatch.Kind = in.In->BS();
  if (!hatch.Solid && !ReadPattern(hatch, in))
  {
    return false;
  }
  hatch.Pixel = HasDerived(hatch) ? in.In->BD() : 0.0;
  const std::optional<std::size_t> seeds = ReadCount(in);
  if (!seeds)
  {
    return false;
  }
  hatch.Seeds.assign(*seeds, Vector{});
  for (Vector &seed : hatch.Seeds)
  {
    seed = ReadPair(in);
  }
  return !in.In->HasFailed();
}

inline auto WriteHatch(const Hatch &hatch, const Fields::Output &to) -> bool
{
  if (to.With.Release >= Version::R2004)
  {
    WriteGradient(hatch.Gradient, to);
  }
  to.To->BD(hatch.Elevation);
  to.To->Point3(hatch.Normal);
  Fields::TextCode::Write(hatch.Name, to);
  to.To->B(hatch.Solid);
  to.To->B(hatch.Associative);
  to.To->BL(static_cast<std::int32_t>(hatch.Paths.size()));
  for (const Path &path : hatch.Paths)
  {
    WritePath(path, to);
  }
  to.To->BS(hatch.Style);
  to.To->BS(hatch.Kind);
  if (!hatch.Solid)
  {
    WritePattern(hatch, to);
  }
  if (HasDerived(hatch))
  {
    to.To->BD(hatch.Pixel);
  }
  to.To->BL(static_cast<std::int32_t>(hatch.Seeds.size()));
  for (const Vector &seed : hatch.Seeds)
  {
    WritePair(seed, to);
  }
  return true;
}

inline auto ReadSources(Hatch &hatch, const Fields::Input &in) -> bool
{
  return std::ranges::all_of(hatch.Paths,
                             [&in](Path &path) -> bool
                             {
                               return std::ranges::all_of(path.Sources, [&in](Handle &source) -> bool
                                                          { return Fields::ReadLink(source, 4, in); });
                             });
}

inline auto WriteSources(const Hatch &hatch, const Fields::Output &to) -> bool
{
  return std::ranges::all_of(hatch.Paths,
                             [&to](const Path &path) -> bool
                             {
                               return std::ranges::all_of(path.Sources, [&to](const Handle source) -> bool
                                                          { return Fields::WriteLink(source, 4, to); });
                             });
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Hatch>
{
  static constexpr std::array<std::uint16_t, 1> Types{78};
  static constexpr Version Fixed = Version::R2000; /**< A class before R2000, which gave it its type code. */
  static constexpr std::string_view Name = "HATCH";

  static auto Layout()
  {
    return Fields::Layout(Fields::Custom(Entities::Shapes::ReadHatch, Entities::Shapes::WriteHatch),
                          Fields::Custom(Entities::Shapes::ReadSources, Entities::Shapes::WriteSources));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_HATCH_HPP */
