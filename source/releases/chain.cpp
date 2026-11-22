/***********************************************************************************************************************
 *  @file      source/releases/chain.cpp
 *  @brief     An owner's list and chain of entities as another release keeps them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/chain.hpp"

#include "releases/lookup.hpp"
#include "releases/passage.hpp"

#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/block.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  From First through each Next to Last: what a cut chain reaches, nothing of a loop twice, nothing owned by another.
 */
auto WalkChain(const Lookup &found, const Ownership &owning) -> std::vector<Handle>
{
  std::vector<Handle> reached;
  std::unordered_set<std::uint64_t> seen;
  for (Handle next = *owning.First; next;)
  {
    const Object *object = found.Of(next);
    const Common *common = object == nullptr ? nullptr : CommonOf(*object);
    if (common == nullptr || BaseOf(*object).Owner != owning.Owner || !seen.insert(next.Value).second)
    {
      break;
    }
    reached.push_back(next);
    if (next == *owning.Last)
    {
      break;
    }
    next = common->Next;
  }
  return reached;
}

void ChainList(std::vector<Handle> &list, Handle &first, Handle &last)
{
  first = list.empty() ? Handle{} : list.front();
  last = list.empty() ? Handle{} : list.back();
  list.clear();
}

/**
 *  The list an object owns entities by, or null: a block record's, an insert's or a polyline's.
 */
auto ListOf(const Object &object) -> const std::vector<Handle> *
{
  return Visit(
      [](const auto &kind) -> const std::vector<Handle> *
      {
        using KindT = std::remove_cvref_t<decltype(kind)>;
        if constexpr (std::is_same_v<KindT, Tables::BlockRecord>)
        {
          return &kind.Entities;
        }
        else if constexpr (std::is_same_v<KindT, Entities::Insert>)
        {
          return &kind.Owned;
        }
        else if constexpr (std::is_same_v<KindT, Entities::Polyline>)
        {
          return &kind.Vertices;
        }
        else
        {
          return nullptr;
        }
      },
      object);
}
} // namespace

void LinkEntity(Common &common, const Base &base, const Lookup &found)
{
  const Object *owner = found.Of(base.Owner);
  const std::vector<Handle> *list = owner == nullptr ? nullptr : ListOf(*owner);
  if (list == nullptr)
  {
    return;
  }
  const std::optional<std::size_t> at = found.Place(base.Owner, *list, base.Self);
  if (!at)
  {
    return;
  }
  common.Previous = *at == 0 ? Handle{} : (*list)[*at - 1];
  common.Next = *at + 1 == list->size() ? Handle{} : (*list)[*at + 1];
}

void ConvertOwned(const Ownership &owning, const Passage &passage, const Crossings &crossing)
{
  if (crossing.R2004Up)
  {
    *owning.List = WalkChain(*passage.Lookup, owning);
    *owning.First = {};
    *owning.Last = {};
    const std::size_t left = passage.Lookup->CountOwned(owning.Owner) - owning.List->size();
    if (left > 0)
    {
      Note(passage, owning.Owner,
           std::format("{} {} of {} not on its chain, left out of its list", left, left == 1 ? "entity" : "entities",
                       passage.What));
    }
  }
  if (crossing.R2004Down)
  {
    ChainList(*owning.List, *owning.First, *owning.Last);
  }
}
} // namespace DwgIO::Releases
