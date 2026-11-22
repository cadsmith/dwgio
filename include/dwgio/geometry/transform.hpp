/***********************************************************************************************************************
 *  @file      dwgio/geometry/transform.hpp
 *  @brief     The transform an insert places its block with, and transforms applied and composed.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_GEOMETRY_TRANSFORM_HPP
#define DWGIO_GEOMETRY_TRANSFORM_HPP

#include "dwgio/api.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/matrix.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Geometry
{
/**
 *  What carries a point of the block into world coordinates: the block's base point taken off, the insert's scale and
 *  rotation applied, the result moved to its position and taken from its object coordinate system to the world.
 *
 *  A MINSERT's first copy, at its first column and row. For a block placed inside another block, compose the inner
 *  insert's transform after the outer's: `Compose(outer, inner)`.
 */
[[nodiscard]] DWGIO_API auto TransformOf(const Entities::Insert &insert, const Tables::BlockRecord &block) -> Matrix;

/**
 *  The point the transform carries a point to.
 */
[[nodiscard]] DWGIO_API auto Transform(const Matrix &transform, Vector point) -> Vector;

/**
 *  The transform that applies the inner one, then the outer.
 */
[[nodiscard]] DWGIO_API auto Compose(const Matrix &outer, const Matrix &inner) -> Matrix;
} // namespace DwgIO::Geometry

#endif /* DWGIO_GEOMETRY_TRANSFORM_HPP */
