/***********************************************************************************************************************
 *  @file      source/edit/dependents.cpp
 *  @brief     What a removal takes along with an object.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "edit/dependents.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/viewportheader.hpp"
#include "dwgio/unknown.hpp"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace DwgIO::Edit
{
namespace
{
/**
 *  Whether an object is a DIMASSOC, which binds a dimension to the entities it measures and reacts to each.
 */
auto Associates(const Object &object) -> bool
{
  const auto *unknown = GetIf<Unknown>(&object);
  return unknown != nullptr && unknown->Class.View() == "DIMASSOC";
}
} // namespace

auto CollectRemoved(const Drawing &drawing, Handle self) -> std::unordered_set<std::uint64_t>
{
  std::unordered_set<std::uint64_t> associations;
  for (const Object &object : drawing.Objects)
  {
    if (Associates(object))
    {
      associations.insert(BaseOf(object).Self.Value);
    }
  }
  std::unordered_map<std::uint64_t, std::vector<Handle>> taken;
  for (const Object &object : drawing.Objects)
  {
    const Base &base = BaseOf(object);
    taken[base.Owner.Value].push_back(base.Self);
    if (const auto *header = GetIf<Tables::ViewportEntityHeader>(&object))
    {
      taken[header->Viewport.Value].push_back(base.Self);
    }
    for (const Handle reactor : base.Reactors)
    {
      if (associations.contains(reactor.Value))
      {
        taken[base.Self.Value].push_back(reactor);
      }
    }
  }
  std::unordered_set<std::uint64_t> removed{self.Value};
  std::vector<Handle> open{self};
  while (!open.empty())
  {
    const Handle one = open.back();
    open.pop_back();
    for (const Handle next : taken[one.Value])
    {
      if (removed.insert(next.Value).second)
      {
        open.push_back(next);
      }
    }
  }
  removed.erase(0); // An object without a handle is no object a removal can name.
  return removed;
}
} // namespace DwgIO::Edit
