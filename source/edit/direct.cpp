/***********************************************************************************************************************
 *  @file      source/edit/direct.cpp
 *  @brief     The ledger that walks the drawing for each answer.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "edit/direct.hpp"

#include "ascii.hpp"
#include "making/records.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/view.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Edit
{
auto DirectLedger::Find(Handle self) -> Object *
{
  return DwgIO::Find(Target(), self);
}

auto DirectLedger::MakeHandle() -> Handle
{
  Handle &seed = Target().Header.Handseed;
  for (const Object &object : Target().Objects)
  {
    seed = std::max(seed, Handle{.Value = BaseOf(object).Self.Value + 1});
  }
  const Handle given = seed;
  seed = Handle{.Value = seed.Value + 1};
  return given;
}

auto DirectLedger::FindRecord(Tables::Table table, std::string_view name) -> Object *
{
  const Tables::Control *control = Making::ControlIn(Target(), table);
  if (control == nullptr)
  {
    return nullptr;
  }
  std::vector<Handle> order{control->ModelSpace, control->PaperSpace, control->ByBlock, control->ByLayer};
  order.insert(order.end(), control->Entries.begin(), control->Entries.end());
  for (const Handle self : order)
  {
    Object *object = Find(self);
    if (object != nullptr && Making::TableIn(*object) == table && Ascii::Same(Making::NameIn(*object), name))
    {
      return object;
    }
  }
  return nullptr;
}

auto DirectLedger::Control(Tables::Table table) -> Tables::Control *
{
  return Making::ControlIn(Target(), table);
}

void DirectLedger::Put(std::size_t at, Object object)
{
  std::vector<Object> &objects = Target().Objects;
  objects.insert(objects.begin() + static_cast<std::ptrdiff_t>(at), std::move(object));
}
} // namespace DwgIO::Edit
