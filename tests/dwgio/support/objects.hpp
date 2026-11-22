/***********************************************************************************************************************
 *  @file      tests/dwgio/support/objects.hpp
 *  @brief     What every object test looks up in the DXF twin and the drawing: records, entries, names.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_OBJECTS_HPP
#define DWGIO_TESTS_SUPPORT_OBJECTS_HPP

#include "support/twin.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Objects
{
/**
 *  The record of the twin, in any section, whose 5 group is that handle.
 */
inline auto FindHandled(const std::vector<Pair> &twin, Handle self) -> std::optional<std::size_t>
{
  for (std::size_t at = 0; at < twin.size(); ++at)
  {
    if (twin[at].Code == 0 && twin[at].Value != "SECTION" && twin[at].Value != "ENDSEC" && Self(twin, at) == self)
    {
      return at;
    }
  }
  return std::nullopt;
}

/**
 *  A dictionary's entries in the twin: each 3 group with the 350 or 360 that follows it.
 */
inline auto Entries(const std::vector<Pair> &twin, std::size_t opened) -> std::vector<std::pair<std::string, Handle>>
{
  std::vector<std::pair<std::string, Handle>> entries;
  for (std::size_t at = opened + 1; at + 1 < twin.size() && twin[at].Code != 0; ++at)
  {
    if (twin[at].Code == 3 && (twin[at + 1].Code == 350 || twin[at + 1].Code == 360))
    {
      entries.emplace_back(twin[at].Value, ParseHandle(twin[at + 1].Value).value_or(Handle{}));
    }
  }
  return entries;
}

/**
 *  Whether two names agree, in any case for an R14 drawing, which keeps names in upper case.
 */
inline auto SameName(std::string_view one, std::string_view other, const Drawing &drawing) -> bool
{
  return drawing.Release >= Version::R2000 ? one == other : Upper(one) == Upper(other);
}

/**
 *  The entry of a dictionary with that name, in the drawing's case, or null.
 */
template <typename DictionaryT>
auto EntryNamed(const DictionaryT &dictionary, std::string_view name, const Drawing &drawing) -> const Handle *
{
  for (const auto &entry : dictionary.Entries)
  {
    if (SameName(entry.Name, name, drawing))
    {
      return &entry.Object;
    }
  }
  return nullptr;
}

/**
 *  The object of the drawing with that handle, or null.
 */
inline auto FindObject(const Drawing &drawing, Handle self) -> const Object *
{
  const auto found = std::ranges::find(drawing.Objects, self, [](const Object &one) { return BaseOf(one).Self; });
  return found == drawing.Objects.end() ? nullptr : &*found;
}
} // namespace DwgIO::Objects

#endif /* DWGIO_TESTS_SUPPORT_OBJECTS_HPP */
