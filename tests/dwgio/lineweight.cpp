/***********************************************************************************************************************
 *  @file      tests/dwgio/lineweight.cpp
 *  @brief     The width of each lineweight code, and named flags tested.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/lineweight.hpp"

#include "dwgio/entities/polyline.hpp"
#include "dwgio/flags.hpp"

#include <doctest/doctest.h>

namespace DwgIO
{
TEST_CASE("lineweight: each standard code gives its width and other codes none")
{
  CHECK(MillimetersOf(Lineweight::Mm000) == 0.0);
  CHECK(MillimetersOf(Lineweight::Mm025) == doctest::Approx(0.25));
  CHECK(MillimetersOf(Lineweight::Mm211) == doctest::Approx(2.11));
  CHECK_FALSE(MillimetersOf(Lineweight::ByLayer));
  CHECK_FALSE(MillimetersOf(Lineweight::ByBlock));
  CHECK_FALSE(MillimetersOf(Lineweight::Default));
  CHECK_FALSE(MillimetersOf(static_cast<Lineweight>(24)));
}

TEST_CASE("lineweight: Has requires every bit of the flag")
{
  const int flags = 1 | 8;
  CHECK(Has(flags, Entities::PolylineFlags::Closed));
  CHECK(Has(flags, Entities::PolylineFlags::Polyline3d));
  CHECK_FALSE(Has(flags, Entities::PolylineFlags::PolyfaceMesh));
}
} // namespace DwgIO
