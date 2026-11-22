/***********************************************************************************************************************
 *  @file      source/entities/shapes/spline.hpp
 *  @brief     How a SPLINE lies in the file, by its poles or its fit points, chapter 20.4.40.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_SPLINE_HPP
#define DWGIO_ENTITIES_SHAPES_SPLINE_HPP

#include "entities/shapes/count.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/spline.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  The two scenarios a spline is stored in, and the bits of group 70 the first stores.
 */
namespace Nurbs
{
constexpr std::int32_t ByPoles = 1;
constexpr std::int32_t ByFits = 2;
constexpr unsigned Closed = 1;
constexpr unsigned Periodic = 2;
constexpr unsigned Rational = 4;
/**
 *  In R2013's spline flags.
 */
constexpr unsigned FitMethod = 1;
} // namespace Nurbs

template <typename CurveT>
auto IsRational(const CurveT &spline) -> bool
{
  return (static_cast<unsigned>(spline.Flags) & Nurbs::Rational) != 0;
}

/**
 *  Chapter 20.4.40; a helix stores its curve the same way.
 */
template <typename CurveT>
auto ReadSpline(CurveT &spline, const Fields::Input &in) -> bool
{
  std::int32_t scenario = in.In->BL();
  if (in.With.Release >= Version::R2013)
  {
    // From R2013 the spline flags say which form follows; AutoCAD writes the scenario as 1 for either.
    spline.Options = in.In->BL();
    spline.Parameter = in.In->BL();
    scenario = (static_cast<unsigned>(spline.Options) & Nurbs::FitMethod) != 0 ? Nurbs::ByFits : Nurbs::ByPoles;
  }
  spline.Degree = static_cast<int>(in.In->BL());
  std::optional<std::size_t> knots{0};
  std::optional<std::size_t> poles{0};
  std::optional<std::size_t> fits{0};
  spline.FitTolerance.reset();
  spline.Flags = 0;
  if (scenario == Nurbs::ByFits)
  {
    spline.FitTolerance = in.In->BD();
    spline.StartTangent = in.In->Point3();
    spline.EndTangent = in.In->Point3();
    fits = ReadCount(in);
  }
  else if (scenario == Nurbs::ByPoles)
  {
    const auto rational = static_cast<unsigned>(in.In->B());
    const auto closed = static_cast<unsigned>(in.In->B());
    const auto periodic = static_cast<unsigned>(in.In->B());
    spline.Flags =
        static_cast<int>((rational * Nurbs::Rational) | (closed * Nurbs::Closed) | (periodic * Nurbs::Periodic));
    spline.KnotTolerance = in.In->BD();
    spline.ControlTolerance = in.In->BD();
    knots = ReadCount(in);
    poles = ReadCount(in);
    static_cast<void>(in.In->B());
  }
  else
  {
    Fields::Note(in, IssueCode::Unknown, std::format("spline scenario {} unknown", scenario));
    return false;
  }
  if (!knots || !poles || !fits)
  {
    return false;
  }
  spline.Knots.assign(*knots, 0.0);
  for (double &knot : spline.Knots)
  {
    knot = in.In->BD();
  }
  spline.ControlPoints.assign(*poles, Vector{});
  spline.Weights.clear();
  for (Vector &pole : spline.ControlPoints)
  {
    pole = in.In->Point3();
    if (IsRational(spline))
    {
      spline.Weights.push_back(in.In->BD());
    }
  }
  spline.FitPoints.assign(*fits, Vector{});
  for (Vector &fit : spline.FitPoints)
  {
    fit = in.In->Point3();
  }
  return !in.In->HasFailed();
}

template <typename CurveT>
auto WriteSpline(const CurveT &spline, const Fields::Output &to) -> bool
{
  const auto flags = static_cast<unsigned>(spline.Flags);
  if (to.With.Release >= Version::R2013)
  {
    const auto options = static_cast<unsigned>(spline.Options);
    to.To->BL(Nurbs::ByPoles);
    to.To->BL(
        static_cast<std::int32_t>(spline.FitTolerance ? options | Nurbs::FitMethod : options & ~Nurbs::FitMethod));
    to.To->BL(spline.Parameter);
  }
  else
  {
    to.To->BL(spline.FitTolerance ? Nurbs::ByFits : Nurbs::ByPoles);
  }
  to.To->BL(spline.Degree);
  if (spline.FitTolerance)
  {
    to.To->BD(*spline.FitTolerance);
    to.To->Point3(spline.StartTangent);
    to.To->Point3(spline.EndTangent);
    to.To->BL(static_cast<std::int32_t>(spline.FitPoints.size()));
    for (const Vector &fit : spline.FitPoints)
    {
      to.To->Point3(fit);
    }
    return true;
  }
  to.To->B((flags & Nurbs::Rational) != 0);
  to.To->B((flags & Nurbs::Closed) != 0);
  to.To->B((flags & Nurbs::Periodic) != 0);
  to.To->BD(spline.KnotTolerance);
  to.To->BD(spline.ControlTolerance);
  to.To->BL(static_cast<std::int32_t>(spline.Knots.size()));
  to.To->BL(static_cast<std::int32_t>(spline.ControlPoints.size()));
  to.To->B((flags & Nurbs::Rational) != 0);
  for (const double knot : spline.Knots)
  {
    to.To->BD(knot);
  }
  for (std::size_t at = 0; at < spline.ControlPoints.size(); ++at)
  {
    to.To->Point3(spline.ControlPoints[at]);
    if (IsRational(spline))
    {
      to.To->BD(at < spline.Weights.size() ? spline.Weights[at] : 1.0);
    }
  }
  return true;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Spline>
{
  static constexpr std::array<std::uint16_t, 1> Types{36};
  static constexpr std::string_view Name = "SPLINE";

  static auto Layout()
  {
    using Entities::Curve;
    using Entities::Spline;
    return Fields::Layout(Fields::Part(
        &Spline::Curve,
        Fields::Layout(Fields::Custom(Entities::Shapes::ReadSpline<Curve>, Entities::Shapes::WriteSpline<Curve>))));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_SPLINE_HPP */
