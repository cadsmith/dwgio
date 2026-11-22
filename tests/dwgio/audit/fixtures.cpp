/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/fixtures.cpp
 *  @brief     Every AutoCAD fixture passes the audit clean.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/load.hpp"

#include <doctest/doctest.h>

#include <expected>
#include <string>

namespace DwgIO
{
TEST_CASE("audit/fixtures: every AutoCAD fixture audits clean")
{
  for (const std::string name : {"tables", "entities", "curves", "objects", "empty"})
  {
    for (const std::string release : {"r2000", "r14"})
    {
      const std::string file = name + "-" + release + ".dwg";
      CAPTURE(file);
      const std::expected<Loaded, Failure> loaded = Load(ReadFixture(file));
      REQUIRE(loaded);
      const Audit::Report checked = Audit::Check(loaded->Drawing);
      for (const Audit::Finding &finding : checked.Findings)
      {
        CAPTURE(finding.What);
        CHECK(false);
      }
      Drawing drawing = loaded->Drawing;
      Audit::Repair(drawing);
      CHECK(drawing == loaded->Drawing);
    }
  }
}
} // namespace DwgIO
