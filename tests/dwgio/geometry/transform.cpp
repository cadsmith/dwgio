/***********************************************************************************************************************
 *  @file      tests/dwgio/geometry/transform.cpp
 *  @brief     The transform an insert places its block with, against the inserts AutoCAD drew.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/geometry/transform.hpp"

#include "support/vector.hpp"

#include "dwgio/entities/insert.hpp"
#include "dwgio/matrix.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/vector.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <numbers>

namespace DwgIO::Geometry
{
TEST_CASE("geometry/transform: an insert maps from its base point through scale and rotation into its plane")
{
  Tables::BlockRecord block;
  block.Origin = Vector{.X = 1, .Y = 1, .Z = 0};
  Entities::Insert insert;
  insert.Position = Vector{.X = 10, .Y = 20, .Z = 0};
  insert.Scale = Vector{.X = 2, .Y = 3, .Z = 4};
  insert.Rotation = std::numbers::pi / 2;
  const Matrix placed = TransformOf(insert, block);
  // (2, 1, 1) is (1, 0, 1) from the base point, (2, 0, 4) scaled, (0, 2, 4) turned.
  CHECK(Close(Transform(placed, Vector{.X = 2, .Y = 1, .Z = 1}), Vector{.X = 10, .Y = 22, .Z = 4}));
  CHECK(Close(Transform(placed, block.Origin), insert.Position));
  insert.Extrusion = Vector{.X = 0, .Y = 0, .Z = -1};
  CHECK(Close(Transform(TransformOf(insert, block), block.Origin), Vector{.X = -10, .Y = 20, .Z = 0}));
}

TEST_CASE("geometry/transform: the inner transform of a composition applies first")
{
  Tables::BlockRecord block;
  Entities::Insert inner;
  inner.Position = Vector{.X = 1, .Y = 0, .Z = 0};
  Entities::Insert outer;
  outer.Scale = Vector{.X = 2, .Y = 2, .Z = 2};
  const Matrix both = Compose(TransformOf(outer, block), TransformOf(inner, block));
  CHECK(Close(Transform(both, Vector{.X = 1, .Y = 1, .Z = 1}), Vector{.X = 4, .Y = 2, .Z = 2}));
  CHECK(Compose(Identity, both) == both);
  CHECK(Close(Transform(Identity, Vector{.X = 3, .Y = 4, .Z = 5}), Vector{.X = 3, .Y = 4, .Z = 5}));
}
} // namespace DwgIO::Geometry
