/***********************************************************************************************************************
 *  @file      source/audit/common.cpp
 *  @brief     What the audit's checks share: findings, lookups, chains and erasure.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "audit/common.hpp"

#include "ascii.hpp"
#include "audit/checks.hpp"
#include "making/records.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Audit
{
void NoteFailure(Report &report, std::string what, Handle subject)
{
  NoteFixed(report, Finding{.Level = Severity::Error, .What = std::move(what), .Subject = subject});
}

void NoteWarning(Report &report, std::string what, Handle subject)
{
  NoteFixed(report, Finding{.Level = Severity::Warning, .What = std::move(what), .Subject = subject});
}

auto ReserveHandle(Drawing &drawing, Handle kept) -> Handle
{
  if (!kept || Find(drawing, kept) != nullptr)
  {
    return Edit::MakeHandle(drawing);
  }
  Handle &seed = drawing.Header.Handseed;
  seed = std::max(seed, Handle{.Value = kept.Value + 1});
  return kept;
}

Index::Index(Drawing &drawing)
{
  for (Object &object : drawing.Objects)
  {
    m_At.try_emplace(BaseOf(object).Self.Value, &object);
  }
}

auto Index::At(Handle handle) const -> Object *
{
  const auto found = m_At.find(handle.Value);
  return found == m_At.end() ? nullptr : found->second;
}

auto DictionaryAt(Drawing &drawing, Handle self) -> Objects::Dictionary *
{
  Object *object = Find(drawing, self);
  return object == nullptr ? nullptr : GetIf<Objects::Dictionary>(object);
}

void SetEntry(Objects::Dictionary &dictionary, std::string_view name, Handle object)
{
  for (Objects::Entry &entry : dictionary.Entries)
  {
    if (Ascii::Same(entry.Name, name))
    {
      entry.Object = object;
      return;
    }
  }
  dictionary.Entries.push_back(Objects::Entry{.Name = std::string{name}, .Object = object});
}

auto Present(const Drawing &drawing, Tables::Table table, std::string_view name) -> Handle
{
  for (const Object &object : drawing.Objects)
  {
    if (Ascii::Same(Making::NameIn(object), name) && Making::TableIn(object) == table)
    {
      return BaseOf(object).Self;
    }
  }
  return {};
}

auto MakeLayout(Drawing &drawing, Handle record, bool model) -> Handle
{
  const Handle layouts = drawing.Header.DictionaryLayouts;
  const Handle self = Edit::MakeHandle(drawing);
  Objects::Layout layout =
      Making::LayoutOf(Making::MakeReactingBase(self, layouts), model ? "Model" : "Layout1", record);
  layout.Order = model ? 0 : 1;
  if (model)
  {
    layout.Plot = Making::ModelPlot();
  }
  SetEntry(*DictionaryAt(drawing, layouts), layout.Name, self);
  drawing.Objects.emplace_back(std::move(layout));
  return self;
}

namespace
{
/**
 *  Whether an entity is a block's frame or a member of a sequence, so no link of a block's chain; a SEQEND, which is
 *  neither, CollectOwned skips before asking.
 */
auto IsFraming(const Object &object) -> bool
{
  return Holds<Entities::Block>(object) || Holds<Entities::EndBlk>(object) || Holds<Entities::Vertex>(object) ||
         Holds<Entities::Attrib>(object);
}

/**
 *  Closes the chain of an entity over it: its neighbours link past it, its owner's ends move off it. From R2004, which
 *  keeps no chain, its owner's list drops it instead.
 */
void Unlink(const Index &index, const Object &object, bool listed)
{
  const Common *common = CommonOf(object);
  if (common == nullptr)
  {
    return;
  }
  const Handle self = BaseOf(object).Self;
  if (Object *before = index.At(common->Previous); !listed && before != nullptr && CommonOf(*before) != nullptr)
  {
    CommonOf(*before)->Next = common->Next;
  }
  if (Object *after = index.At(common->Next); !listed && after != nullptr && CommonOf(*after) != nullptr)
  {
    CommonOf(*after)->Previous = common->Previous;
  }
  Object *owner = index.At(BaseOf(object).Owner);
  if (owner == nullptr)
  {
    return;
  }
  Visit(
      [&](auto &kind) -> void
      {
        if constexpr (requires {
                        kind.First;
                        kind.Last;
                      })
        {
          if (!listed)
          {
            kind.First = kind.First == self ? common->Next : kind.First;
            kind.Last = kind.Last == self ? common->Previous : kind.Last;
          }
        }
        if constexpr (requires { kind.Entities; })
        {
          std::erase(kind.Entities, self);
        }
        if constexpr (requires { kind.Owned; })
        {
          std::erase(kind.Owned, self);
        }
        if constexpr (requires { std::ranges::find(kind.Vertices, self); }) // A mesh's vertices are points.
        {
          std::erase(kind.Vertices, self);
        }
      },
      *owner);
}
} // namespace

void EraseObjects(Drawing &drawing, const std::unordered_set<std::uint64_t> &removed)
{
  if (removed.empty())
  {
    return;
  }
  const auto named = [&removed](Handle handle) -> bool
  {
    return removed.contains(handle.Value);
  };
  const Index index{drawing};
  const bool listed = drawing.Release >= Version::R2004;
  for (const Object &object : drawing.Objects)
  {
    if (named(BaseOf(object).Self))
    {
      Unlink(index, object, listed);
    }
  }
  std::erase_if(drawing.Objects, [&named](const Object &object) -> bool { return named(BaseOf(object).Self); });
  for (Object &object : drawing.Objects)
  {
    if (auto *dictionary = GetIf<Objects::Dictionary>(&object))
    {
      std::erase_if(dictionary->Entries, [&named](const Objects::Entry &entry) -> bool { return named(entry.Object); });
    }
    else if (auto *control = GetIf<Tables::Control>(&object))
    {
      std::erase_if(control->Entries, named);
    }
  }
}

auto CollectOwned(Drawing &drawing, Handle owner) -> std::vector<Object *>
{
  return CollectChains(drawing, false)[owner.Value];
}

auto Walks(const Index &index, const Chain &chain, const std::vector<Object *> &owned) -> bool
{
  std::unordered_set<std::uint64_t> expected;
  for (const Object *object : owned)
  {
    expected.insert(BaseOf(*object).Self.Value);
  }
  std::unordered_set<std::uint64_t> seen;
  if (chain.List != nullptr)
  {
    for (const Handle one : *chain.List)
    {
      if (!expected.contains(one.Value) || !seen.insert(one.Value).second)
      {
        return false;
      }
    }
    return seen.size() == expected.size();
  }
  Handle previous;
  for (Handle at = *chain.First; at;)
  {
    const Object *object = index.At(at);
    const Common *common = object == nullptr ? nullptr : CommonOf(*object);
    if (common == nullptr || !expected.contains(at.Value) || !seen.insert(at.Value).second ||
        common->Previous != previous)
    {
      return false;
    }
    previous = at;
    at = common->Next;
  }
  return seen.size() == expected.size() && previous == *chain.Last;
}

void Relink(const Chain &chain, const std::vector<Object *> &owned)
{
  if (chain.List != nullptr)
  {
    chain.List->clear();
    for (Object *object : owned)
    {
      chain.List->push_back(BaseOf(*object).Self);
      CommonOf(*object)->Previous = {};
      CommonOf(*object)->Next = {};
    }
    *chain.First = {};
    *chain.Last = {};
    return;
  }
  Object *before = nullptr;
  for (Object *object : owned)
  {
    Common &common = *CommonOf(*object);
    common.Previous = before == nullptr ? Handle{} : BaseOf(*before).Self;
    common.Next = {};
    if (before != nullptr)
    {
      CommonOf(*before)->Next = BaseOf(*object).Self;
    }
    before = object;
  }
  *chain.First = owned.empty() ? Handle{} : BaseOf(*owned.front()).Self;
  *chain.Last = before == nullptr ? Handle{} : BaseOf(*before).Self;
}

auto CollectChains(Drawing &drawing, bool members) -> std::unordered_map<std::uint64_t, std::vector<Object *>>
{
  std::unordered_map<std::uint64_t, std::vector<Object *>> grouped;
  for (Object &object : drawing.Objects)
  {
    if (CommonOf(object) == nullptr || Holds<Entities::SeqEnd>(object))
    {
      continue;
    }
    const bool member = Holds<Entities::Vertex>(object) || Holds<Entities::Attrib>(object);
    if (members ? member : !IsFraming(object))
    {
      grouped[BaseOf(object).Owner.Value].push_back(&object);
    }
  }
  return grouped;
}

auto Seqends(const Drawing &drawing) -> std::unordered_map<std::uint64_t, Handle>
{
  std::unordered_map<std::uint64_t, Handle> seqends;
  for (const Object &object : drawing.Objects)
  {
    if (Holds<Entities::SeqEnd>(object))
    {
      seqends.try_emplace(BaseOf(object).Owner.Value, BaseOf(object).Self);
    }
  }
  return seqends;
}

auto CollectRoots(Drawing &drawing) -> std::vector<Handle *>
{
  Header &header = drawing.Header;
  std::vector<Handle *> rooted{&header.Clayer,
                               &header.Textstyle,
                               &header.Dimstyle,
                               &header.Celtype,
                               &header.Dimtxsty,
                               &header.LTypeByBlock,
                               &header.LTypeByLayer,
                               &header.LTypeContinuous,
                               &header.BlockRecordModelSpace,
                               &header.BlockRecordPaperSpace};
  for (Object &object : drawing.Objects)
  {
    if (auto *control = GetIf<Tables::Control>(&object))
    {
      rooted.insert(rooted.end(), {&control->ModelSpace, &control->PaperSpace, &control->ByBlock, &control->ByLayer});
    }
  }
  return rooted;
}
} // namespace DwgIO::Audit
