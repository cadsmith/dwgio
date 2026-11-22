/***********************************************************************************************************************
 *  @file      source/releases/guid.cpp
 *  @brief     An ACIS entity's GUID kept in extended data below R2013.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/guid.hpp"

#include "encoding/guid.hpp"
#include "entities/shapes/geometry.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/geometry.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The GUID's parts as its text has them: a long, two shorts and eight bytes, where they start and how many digits.
 */
constexpr std::size_t Parts = 11;
constexpr std::array<std::size_t, Parts> Starts{1, 10, 15, 20, 22, 25, 27, 29, 31, 33, 35};
constexpr std::array<std::size_t, Parts> Digits{8, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2};
/**
 *  The groups of the extended data: a short of 1, then the parts, the long as a long and the rest as shorts.
 */
constexpr int ShortCode = 1070;
constexpr int LongCode = 1071;
constexpr std::size_t Groups = Parts + 1;

auto ParseGuid(std::string_view text) -> std::optional<std::array<std::uint32_t, Parts>>
{
  std::array<std::uint32_t, Parts> parts{};
  if (text.size() != Entities::Shapes::NoGuid.size())
  {
    return std::nullopt;
  }
  for (std::size_t at = 0; at < Parts; ++at)
  {
    const std::optional<std::uint32_t> part = Entities::Shapes::ParseHex(text, Starts.at(at), Digits.at(at));
    if (!part)
    {
      return std::nullopt;
    }
    parts.at(at) = *part;
  }
  return parts;
}

/**
 *  The extended data's values, when it is the GUID's: twelve integers, the first 1.
 */
auto ValuesOf(const Xdata &xdata) -> std::optional<std::array<std::int64_t, Groups>>
{
  std::array<std::int64_t, Groups> values{};
  if (xdata.Tags.size() != Groups)
  {
    return std::nullopt;
  }
  for (std::size_t at = 0; at < Groups; ++at)
  {
    const auto *value = std::get_if<std::int64_t>(&xdata.Tags[at].Value);
    if (value == nullptr)
    {
      return std::nullopt;
    }
    values.at(at) = *value;
  }
  return values;
}
} // namespace

auto HasGuid(const Entities::Geometry &geometry) -> bool
{
  return !geometry.Guid.empty() && geometry.Guid != Entities::Shapes::NoGuid;
}

void KeepGuid(Base &base, const Entities::Geometry &geometry, const Drawing &drawing)
{
  const auto *application = View::Record<Tables::AppId>(drawing, GuidApplication);
  const std::optional<std::array<std::uint32_t, Parts>> parts = ParseGuid(geometry.Guid);
  if (application == nullptr || !HasGuid(geometry) || !parts)
  {
    return;
  }
  const Handle appid = application->Record.Base.Self;
  std::erase_if(base.Extended, [appid](const Xdata &xdata) -> bool { return xdata.AppId == appid; });
  std::vector<Tag> groups{Tag{.Code = ShortCode, .Value = std::int64_t{1}},
                          Tag{.Code = LongCode, .Value = std::int64_t{static_cast<std::int32_t>((*parts)[0])}}};
  for (std::size_t at = 1; at < Parts; ++at)
  {
    // The shorts are signed; the bytes are not.
    const std::int64_t value = at < 3 ? std::int64_t{static_cast<std::int16_t>(parts->at(at))} : parts->at(at);
    groups.push_back(Tag{.Code = ShortCode, .Value = value});
  }
  base.Extended.push_back(Xdata{.Application = {}, .AppId = appid, .Tags = std::move(groups)});
}

auto TakeGuid(Base &base, Entities::Geometry &geometry, const Drawing &drawing) -> bool
{
  const auto *application = View::Record<Tables::AppId>(drawing, GuidApplication);
  if (application == nullptr)
  {
    return false;
  }
  const Handle appid = application->Record.Base.Self;
  const auto found = std::ranges::find(base.Extended, appid, &Xdata::AppId);
  const std::optional<std::array<std::int64_t, Groups>> values =
      found == base.Extended.end() ? std::nullopt : ValuesOf(*found);
  if (!values)
  {
    return false;
  }
  // The values after the 1: the long, the two shorts and the eight bytes.
  const std::array<std::int64_t, Groups> &all = *values;
  Encoding::Guid guid{.Data1 = static_cast<std::uint32_t>(all[1]),
                      .Data2 = static_cast<std::uint16_t>(all[2]),
                      .Data3 = static_cast<std::uint16_t>(all[3]),
                      .Data4 = {}};
  for (std::size_t at = 0; at < guid.Data4.size(); ++at)
  {
    guid.Data4.at(at) = static_cast<std::uint8_t>(all.at(at + 4));
  }
  geometry.Guid = Encoding::FormatGuid(guid, false);
  base.Extended.erase(found);
  return true;
}
} // namespace DwgIO::Releases
