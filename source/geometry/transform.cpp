/***********************************************************************************************************************
 *  @file      source/geometry/transform.cpp
 *  @brief     The transform an insert places its block with, and transforms applied and composed.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/geometry/transform.hpp"

#include "geometry/axes.hpp"

#include <cmath>
#include <cstddef>

namespace DwgIO::Geometry
{
namespace
{
constexpr std::size_t Side = 4;

auto Cell(const Matrix &matrix, std::size_t row, std::size_t column) -> double
{
  return matrix.at((row * Side) + column);
}
} // namespace

auto TransformOf(const Entities::Insert &insert, const Tables::BlockRecord &block) -> Matrix
{
  const Axes axes = AxesOf(insert.Extrusion);
  const double cos = std::cos(insert.Rotation);
  const double sin = std::sin(insert.Rotation);
  // In the insert's object coordinates: scale, then turn about Z, then move to the position.
  const Matrix placed{insert.Scale.X * cos,
                      -insert.Scale.Y * sin,
                      0,
                      insert.Position.X,
                      insert.Scale.X * sin,
                      insert.Scale.Y * cos,
                      0,
                      insert.Position.Y,
                      0,
                      0,
                      insert.Scale.Z,
                      insert.Position.Z,
                      0,
                      0,
                      0,
                      1};
  const Matrix world{axes.X.X, axes.Y.X, axes.Z.X, 0, axes.X.Y, axes.Y.Y, axes.Z.Y, 0,
                     axes.X.Z, axes.Y.Z, axes.Z.Z, 0, 0,        0,        0,        1};
  const Matrix based{1, 0, 0, -block.Origin.X, 0, 1, 0, -block.Origin.Y, 0, 0, 1, -block.Origin.Z, 0, 0, 0, 1};
  return Compose(world, Compose(placed, based));
}

auto Transform(const Matrix &transform, Vector point) -> Vector
{
  const auto row = [&](std::size_t at) -> double
  {
    return (Cell(transform, at, 0) * point.X) + (Cell(transform, at, 1) * point.Y) +
           (Cell(transform, at, 2) * point.Z) + Cell(transform, at, 3);
  };
  return Vector{.X = row(0), .Y = row(1), .Z = row(2)};
}

auto Compose(const Matrix &outer, const Matrix &inner) -> Matrix
{
  Matrix composed{};
  for (std::size_t row = 0; row < Side; ++row)
  {
    for (std::size_t column = 0; column < Side; ++column)
    {
      double sum = 0;
      for (std::size_t at = 0; at < Side; ++at)
      {
        sum += Cell(outer, row, at) * Cell(inner, at, column);
      }
      composed.at((row * Side) + column) = sum;
    }
  }
  return composed;
}
} // namespace DwgIO::Geometry
