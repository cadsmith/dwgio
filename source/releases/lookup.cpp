/***********************************************************************************************************************
 *  @file      source/releases/lookup.cpp
 *  @brief     A drawing's objects by handle, for a pass over all of them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/lookup.hpp"

#include "ascii.hpp"

#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
Lookup::Lookup(const DwgIO::Drawing &drawing) : m_Index{drawing}
{
  for (const Object &object : drawing.Objects)
  {
    const Base &base = BaseOf(object);
    if (CommonOf(object) != nullptr && !Holds<Entities::Block>(object) && !Holds<Entities::EndBlk>(object) &&
        !Holds<Entities::SeqEnd>(object))
    {
      ++m_Owned[base.Owner.Value];
    }
  }
}

auto Lookup::Of(Handle self) const -> const Object *
{
  return m_Index.Find(self);
}

auto Lookup::First(const Object &object) const -> bool
{
  return Of(BaseOf(object).Self) == &object;
}

auto Lookup::CountOwned(Handle owner) const -> std::size_t
{
  const auto found = m_Owned.find(owner.Value);
  return found == m_Owned.end() ? 0 : found->second;
}

auto Lookup::Place(Handle owner, const std::vector<Handle> &list, Handle self) const -> std::optional<std::size_t>
{
  auto [places, made] = m_Places.try_emplace(owner.Value);
  if (made)
  {
    places->second.reserve(list.size());
    for (std::size_t at = 0; at < list.size(); ++at)
    {
      places->second.try_emplace(list[at].Value, at);
    }
  }
  const auto found = places->second.find(self.Value);
  return found == places->second.end() ? std::nullopt : std::optional{found->second};
}

auto Lookup::FindNamed(Tables::Table table, std::string_view name,
                       auto (*recordOf)(const Object &)->const Tables::Record *) const -> const Object *
{
  const Tables::Control *control = m_Index.Control(table);
  if (control == nullptr)
  {
    return nullptr;
  }
  const Tables::Control &kept = *control;
  std::vector<Handle> handles = kept.Entries;
  handles.insert(handles.end(), {kept.ModelSpace, kept.PaperSpace, kept.ByBlock, kept.ByLayer});
  for (const Handle handle : handles)
  {
    const Object *object = Of(handle);
    const Tables::Record *record = object == nullptr ? nullptr : recordOf(*object);
    if (record != nullptr && Ascii::Same(record->Name, name))
    {
      return object;
    }
  }
  return nullptr;
}
} // namespace DwgIO::Releases
