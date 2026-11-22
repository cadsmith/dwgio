/***********************************************************************************************************************
 *  @file      tests/dwgio/xdata.cpp
 *  @brief     An extended data record's application and groups.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <string>

namespace DwgIO
{
TEST_CASE("xdata: xdata holds an application's name and its groups")
{
  const Xdata acad{.Application = "ACAD", .Tags = {Tag{.Code = 1000, .Value = std::string{"x"}}}};
  CHECK(acad == acad);
  CHECK_FALSE(acad == Xdata{.Application = "ACAD", .Tags = {}});
}
} // namespace DwgIO
