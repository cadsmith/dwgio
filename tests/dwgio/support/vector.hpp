/***********************************************************************************************************************
 *  @file      tests/dwgio/support/vector.hpp
 *  @brief     Vectors compared within a tolerance.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_VECTOR_HPP
#define DWGIO_TESTS_SUPPORT_VECTOR_HPP

#include "dwgio/vector.hpp"

#include <doctest/doctest.h>

#include <cmath>

namespace DwgIO
{
/**
 *  Whether two vectors agree on each axis as doctest::Approx compares, relative to their size.
 */
inline auto Near(const Vector &one, const Vector &other) -> bool
{
  return one.X == doctest::Approx(other.X) && one.Y == doctest::Approx(other.Y) && one.Z == doctest::Approx(other.Z);
}

/**
 *  Whether two vectors lie within 1e-9 of each other on each axis.
 */
inline auto Close(const Vector &one, const Vector &other) -> bool
{
  return std::abs(one.X - other.X) < 1e-9 && std::abs(one.Y - other.Y) < 1e-9 && std::abs(one.Z - other.Z) < 1e-9;
}
} // namespace DwgIO

#endif /* DWGIO_TESTS_SUPPORT_VECTOR_HPP */
