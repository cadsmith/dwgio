/***********************************************************************************************************************
 *  @file      tests/dwgio/library.cpp
 *  @brief     The library's version, as the build gives it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/library.hpp"

#include <doctest/doctest.h>

#include <format>

namespace DwgIO
{
TEST_CASE("library: the version string is the project's three version numbers")
{
  CHECK(LibraryVersion == std::format("{}.{}.{}", DWGIO_VERSION_MAJOR, DWGIO_VERSION_MINOR, DWGIO_VERSION_PATCH));
  CHECK(LibraryVersion == DWGIO_VERSION_STRING);
}
} // namespace DwgIO
