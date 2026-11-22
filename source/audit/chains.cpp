/***********************************************************************************************************************
 *  @file      source/audit/chains.cpp
 *  @brief     The audit of owners, spaces, block chains and sequences.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "audit/checks.hpp"
#include "audit/common.hpp"
#include "making/records.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/version.hpp"

#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Audit
{
namespace
{
/**
 *  Whether an entity is a member of a sequence or a block's frame, so no link of a block's chain.
 */
auto IsOffChain(const Object &object) -> bool
{
  return Holds<Entities::Block>(object) || Holds<Entities::EndBlk>(object) || Holds<Entities::Vertex>(object) ||
         Holds<Entities::Attrib>(object) || Holds<Entities::SeqEnd>(object);
}

/**
 *  Every entity of its own is owned by a block record, else by model space, at the end of its chain; AutoCAD re-owns
 *  such an entity itself.
 */
void CheckOwners(Drawing &drawing, Report &report)
{
  const Index index{drawing};
  const Handle model = drawing.Header.BlockRecordModelSpace;
  auto &record = *GetIf<Tables::BlockRecord>(index.At(model));
  for (Object &object : drawing.Objects)
  {
    if (CommonOf(object) == nullptr || IsOffChain(object))
    {
      continue;
    }
    Base &base = BaseOf(object);
    const Object *owner = index.At(base.Owner);
    if (owner != nullptr && Holds<Tables::BlockRecord>(*owner))
    {
      continue;
    }
    NoteWarning(
        report,
        std::format("owner {} of {} {} is no block record", HexOf(base.Owner), TypeOf(object), HexOf(base.Self)),
        base.Self);
    base.Owner = model;
    Common &common = *CommonOf(object);
    common.Space = Space::Model;
    if (drawing.Release >= Version::R2004)
    {
      common.Previous = {};
      common.Next = {};
      record.Entities.push_back(base.Self);
      continue;
    }
    Object *last = record.Last ? index.At(record.Last) : nullptr;
    common.Previous = record.Last;
    common.Next = {};
    if (last != nullptr && CommonOf(*last) != nullptr)
    {
      CommonOf(*last)->Next = base.Self;
    }
    else
    {
      record.First = base.Self;
    }
    record.Last = base.Self;
  }
}

/**
 *  Every entity of a block is in the space of its block; AutoCAD opens a drawing with a wrong space code as it is.
 */
void CheckSpaces(Drawing &drawing, Report &report)
{
  const Index index{drawing};
  for (Object &object : drawing.Objects)
  {
    Common *common = CommonOf(object);
    if (common == nullptr)
    {
      continue;
    }
    // The members of a sequence take whatever space code AutoCAD gave them, often not their head's.
    const Base &base = BaseOf(object);
    const Object *owner = index.At(base.Owner);
    if (owner == nullptr || CommonOf(*owner) != nullptr)
    {
      continue;
    }
    const Space expected = Making::SpaceOf(drawing, base.Owner);
    if (common->Space != expected)
    {
      NoteWarning(report, std::format("space of {} {} is not its block's", TypeOf(object), HexOf(base.Self)),
                  base.Self);
      common->Space = expected;
    }
  }
}

/**
 *  Every block record's chain walks what it owns; from R2004 its list names what it owns, each once.
 *
 *  AutoCAD refuses a drawing with a broken chain, and silently opens one whose list names an entity the block does not
 *  own, misses an entity or names one twice.
 */
void CheckBlocks(Drawing &drawing, Report &report)
{
  const Index index{drawing};
  const bool listed = drawing.Release >= Version::R2004;
  std::unordered_map<std::uint64_t, std::vector<Object *>> grouped = CollectChains(drawing, false);
  for (Object &object : drawing.Objects)
  {
    auto *record = GetIf<Tables::BlockRecord>(&object);
    if (record == nullptr)
    {
      continue;
    }
    const Chain chain{.Owner = record->Record.Base.Self,
                      .First = &record->First,
                      .Last = &record->Last,
                      .SeqEnd = nullptr,
                      .List = listed ? &record->Entities : nullptr};
    const std::vector<Object *> &owned = grouped[chain.Owner.Value];
    if (Walks(index, chain, owned))
    {
      continue;
    }
    if (listed)
    {
      NoteWarning(report, std::format("entities of block {} broken", record->Record.Name), chain.Owner);
    }
    else
    {
      NoteFailure(report, std::format("chain of block {} broken", record->Record.Name), chain.Owner);
    }
    Relink(chain, owned);
  }
}

/**
 *  A vertex is owned by a polyline, an attribute by an insert, a SEQEND by either; the rest are left out, as AutoCAD
 *  silently leaves them out.
 */
void CheckOrphans(Drawing &drawing, Report &report)
{
  const Index index{drawing};
  std::unordered_set<std::uint64_t> removed;
  for (const Object &object : drawing.Objects)
  {
    const Base &base = BaseOf(object);
    const Object *owner = index.At(base.Owner);
    const bool polyline = owner != nullptr && Holds<Entities::Polyline>(*owner);
    const bool insert = owner != nullptr && Holds<Entities::Insert>(*owner);
    const char *needs = nullptr;
    if (Holds<Entities::Vertex>(object) && !polyline)
    {
      needs = "polyline";
    }
    else if (Holds<Entities::Attrib>(object) && !insert)
    {
      needs = "insert";
    }
    else if (Holds<Entities::SeqEnd>(object) && !polyline && !insert)
    {
      needs = "sequence";
    }
    if (needs != nullptr)
    {
      NoteWarning(report, std::format("{} {} owned by no {}, left out", TypeOf(object), HexOf(base.Self), needs),
                  base.Self);
      removed.insert(base.Self.Value);
    }
  }
  EraseObjects(drawing, removed);
}

/**
 *  What every sequence check looks at: the objects by handle, the members by head, the SEQENDs by head.
 */
struct Sequences
{
  Audit::Index Index;
  std::unordered_map<std::uint64_t, std::vector<Object *>> Members;
  std::unordered_map<std::uint64_t, Handle> Seqends;
};

/**
 *  Checks the sequence of a head whose members are what it owns, noting a broken chain and a SEQEND that is not the
 *  head's own, and says whether it lacks its SEQEND.
 */
auto CheckSequence(const Sequences &with, Chain chain, Report &report) -> bool
{
  const Object &head = *with.Index.At(chain.Owner);
  const auto owned = with.Members.find(chain.Owner.Value);
  static const std::vector<Object *> none;
  const std::vector<Object *> &members = owned == with.Members.end() ? none : owned->second;
  if (!Walks(with.Index, chain, members))
  {
    NoteWarning(report, std::format("sequence of {} {} broken", TypeOf(head), HexOf(chain.Owner)), chain.Owner);
    Relink(chain, members);
  }
  const auto seqend = with.Seqends.find(chain.Owner.Value);
  if (seqend == with.Seqends.end())
  {
    NoteWarning(report, std::format("{} {} lacks its SEQEND", TypeOf(head), HexOf(chain.Owner)), chain.Owner);
    return true;
  }
  if (*chain.SeqEnd != seqend->second)
  {
    NoteWarning(report, std::format("SEQEND of {} {} is not its own", TypeOf(head), HexOf(chain.Owner)), chain.Owner);
    *chain.SeqEnd = seqend->second;
  }
  return false;
}

/**
 *  Makes the SEQEND a sequence lacks, after its head.
 */
void MakeSeqEnd(Drawing &drawing, Handle head)
{
  Object &owner = *Find(drawing, head);
  Entities::SeqEnd seqend;
  seqend.Base = Making::MakeBase(Edit::MakeHandle(drawing), head);
  seqend.Common = Making::MakeBlockCommon(CommonOf(owner)->Space, CommonOf(owner)->Layer);
  const Handle self = seqend.Base.Self;
  drawing.Objects.emplace_back(std::move(seqend));
  Visit(
      [self](auto &kind) -> void
      {
        if constexpr (requires { kind.SeqEnd; })
        {
          kind.SeqEnd = self;
        }
      },
      *Find(drawing, head));
}

/**
 *  Every polyline's vertices and every insert's attributes follow their head, closed by a SEQEND; an insert says
 *  whether attributes follow it. AutoCAD silently repairs all of it itself.
 */
void CheckSequences(Drawing &drawing, Report &report)
{
  const Sequences with{.Index = Index{drawing}, .Members = CollectChains(drawing, true), .Seqends = Seqends(drawing)};
  const bool listed = drawing.Release >= Version::R2004;
  std::vector<Handle> lacking;
  for (Object &object : drawing.Objects)
  {
    const Handle self = BaseOf(object).Self;
    if (auto *polyline = GetIf<Entities::Polyline>(&object))
    {
      const Chain chain{.Owner = self,
                        .First = &polyline->First,
                        .Last = &polyline->Last,
                        .SeqEnd = &polyline->SeqEnd,
                        .List = listed ? &polyline->Vertices : nullptr};
      if (CheckSequence(with, chain, report))
      {
        lacking.push_back(self);
      }
      continue;
    }
    auto *insert = GetIf<Entities::Insert>(&object);
    if (insert == nullptr)
    {
      continue;
    }
    const bool attributed = with.Members.contains(self.Value);
    if (insert->Follows && !attributed)
    {
      NoteWarning(report, std::format("INSERT {} has no attribute", HexOf(self)), self);
      insert->Follows = false;
      insert->First = {};
      insert->Last = {};
      insert->SeqEnd = {};
      insert->Owned.clear();
      continue;
    }
    if (!insert->Follows)
    {
      if (!attributed)
      {
        continue;
      }
      NoteWarning(report, std::format("INSERT {} has attributes it does not say", HexOf(self)), self);
      insert->Follows = true;
    }
    const Chain chain{.Owner = self,
                      .First = &insert->First,
                      .Last = &insert->Last,
                      .SeqEnd = &insert->SeqEnd,
                      .List = listed ? &insert->Owned : nullptr};
    if (CheckSequence(with, chain, report))
    {
      lacking.push_back(self);
    }
  }
  for (const Handle head : lacking)
  {
    MakeSeqEnd(drawing, head);
  }
}
} // namespace

void Chains(Drawing &drawing, Report &report)
{
  CheckOwners(drawing, report);
  CheckSpaces(drawing, report);
  CheckBlocks(drawing, report);
  CheckOrphans(drawing, report);
  CheckSequences(drawing, report);
}
} // namespace DwgIO::Audit
