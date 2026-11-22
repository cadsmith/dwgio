/***********************************************************************************************************************
 *  @file      source/edit/cached.cpp
 *  @brief     The ledger that keeps its answers for a run of edits.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "edit/cached.hpp"

#include "ascii.hpp"
#include "making/records.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Edit
{
namespace
{
auto Capitals(std::string_view name) -> std::string
{
  std::string capitals{name};
  std::ranges::transform(capitals, capitals.begin(), Ascii::Upper);
  return capitals;
}
} // namespace

CachedLedger::CachedLedger(Drawing &drawing) : Ledger{drawing}
{
  const std::vector<Object> &objects = drawing.Objects;
  m_Places.reserve(objects.size());
  for (std::size_t at = 0; at < objects.size(); ++at)
  {
    Register(at);
    const auto *control = GetIf<Tables::Control>(&objects[at]);
    if (control != nullptr && !m_Controls.at(static_cast<std::size_t>(control->Table)))
    {
      m_Controls.at(static_cast<std::size_t>(control->Table)) = at;
    }
  }
  for (std::size_t table = 0; table < Tables::Count; ++table)
  {
    const Tables::Control *control = Control(static_cast<Tables::Table>(table));
    if (control == nullptr)
    {
      continue;
    }
    std::vector<Handle> order{control->ModelSpace, control->PaperSpace, control->ByBlock, control->ByLayer};
    order.insert(order.end(), control->Entries.begin(), control->Entries.end());
    for (const Handle self : order)
    {
      const Object *object = Find(self);
      if (object != nullptr && Making::TableIn(*object) == static_cast<Tables::Table>(table))
      {
        m_Names.at(table).try_emplace(Capitals(Making::NameIn(*object)), self);
      }
    }
  }
}

auto CachedLedger::Find(Handle self) -> Object *
{
  const auto found = m_Places.find(self.Value);
  return !self || found == m_Places.end() ? nullptr : &Target().Objects[found->second];
}

auto CachedLedger::MakeHandle() -> Handle
{
  Handle &seed = Target().Header.Handseed;
  const Handle given = std::max(seed, m_Top);
  seed = Handle{.Value = given.Value + 1};
  return given;
}

auto CachedLedger::FindRecord(Tables::Table table, std::string_view name) -> Object *
{
  const auto &names = m_Names.at(static_cast<std::size_t>(table));
  const auto found = names.find(Capitals(name));
  return found == names.end() ? nullptr : Find(found->second);
}

auto CachedLedger::Control(Tables::Table table) -> Tables::Control *
{
  const std::optional<std::size_t> at = m_Controls.at(static_cast<std::size_t>(table));
  return at ? GetIf<Tables::Control>(&Target().Objects[*at]) : nullptr;
}

void CachedLedger::Put(std::size_t at, Object object)
{
  std::vector<Object> &objects = Target().Objects;
  objects.insert(objects.begin() + static_cast<std::ptrdiff_t>(at), std::move(object));
  // From the end, so that a handle two objects share moves with the first of them only.
  for (std::size_t place = objects.size() - 1; place > at; --place)
  {
    const auto found = m_Places.find(BaseOf(objects[place]).Self.Value);
    if (found != m_Places.end() && found->second == place - 1)
    {
      found->second = place;
    }
  }
  for (std::optional<std::size_t> &control : m_Controls)
  {
    if (control && *control >= at)
    {
      ++*control;
    }
  }
  Register(at);
  // An object put is a record only as ListRecord puts it, last among its control's entries.
  if (const std::optional<Tables::Table> table = Making::TableIn(objects[at]))
  {
    m_Names.at(static_cast<std::size_t>(*table))
        .try_emplace(Capitals(Making::NameIn(objects[at])), BaseOf(objects[at]).Self);
  }
}

void CachedLedger::Register(std::size_t at)
{
  const Handle self = BaseOf(Target().Objects[at]).Self;
  m_Top = std::max(m_Top, Handle{.Value = self.Value + 1});
  if (self)
  {
    m_Places.try_emplace(self.Value, at);
  }
}
} // namespace DwgIO::Edit
