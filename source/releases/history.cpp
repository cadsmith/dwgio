/***********************************************************************************************************************
 *  @file      source/releases/history.cpp
 *  @brief     A 3DSOLID's history, kept in a record below R2007.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/history.hpp"

#include "dwgio/entities/modeler.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tag.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
constexpr std::string_view History = "ACAD_SOLID_HIST";
constexpr std::string_view Long = "ACAD_SOLID_HIST1";
constexpr std::string_view Self = "ACAD_SOLID_HIST2";

constexpr int Marker = 102;
constexpr int Owned = 360;
constexpr int Whole = 90;

/**
 *  The marker's name, or empty for a group that is none.
 */
auto MarkerOf(const Tag &group) -> std::string_view
{
  const auto *text = std::get_if<std::string>(&group.Value);
  return group.Code == Marker && text != nullptr ? std::string_view{*text} : std::string_view{};
}
} // namespace

auto MakeHistoryRecord(const Entities::Solid3d &solid) -> std::vector<Tag>
{
  return {Tag{.Code = Marker, .Value = std::string{History}},
          Tag{.Code = Owned, .Value = solid.History},
          Tag{.Code = Marker, .Value = std::string{Long}},
          Tag{.Code = Whole, .Value = std::int64_t{solid.Geometry.Unknown1}},
          Tag{.Code = Marker, .Value = std::string{Self}},
          Tag{.Code = Whole, .Value = static_cast<std::int64_t>(solid.Base.Self.Value)}};
}

void RestoreHistory(Entities::Solid3d &solid, std::vector<Tag> &record)
{
  std::vector<Tag> rest;
  for (std::size_t at = 0; at < record.size(); ++at)
  {
    const std::string_view marker = MarkerOf(record[at]);
    if (marker != History && marker != Long && marker != Self)
    {
      rest.push_back(std::move(record[at]));
      continue;
    }
    if (at + 1 == record.size())
    {
      break;
    }
    const Tag &value = record[++at];
    if (const auto *handle = std::get_if<Handle>(&value.Value); handle != nullptr && marker == History)
    {
      solid.History = *handle;
    }
    else if (const auto *whole = std::get_if<std::int64_t>(&value.Value); whole != nullptr && marker == Long)
    {
      solid.Geometry.Unknown1 = static_cast<std::int32_t>(*whole);
    }
  }
  record = std::move(rest);
}
} // namespace DwgIO::Releases
