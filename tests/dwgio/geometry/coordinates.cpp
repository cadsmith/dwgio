/***********************************************************************************************************************
 *  @file      tests/dwgio/geometry/coordinates.cpp
 *  @brief     Points between object and world coordinates, against a circle AutoCAD drew in a turned UCS.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/geometry/coordinates.hpp"

#include "support/fixtures.hpp"
#include "support/vector.hpp"

#include "dwgio/entities/circle.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/vector.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <expected>

namespace DwgIO::Geometry
{
TEST_CASE("geometry/coordinates: the circle drawn at (0 0 -120) in a UCS turned about X lies at (0 120 0)")
{
  for (const char *name : {"entities-r2000.dwg", "entities-r2018.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    const auto &circle = Get<Entities::Circle>(*Find(loaded->Drawing, Handle{.Value = 0x2FC}));
    CHECK(Close(ToWorld(circle.Center, circle.Extrusion), Vector{.X = 0, .Y = 120, .Z = 0}));
    CHECK(Close(ToObject(Vector{.X = 0, .Y = 120, .Z = 0}, circle.Extrusion), circle.Center));
  }
}

TEST_CASE("geometry/coordinates: the arbitrary axes of a normal along X and mirrored and the world's and none")
{
  const Vector point{.X = 2, .Y = 3, .Z = 5};
  CHECK(Close(ToWorld(point, Vector{.X = 1, .Y = 0, .Z = 0}), Vector{.X = 5, .Y = 2, .Z = 3}));
  CHECK(Close(ToWorld(point, Vector{.X = 0, .Y = 0, .Z = -1}), Vector{.X = -2, .Y = 3, .Z = -5}));
  CHECK(Close(ToWorld(point, Up), point));
  CHECK(Close(ToWorld(point, Vector{}), point));
  CHECK(Close(ToWorld(point, Vector{.X = 0, .Y = 0, .Z = 7}), point));
  const Vector slanted{.X = 1, .Y = 2, .Z = 3};
  CHECK(Close(ToObject(ToWorld(point, slanted), slanted), point));
}
} // namespace DwgIO::Geometry
