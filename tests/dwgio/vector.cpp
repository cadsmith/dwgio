/***********************************************************************************************************************
 *  @file      tests/dwgio/vector.cpp
 *  @brief     Comparing vectors.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/vector.hpp"

#include <doctest/doctest.h>

namespace DwgIO
{
TEST_CASE("vector: vectors compare by their three coordinates with z 0 by default")
{
  CHECK(Vector{.X = 1, .Y = 2} == Vector{.X = 1, .Y = 2, .Z = 0});
  CHECK_FALSE(Vector{.X = 1} == Vector{.Y = 1});
  CHECK(Up == Vector{.X = 0, .Y = 0, .Z = 1});
  CHECK_FALSE(Up == Vector{});
}
} // namespace DwgIO
