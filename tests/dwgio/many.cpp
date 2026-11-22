/***********************************************************************************************************************
 *  @file      tests/dwgio/many.cpp
 *  @brief     A drawing of many entities, its object map in several sections, in every release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/circle.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
template <typename KindT>
auto CountOf(const Drawing &drawing) -> std::size_t
{
  return static_cast<std::size_t>(
      std::ranges::count_if(drawing.Objects, [](const Object &object) -> bool { return Holds<KindT>(object); }));
}
} // namespace

TEST_CASE("many: a drawing of 1800 entities with a map in several sections loads whole in every release")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("many-{}.dwg", release)));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    CHECK(CountOf<Entities::Line>(loaded->Drawing) == 1200);
    CHECK(CountOf<Entities::Circle>(loaded->Drawing) == 600);
  }
}

TEST_CASE("many: the drawing saves to its own release and loads back equal")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    const std::vector<std::byte> bytes = ReadFixture(std::format("many-{}.dwg", release));
    const Drawing drawing = Load(bytes).value().Drawing;
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    if (release == "r14" || release == "r2000")
    {
      CHECK(saved->Bytes == bytes);
    }
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(back->Drawing == drawing);
  }
}

TEST_CASE("many: erasing half the lines rewrites the map in every release")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    Drawing drawing = Load(ReadFixture(std::format("many-{}.dwg", release))).value().Drawing;
    bool odd = false;
    std::erase_if(drawing.Objects,
                  [&odd](const Object &object) -> bool { return Holds<Entities::Line>(object) && (odd = !odd); });
    Audit::Repair(drawing); // The block's list and chain lose what was erased, as a program that erases them would.
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(CountOf<Entities::Line>(back->Drawing) == 600);
    QueueAudit(saved->Bytes, std::format("dwgio-many-{}-halved.dwg", release));
  }
}

TEST_CASE("many: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
