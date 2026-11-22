/***********************************************************************************************************************
 *  @file      tests/dwgio/ascii.cpp
 *  @brief     ASCII comparison without case.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "ascii.hpp"

#include <doctest/doctest.h>

namespace DwgIO::Ascii
{
TEST_CASE("ascii: letters compare without case and other characters as they are")
{
  CHECK(Same("ansi_1252", "ANSI_1252"));
  CHECK(Same("a{b", "A{B"));
  CHECK_FALSE(Same("a}", "a{"));
  CHECK_FALSE(Same("ab", "a"));
}
} // namespace DwgIO::Ascii
