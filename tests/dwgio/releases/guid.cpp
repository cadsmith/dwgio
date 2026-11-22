/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/guid.cpp
 *  @brief     An entity's GUID kept in extended data below R2013 and taken back on upgrading.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/guid.hpp"

#include "entities/shapes/geometry.hpp"

#include "dwgio/base.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/geometry.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
constexpr const char *Guid = "{1b628c31-6140-d449-b88d-b68a5d455c52}";

/**
 *  A drawing registering the GUID's application, and its record's handle.
 */
auto Registering() -> std::pair<Drawing, Handle>
{
  Drawing drawing = Create(Version::R2013);
  Tables::AppId appid;
  appid.Record.Name = std::string{GuidApplication};
  const std::expected<Handle, Failure> added = Edit::Add(drawing, appid);
  REQUIRE(added);
  return {drawing, *added};
}
} // namespace

TEST_CASE("releases/guid: a GUID moves into its application's xdata below R2013 and back")
{
  const auto [drawing, appid] = Registering();
  Entities::Geometry geometry;
  geometry.Guid = Guid;
  Base base;
  base.Extended.push_back(Xdata{.Application = {}, .AppId = appid, .Tags = {}});
  base.Extended.push_back(Xdata{.Application = {}, .AppId = Handle{0x12}, .Tags = {}});
  KeepGuid(base, geometry, drawing);
  REQUIRE(base.Extended.size() == 2);
  const Xdata &kept = base.Extended.back();
  CHECK(kept.AppId == appid);
  const std::vector<std::int64_t> values{1, 459443249, 24896, -11191, 184, 141, 182, 138, 93, 69, 92, 82};
  REQUIRE(kept.Tags.size() == values.size());
  for (std::size_t at = 0; at < values.size(); ++at)
  {
    CHECK(kept.Tags.at(at).Code == (at == 1 ? 1071 : 1070));
    CHECK(std::get<std::int64_t>(kept.Tags.at(at).Value) == values.at(at));
  }
  Entities::Geometry upgraded;
  CHECK(TakeGuid(base, upgraded, drawing));
  CHECK(upgraded.Guid == Guid);
  CHECK(base.Extended.size() == 1);
  CHECK_FALSE(TakeGuid(base, upgraded, drawing));
}

TEST_CASE("releases/guid: no GUID or a broken one or no application gives nothing and only the data comes back")
{
  const auto [drawing, appid] = Registering();
  Entities::Geometry geometry;
  CHECK_FALSE(HasGuid(geometry));
  geometry.Guid = Entities::Shapes::NoGuid;
  CHECK_FALSE(HasGuid(geometry));
  Base base;
  KeepGuid(base, geometry, drawing);
  for (const std::string broken : {"{1b628c31}", "{1b628c31-6140-d449-b88d-b68a5d455cxx}"})
  {
    geometry.Guid = broken;
    CHECK(HasGuid(geometry));
    KeepGuid(base, geometry, drawing);
  }
  CHECK(base.Extended.empty());
  geometry.Guid = Guid;
  KeepGuid(base, geometry, Create(Version::R2013));
  CHECK(base.Extended.empty());
  // Extended data of the application that is not the GUID's stays.
  base.Extended.push_back(
      Xdata{.Application = {}, .AppId = appid, .Tags = {Tag{.Code = 1070, .Value = std::int64_t{1}}}});
  CHECK_FALSE(TakeGuid(base, geometry, drawing));
  base.Extended.front().Tags.resize(12, Tag{.Code = 1000, .Value = std::string{"x"}});
  CHECK_FALSE(TakeGuid(base, geometry, drawing));
  CHECK_FALSE(TakeGuid(base, geometry, Create(Version::R2013)));
  CHECK(base.Extended.size() == 1);
}
} // namespace DwgIO::Releases
