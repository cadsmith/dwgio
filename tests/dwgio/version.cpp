/***********************************************************************************************************************
 *  @file      tests/dwgio/version.cpp
 *  @brief     Each release and its version code.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/version.hpp"

#include <doctest/doctest.h>

namespace DwgIO
{
TEST_CASE("version: each release maps to its file code and back")
{
  CHECK(AcadverOf(Version::R14) == "AC1014");
  CHECK(AcadverOf(Version::R2000) == "AC1015");
  CHECK(AcadverOf(Version::R2018) == "AC1032");
  CHECK(NameOf(Version::R2000) == "R2000");
  CHECK(ReleaseOf("AC1015") == Version::R2000);
  CHECK(ReleaseOf("AC1021") == Version::R2007);
  CHECK_FALSE(ReleaseOf("AC1009").has_value());
  CHECK_FALSE(ReleaseOf("AC1015\r").has_value());
}
} // namespace DwgIO
