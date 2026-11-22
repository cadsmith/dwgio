/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/image.cpp
 *  @brief     The image definition, its reactors and the variables objects against their DXF twin.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/image.hpp"

#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/wipeout.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
/**
 *  The value of a group after the record's subclass marker, where the reactors' brace and the owner are behind.
 */
auto Inside(const std::vector<Pair> &twin, std::size_t opened, int code) -> std::string
{
  bool inside = false;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    inside = inside || twin[at].Code == 100;
    if (inside && twin[at].Code == code)
    {
      return twin[at].Value;
    }
  }
  return {};
}

auto Number(const std::vector<Pair> &twin, std::size_t opened, int code) -> double
{
  return std::stod(Inside(twin, opened, code));
}

auto Integer(const std::vector<Pair> &twin, std::size_t opened, int code) -> int
{
  return std::stoi(Inside(twin, opened, code));
}

/**
 *  Counts the kind's objects of the drawing, checking each against its record in the twin.
 */
template <typename KindT, typename CheckT>
auto Each(const Drawing &drawing, const std::vector<Pair> &twin, const CheckT &check) -> std::size_t
{
  std::size_t count = 0;
  for (const Object &object : drawing.Objects)
  {
    const auto *kind = GetIf<KindT>(&object);
    if (kind == nullptr)
    {
      continue;
    }
    ++count;
    CAPTURE(HexOf(kind->Base.Self));
    const std::optional<std::size_t> opened = FindHandled(twin, kind->Base.Self);
    REQUIRE(opened);
    CHECK(kind->Base.Owner == ParseHandle(Entities::Owner(twin, *opened)).value_or(Handle{}));
    check(*kind, *opened);
  }
  return count;
}
} // namespace

TEST_CASE("objects/image: every release's image definition and its reactors and variables match the DXF twin")
{
  for (const std::string_view year : {"14", "2000", "2004", "2007", "2010", "2013", "2018"})
  {
    CAPTURE(year);
    const std::vector<Pair> twin = ReadTwin(std::format("rasters-r{}.dxf", year == "14" ? "2000" : year));
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("rasters-r{}.dwg", year)));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    const auto definition = [&twin](const ImageDef &kind, std::size_t opened) -> void
    {
      CHECK(kind.Version == Integer(twin, opened, 90));
      CHECK(kind.File == Inside(twin, opened, 1));
      CHECK(kind.Size.X == doctest::Approx(Number(twin, opened, 10)));
      CHECK(kind.Size.Y == doctest::Approx(Number(twin, opened, 20)));
      CHECK(kind.Pixel.X == doctest::Approx(Number(twin, opened, 11)));
      CHECK(kind.Pixel.Y == doctest::Approx(Number(twin, opened, 21)));
      CHECK(kind.Loaded == (Integer(twin, opened, 280) == 1));
      CHECK(kind.Units == Integer(twin, opened, 281));
      CHECK(kind.Base.Reactors.size() == 3);
    };
    CHECK(Each<ImageDef>(drawing, twin, definition) == 1);
    const auto reactor = [&twin](const ImageDefReactor &kind, std::size_t opened) -> void
    {
      CHECK(kind.Version == Integer(twin, opened, 90));
      CHECK(kind.Base.Owner == ParseHandle(Inside(twin, opened, 330)).value_or(Handle{}));
    };
    CHECK(Each<ImageDefReactor>(drawing, twin, reactor) == 2);
    const auto variables = [&twin](const RasterVariables &kind, std::size_t opened) -> void
    {
      CHECK(kind.Version == Integer(twin, opened, 90));
      CHECK(kind.Frame == Integer(twin, opened, 70));
      CHECK(kind.Quality == Integer(twin, opened, 71));
      CHECK(kind.Units == Integer(twin, opened, 72));
    };
    CHECK(Each<RasterVariables>(drawing, twin, variables) == 1);
    const auto wipeouts = [&twin](const WipeoutVariables &kind, std::size_t opened) -> void
    {
      CHECK(kind.Frame == Integer(twin, opened, 70));
    };
    CHECK(Each<WipeoutVariables>(drawing, twin, wipeouts) == 1);
    const Object *vars = View::Entry(drawing, *View::Root(drawing), "ACAD_IMAGE_VARS");
    REQUIRE(vars != nullptr);
    CHECK(TypeOf(*vars) == "RASTERVARIABLES");
  }
}
} // namespace DwgIO::Objects
