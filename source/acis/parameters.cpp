/***********************************************************************************************************************
 *  @file      source/acis/parameters.cpp
 *  @brief     The parameters at an edge's vertices found from its curve.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/parameters.hpp"

#include "acis/grammar.hpp"
#include "acis/model.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace DwgIO::Acis
{
namespace
{
using Triple = std::array<double, 3>;
using Range = std::pair<double, double>;

/**
 *  Where an edge's values lie after its prefix: its start vertex and parameter, end vertex and parameter, coedge, curve
 *  and sense.
 *
 *  A vertex's point, a point's position, and a curve's first values after the prefix.
 */
namespace Edge
{
constexpr std::size_t Start = Prefix::Entity;
constexpr std::size_t Lower = Start + 1;
constexpr std::size_t End = Start + 2;
constexpr std::size_t Upper = Start + 3;
constexpr std::size_t Curve = Start + 5;
constexpr std::size_t Sense = Start + 6;
} // namespace Edge

constexpr std::size_t VertexPoint = Prefix::Entity + 2;
constexpr std::size_t First = Prefix::Entity;

/**
 *  Where an intcurve's B-spline lies: its sense, the subtype's name, version and approximation, then the form, degree,
 *  closure and knot count, then the knots.
 */
namespace Intcurve
{
constexpr std::size_t Sense = Prefix::Entity;
constexpr std::size_t Name = Sense + 1;
constexpr std::size_t Form = Sense + 4;
constexpr std::size_t Degree = Sense + 5;
constexpr std::size_t Knots = Sense + 7;
} // namespace Intcurve

auto Minus(const Triple &left, const Triple &right) -> Triple
{
  return {left[0] - right[0], left[1] - right[1], left[2] - right[2]};
}

auto Dot(const Triple &left, const Triple &right) -> double
{
  return (left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]);
}

auto Cross(const Triple &left, const Triple &right) -> Triple
{
  return {(left[1] * right[2]) - (left[2] * right[1]), (left[2] * right[0]) - (left[0] * right[2]),
          (left[0] * right[1]) - (left[1] * right[0])};
}

/**
 *  Whether two points are the same within the tolerance AutoCAD's bodies give.
 */
auto Near(const Triple &left, const Triple &right) -> bool
{
  constexpr double Tolerance = 1e-6;
  const Triple apart = Minus(left, right);
  return Dot(apart, apart) <= Tolerance * Tolerance;
}

auto Position(const Model &model, std::int64_t vertex) -> std::optional<Triple>
{
  const Record *record = FindRecord(model, vertex, "vertex");
  const Record *point = record == nullptr ? nullptr : FindRecord(model, record->Values[VertexPoint].Integer, "point");
  return point == nullptr ? std::nullopt : std::optional{point->Values[First].Reals};
}

/**
 *  The parameter of a point along a straight line: its distance from the root, in lengths of the direction.
 */
auto Along(const Record &line, const Triple &point) -> double
{
  const Triple &direction = line.Values[First + 1].Reals;
  return Dot(Minus(point, line.Values[First].Reals), direction) / Dot(direction, direction);
}

/**
 *  The parameter of a point around an ellipse: its angle from the major axis, from a half turn back to just short of a
 *  half turn on, where AutoCAD starts a circle closed on its far side.
 */
auto Around(const Record &ellipse, const Triple &point) -> double
{
  const Triple offset = Minus(point, ellipse.Values[First].Reals);
  const Triple &normal = ellipse.Values[First + 1].Reals;
  const Triple &major = ellipse.Values[First + 2].Reals;
  const double cosine = Dot(offset, major);
  const double sine = Dot(offset, Cross(normal, major)) / ellipse.Values[First + 3].Reals[0];
  const double angle = std::atan2(sine, cosine);
  return angle >= std::numbers::pi ? angle - (2 * std::numbers::pi) : angle;
}

/**
 *  A B-spline's first and last knots and first and last control points.
 */
struct Ends
{
  Range Knots{};
  std::pair<Triple, Triple> Points{};
};

/**
 *  The ends of an intcurve's B-spline, when it is one of its own and has control points.
 */
auto Bspline(const Record &curve) -> std::optional<Ends>
{
  if (curve.Values[Intcurve::Name].Text == "ref" || curve.Values[Intcurve::Knots].Integer < 1)
  {
    return std::nullopt;
  }
  const std::size_t knots = Intcurve::Knots + 1;
  const auto pairs = static_cast<std::size_t>(curve.Values[Intcurve::Knots].Integer);
  std::int64_t sum = 0;
  for (std::size_t knot = 0; knot < pairs; ++knot)
  {
    sum += curve.Values[knots + (2 * knot) + 1].Integer;
  }
  const std::int64_t controls = sum - curve.Values[Intcurve::Degree].Integer + 1;
  if (controls < 1)
  {
    return std::nullopt;
  }
  const std::size_t width = curve.Values[Intcurve::Form].Text == "nurbs" ? 4 : 3;
  const std::size_t first = knots + (2 * pairs);
  const std::size_t last = first + ((static_cast<std::size_t>(controls) - 1) * width);
  const auto point = [&curve](std::size_t at) -> Triple
  {
    return {curve.Values[at].Reals[0], curve.Values[at + 1].Reals[0], curve.Values[at + 2].Reals[0]};
  };
  return Ends{.Knots = {curve.Values[knots].Reals[0], curve.Values[knots + (2 * (pairs - 1))].Reals[0]},
              .Points = {point(first), point(last)}};
}

/**
 *  The parameters at an edge's two ends along an intcurve: its first and last knots when the edge runs from its first
 *  control point to its last or closes on itself at the first.
 */
auto FindIntcurveRange(const Record &curve, const Triple &start, const Triple &end) -> std::optional<Range>
{
  const std::optional<Ends> ends = curve.Values[Intcurve::Sense].Integer == 0 ? Bspline(curve) : std::nullopt;
  if (!ends || !Near(start, ends->Points.first) || !(start == end || Near(end, ends->Points.second)))
  {
    return std::nullopt;
  }
  return ends->Knots;
}

/**
 *  The parameters at an ellipse's two ends, the end past the start, a whole turn past it when the edge closes.
 */
auto MakeEllipseRange(const Record &ellipse, const Triple &start, const Triple &end) -> Range
{
  const double from = Around(ellipse, start);
  double to = Around(ellipse, end);
  while (to <= from)
  {
    to += 2 * std::numbers::pi;
  }
  return {from, to};
}

/**
 *  An edge's parameters at its two ends; nothing for a kind of edge AutoCAD's files never hold.
 */
auto FindRange(const Model &model, const Record &edge) -> std::optional<Range>
{
  const std::int64_t curve = edge.Values[Edge::Curve].Integer;
  if (curve < 0)
  {
    return Range{1.0, 0.0}; // An edge with no curve, a cone's apex, has an empty range.
  }
  const std::optional<Triple> start = Position(model, edge.Values[Edge::Start].Integer);
  const std::optional<Triple> end = Position(model, edge.Values[Edge::End].Integer);
  if (edge.Values[Edge::Sense].Integer != 0 || !start || !end)
  {
    return std::nullopt;
  }
  if (const Record *line = FindRecord(model, curve, "straight-curve"))
  {
    const Triple &direction = line->Values[First + 1].Reals;
    const double from = Along(*line, *start);
    const double to = Along(*line, *end);
    return Dot(direction, direction) != 0 && from != to ? std::optional{Range{from, to}} : std::nullopt;
  }
  if (const Record *ellipse = FindRecord(model, curve, "ellipse-curve"))
  {
    return MakeEllipseRange(*ellipse, *start, *end);
  }
  const Record *spline = FindRecord(model, curve, "intcurve-curve");
  return spline == nullptr ? std::nullopt : FindIntcurveRange(*spline, *start, *end);
}
} // namespace

auto Parameters(Model &model) -> std::string
{
  for (std::size_t at = 0; at < model.Records.size(); ++at)
  {
    Record &edge = model.Records[at];
    if (edge.Name != "edge")
    {
      continue;
    }
    const std::optional<Range> found = FindRange(model, edge);
    if (!found)
    {
      return std::format("parameters of edge {} not found", at);
    }
    edge.Values[Edge::Lower].Reals[0] = found->first;
    edge.Values[Edge::Upper].Reals[0] = found->second;
  }
  return {};
}
} // namespace DwgIO::Acis
