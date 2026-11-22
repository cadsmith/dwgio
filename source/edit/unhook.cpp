/***********************************************************************************************************************
 *  @file      source/edit/unhook.cpp
 *  @brief     The references to removed objects taken out of what remains.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "edit/unhook.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/hatch.hpp"
#include "dwgio/entities/leader.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/group.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/sortents.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/viewportheader.hpp"
#include "dwgio/unknown.hpp"

#include <algorithm>
#include <cstddef>
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
 *  What every kind is unhooked with.
 */
struct Removal
{
  const std::unordered_set<std::uint64_t> *Removed;
  /**
   *  The entity header before each viewport entity header.
   */
  std::unordered_map<std::uint64_t, Handle> Before;
  /**
   *  The hatches that lost their boundary, which the entities they were made from react to no more.
   */
  std::unordered_set<std::uint64_t> Unbound;

  [[nodiscard]] auto IsRemoved(Handle handle) const -> bool { return Removed->contains(handle.Value); }
};

template <class KindT>
void Unhook(KindT & /*kind*/, const Removal & /*with*/)
{
}

void Unhook(Objects::Group &group, const Removal &with)
{
  std::erase_if(group.Members, [&with](Handle member) -> bool { return with.IsRemoved(member); });
}

void Unhook(Objects::SortEntsTable &table, const Removal &with)
{
  std::erase_if(table.Entries,
                [&with](const Objects::SortEntsTables::Entry &entry) -> bool { return with.IsRemoved(entry.Entity); });
}

void Unhook(Tables::Control &control, const Removal &with)
{
  std::erase_if(control.RepeatedEntries, [&with](Handle entry) -> bool { return with.IsRemoved(entry); });
}

void Unhook(Tables::BlockRecord &record, const Removal &with)
{
  std::erase_if(record.Inserts, [&with](Handle insert) -> bool { return with.IsRemoved(insert); });
}

void Unhook(Objects::Layout &layout, const Removal &with)
{
  std::erase_if(layout.Viewports, [&with](Handle viewport) -> bool { return with.IsRemoved(viewport); });
  layout.Viewport = with.IsRemoved(layout.Viewport) ? Handle{} : layout.Viewport;
}

void Unhook(Entities::Viewport &viewport, const Removal &with)
{
  std::erase_if(viewport.Frozen, [&with](Handle layer) -> bool { return with.IsRemoved(layer); });
}

void Unhook(Entities::Leader &leader, const Removal &with)
{
  if (with.IsRemoved(leader.Note))
  {
    leader.Note = {};
    leader.Annotation = 3;
  }
}

void Unhook(Entities::Hatch &hatch, Removal &with)
{
  const auto bound = [&with](const Entities::Path &path) -> bool
  {
    return std::ranges::any_of(path.Sources, [&with](Handle source) -> bool { return with.IsRemoved(source); });
  };
  if (!std::ranges::any_of(hatch.Paths, bound))
  {
    return;
  }
  hatch.Associative = false;
  for (Entities::Path &path : hatch.Paths)
  {
    path.Sources.clear();
  }
  with.Unbound.insert(hatch.Base.Self.Value);
}

void Unhook(Tables::ViewportEntityHeader &header, Removal &with)
{
  std::unordered_set<std::uint64_t> passed;
  while (with.IsRemoved(header.Previous))
  {
    if (!passed.insert(header.Previous.Value).second)
    {
      header.Previous = {}; // Removed headers whose chain loops lead nowhere.
      return;
    }
    header.Previous = with.Before[header.Previous.Value];
  }
}

/**
 *  An untyped object whose header changed is written from its own bits under a new header: its bytes as read hold the
 *  old one.
 *
 *  One whose own bits could not be told apart keeps its bytes, and the link with them.
 */
void Rewrite(Object &object)
{
  if (auto *unknown = GetIf<Unknown>(&object); unknown != nullptr && unknown->Body)
  {
    unknown->Bytes.clear();
  }
}
} // namespace

void UnhookRemoved(Drawing &drawing, const std::unordered_set<std::uint64_t> &removed)
{
  Removal with{.Removed = &removed, .Before = {}, .Unbound = {}};
  for (const Object &object : drawing.Objects)
  {
    if (const auto *header = GetIf<Tables::ViewportEntityHeader>(&object))
    {
      with.Before[header->Record.Base.Self.Value] = header->Previous;
    }
  }
  for (Object &object : drawing.Objects)
  {
    Base &base = BaseOf(object);
    if (with.IsRemoved(base.Self))
    {
      continue;
    }
    const std::size_t reactors = base.Reactors.size();
    const Handle dictionary = base.Dictionary;
    std::erase_if(base.Reactors, [&with](Handle reactor) -> bool { return with.IsRemoved(reactor); });
    base.Dictionary = with.IsRemoved(base.Dictionary) ? Handle{} : base.Dictionary;
    Visit([&with](auto &kind) -> void { Unhook(kind, with); }, object);
    if (base.Reactors.size() != reactors || base.Dictionary != dictionary)
    {
      Rewrite(object);
    }
  }
  for (Object &object : drawing.Objects)
  {
    if (std::erase_if(BaseOf(object).Reactors,
                      [&with](Handle reactor) -> bool { return with.Unbound.contains(reactor.Value); }) > 0)
    {
      Rewrite(object);
    }
  }
}
} // namespace DwgIO::Edit
