/***********************************************************************************************************************
 *  @file      source/view.cpp
 *  @brief     Read-only lookups in a drawing: blocks, records, layouts and dictionaries.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/view.hpp"

#include "ascii.hpp"

#include "dwgio/common.hpp"
#include "dwgio/index.hpp"

#include <algorithm>
#include <unordered_set>

namespace DwgIO::View
{
namespace
{
/**
 *  Every entity the record owns but its BLOCK and ENDBLK, in file order.
 */
auto CollectOwned(const Drawing &drawing, const Tables::BlockRecord &record) -> std::vector<const Object *>
{
  std::vector<const Object *> entities;
  for (const Object &object : drawing.Objects)
  {
    const Handle self = BaseOf(object).Self;
    if (CommonOf(object) != nullptr && BaseOf(object).Owner == record.Record.Base.Self && self != record.Block &&
        self != record.EndBlk)
    {
      entities.push_back(&object);
    }
  }
  return entities;
}

/**
 *  The chain from First through each Next to Last, or nothing when it is cut, loops, leaves the block, ends before Last
 *  or passes through the BLOCK or ENDBLK.
 */
auto WalkChain(const Index &index, const Tables::BlockRecord &record) -> std::vector<const Object *>
{
  std::vector<const Object *> entities;
  std::unordered_set<std::uint64_t> seen;
  for (Handle next = record.First; next != Handle{};)
  {
    const Object *object = index.Find(next);
    const Common *common = object == nullptr ? nullptr : CommonOf(*object);
    if (common == nullptr || BaseOf(*object).Owner != record.Record.Base.Self || next == record.Block ||
        next == record.EndBlk || !seen.insert(next.Value).second)
    {
      return {};
    }
    entities.push_back(object);
    next = common->Next;
  }
  const Handle reached = entities.empty() ? Handle{} : BaseOf(*entities.back()).Self;
  return reached == record.Last ? entities : std::vector<const Object *>{};
}

/**
 *  The entities of the record's list, from R2004, in its order; a handle naming no entity is passed over.
 */
auto CollectListed(const Index &index, const Tables::BlockRecord &record) -> std::vector<const Object *>
{
  std::vector<const Object *> entities;
  for (const Handle one : record.Entities)
  {
    const Object *object = index.Find(one);
    if (object != nullptr && CommonOf(*object) != nullptr)
    {
      entities.push_back(object);
    }
  }
  return entities;
}

auto ListOf(const Entities::Insert &insert) -> const std::vector<Handle> &
{
  return insert.Owned;
}

auto ListOf(const Entities::Polyline &polyline) -> const std::vector<Handle> &
{
  return polyline.Vertices;
}

/**
 *  The members of a sequence: the head's list from R2004, the chain from First to Last before.
 */
template <class MemberT, class HeadT>
auto Sequence(const Index &index, const HeadT &head) -> std::vector<const MemberT *>
{
  const std::vector<Handle> &listed = ListOf(head);
  std::vector<const MemberT *> members;
  for (const Handle one : listed)
  {
    if (const auto *member = GetIf<MemberT>(index.Find(one)))
    {
      members.push_back(member);
    }
  }
  std::unordered_set<std::uint64_t> seen;
  for (Handle at = head.First; at && listed.empty() && seen.insert(at.Value).second;)
  {
    const auto *member = GetIf<MemberT>(index.Find(at));
    if (member == nullptr)
    {
      break;
    }
    members.push_back(member);
    at = at == head.Last ? Handle{} : member->Common.Next;
  }
  return members;
}

auto SpaceOf(const Index &index, Handle record) -> const Tables::BlockRecord *
{
  return GetIf<Tables::BlockRecord>(index.Find(record));
}
} // namespace

auto Matches(std::string_view one, std::string_view other) -> bool
{
  return Ascii::Same(one, other);
}

auto Control(const Index &index, Tables::Table table) -> const Tables::Control *
{
  return index.Control(table);
}

auto Control(const Drawing &drawing, Tables::Table table) -> const Tables::Control *
{
  for (const Object &object : drawing.Objects)
  {
    if (const auto *control = GetIf<Tables::Control>(&object); control != nullptr && control->Table == table)
    {
      return control;
    }
  }
  return nullptr;
}

auto Block(const Index &index, std::string_view name) -> const Tables::BlockRecord *
{
  return Record<Tables::BlockRecord>(index, name);
}

auto Block(const Drawing &drawing, std::string_view name) -> const Tables::BlockRecord *
{
  return Block(Index{drawing}, name);
}

auto ModelSpace(const Index &index) -> const Tables::BlockRecord *
{
  return SpaceOf(index, index.Source().Header.BlockRecordModelSpace);
}

auto ModelSpace(const Drawing &drawing) -> const Tables::BlockRecord *
{
  return GetIf<Tables::BlockRecord>(Find(drawing, drawing.Header.BlockRecordModelSpace));
}

auto PaperSpace(const Index &index) -> const Tables::BlockRecord *
{
  return SpaceOf(index, index.Source().Header.BlockRecordPaperSpace);
}

auto PaperSpace(const Drawing &drawing) -> const Tables::BlockRecord *
{
  return GetIf<Tables::BlockRecord>(Find(drawing, drawing.Header.BlockRecordPaperSpace));
}

auto Entities(const Index &index, const Tables::BlockRecord &record) -> std::vector<const Object *>
{
  std::vector<const Object *> listed = CollectListed(index, record);
  if (!listed.empty())
  {
    return listed;
  }
  const std::vector<const Object *> chained = WalkChain(index, record);
  return chained.empty() ? CollectOwned(index.Source(), record) : chained;
}

auto Entities(const Drawing &drawing, const Tables::BlockRecord &record) -> std::vector<const Object *>
{
  return Entities(Index{drawing}, record);
}

auto Attributes(const Index &index, const Entities::Insert &insert) -> std::vector<const Entities::Attrib *>
{
  return Sequence<Entities::Attrib>(index, insert);
}

auto Attributes(const Drawing &drawing, const Entities::Insert &insert) -> std::vector<const Entities::Attrib *>
{
  return Attributes(Index{drawing}, insert);
}

auto Vertices(const Index &index, const Entities::Polyline &polyline) -> std::vector<const Entities::Vertex *>
{
  return Sequence<Entities::Vertex>(index, polyline);
}

auto Vertices(const Drawing &drawing, const Entities::Polyline &polyline) -> std::vector<const Entities::Vertex *>
{
  return Vertices(Index{drawing}, polyline);
}

auto Layouts(const Index &index) -> std::vector<const Objects::Layout *>
{
  std::vector<const Objects::Layout *> layouts;
  const Objects::Dictionary *dictionary = Dictionary(index, "ACAD_LAYOUT");
  if (dictionary == nullptr)
  {
    return layouts;
  }
  for (const Objects::Entry &entry : dictionary->Entries)
  {
    if (const auto *layout = GetIf<Objects::Layout>(index.Find(entry.Object)))
    {
      layouts.push_back(layout);
    }
  }
  std::ranges::stable_sort(layouts, {}, &Objects::Layout::Order);
  return layouts;
}

auto Layouts(const Drawing &drawing) -> std::vector<const Objects::Layout *>
{
  return Layouts(Index{drawing});
}

auto Layout(const Index &index, std::string_view name) -> const Objects::Layout *
{
  for (const Objects::Layout *layout : Layouts(index))
  {
    if (Matches(layout->Name, name))
    {
      return layout;
    }
  }
  return nullptr;
}

auto Layout(const Drawing &drawing, std::string_view name) -> const Objects::Layout *
{
  return Layout(Index{drawing}, name);
}

auto Root(const Index &index) -> const Objects::Dictionary *
{
  return GetIf<Objects::Dictionary>(index.Find(index.Source().Header.DictionaryNamedObjects));
}

auto Root(const Drawing &drawing) -> const Objects::Dictionary *
{
  return GetIf<Objects::Dictionary>(Find(drawing, drawing.Header.DictionaryNamedObjects));
}

auto Entry(const Index &index, const Objects::Dictionary &dictionary, std::string_view name) -> const Object *
{
  for (const Objects::Entry &entry : dictionary.Entries)
  {
    if (Matches(entry.Name, name))
    {
      return index.Find(entry.Object);
    }
  }
  return nullptr;
}

auto Entry(const Drawing &drawing, const Objects::Dictionary &dictionary, std::string_view name) -> const Object *
{
  for (const Objects::Entry &entry : dictionary.Entries)
  {
    if (Matches(entry.Name, name))
    {
      return Find(drawing, entry.Object);
    }
  }
  return nullptr;
}

auto Dictionary(const Index &index, std::string_view name) -> const Objects::Dictionary *
{
  const Objects::Dictionary *root = Root(index);
  return root == nullptr ? nullptr : GetIf<Objects::Dictionary>(Entry(index, *root, name));
}

auto Dictionary(const Drawing &drawing, std::string_view name) -> const Objects::Dictionary *
{
  const Objects::Dictionary *root = Root(drawing);
  return root == nullptr ? nullptr : GetIf<Objects::Dictionary>(Entry(drawing, *root, name));
}
} // namespace DwgIO::View
