/***********************************************************************************************************************
 *  @file      source/edit/add.cpp
 *  @brief     The additions of entities, blocks, attributes and records.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "edit/add.hpp"

#include "edit/ledger.hpp"
#include "making/records.hpp"

#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attdef.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/flags.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace DwgIO::Edit
{
namespace
{
constexpr std::array<std::string_view, Tables::Count> Words{
    "block", "layer",    "text style",  "linetype",        "view",
    "ucs",   "viewport", "application", "dimension style", "viewport header"};

/**
 *  The handle an addition takes: a new one, or the one it brings, which the seed moves past, as AutoCAD requires.
 */
auto ClaimHandle(Ledger &ledger, Handle self) -> Handle
{
  if (!self)
  {
    return ledger.MakeHandle();
  }
  Handle &seed = ledger.Target().Header.Handseed;
  seed = std::max(seed, Handle{.Value = self.Value + 1});
  return self;
}

/**
 *  Null when the handle names no block record.
 */
auto RecordAt(Ledger &ledger, Handle self) -> Tables::BlockRecord *
{
  Object *object = ledger.Find(self);
  return object == nullptr ? nullptr : GetIf<Tables::BlockRecord>(object);
}

void AppendObject(Ledger &ledger, Object object)
{
  ledger.Put(ledger.Target().Objects.size(), std::move(object));
}

/**
 *  Notes an entity where AutoCAD notes it besides the chain: an ATTDEF in the flag of its block, from R2000 an insert
 *  in the list of the block it places.
 */
void RegisterEntity(Ledger &ledger, const Object &entity, Tables::BlockRecord &block)
{
  block.Attributes = block.Attributes || Holds<Entities::AttDef>(entity);
  const auto *insert = GetIf<Entities::Insert>(&entity);
  Tables::BlockRecord *placed =
      insert == nullptr || ledger.Target().Release < Version::R2000 ? nullptr : RecordAt(ledger, insert->Block);
  if (placed != nullptr)
  {
    placed->Inserts.push_back(BaseOf(entity).Self);
  }
}

/**
 *  Appends an entity to a block record's chain, or its list from R2004, setting what the file needs, and gives its
 *  handle.
 */
auto Into(Ledger &ledger, Object entity, Handle record) -> std::expected<Handle, Failure>
{
  Drawing &drawing = ledger.Target();
  Common *common = CommonOf(entity);
  if (common == nullptr)
  {
    return Refuse(FailureCode::Refused, std::format("object {} is no entity", TypeOf(entity)));
  }
  Tables::BlockRecord *block = RecordAt(ledger, record);
  Object *last = block->Last ? ledger.Find(block->Last) : nullptr;
  Common *before = last == nullptr ? nullptr : CommonOf(*last);
  if (block->Last && before == nullptr)
  {
    return Refuse(FailureCode::Refused, std::format("chain of block {} cut", block->Record.Name));
  }
  Base &base = BaseOf(entity);
  if (base.Self && ledger.Find(base.Self) != nullptr)
  {
    return Refuse(FailureCode::Exists, std::format("handle {} exists", HexOf(base.Self)));
  }
  base.Self = ClaimHandle(ledger, base.Self);
  base.Owner = record;
  common->Space = Making::SpaceOf(drawing, record);
  if (!common->Layer)
  {
    common->Layer = drawing.Header.Clayer;
  }
  if (!common->Lineweight)
  {
    common->Lineweight = Lineweight::ByLayer; // What a save as R2000 writes for the lineweight of an older release.
  }
  // A text with no style takes $TEXTSTYLE, as AutoCAD repairs an MTEXT that names none.
  Visit(
      [&drawing](auto &kind) -> void
      {
        if constexpr (requires { kind.Lettering.Style; })
        {
          kind.Lettering.Style = kind.Lettering.Style ? kind.Lettering.Style : drawing.Header.Textstyle;
        }
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(kind)>, Entities::MText>)
        {
          kind.Style = kind.Style ? kind.Style : drawing.Header.Textstyle;
        }
      },
      entity);
  RegisterEntity(ledger, entity, *block);
  if (drawing.Release >= Version::R2004)
  {
    // From R2004 the record lists its entities and no chain is kept.
    common->Previous = {};
    common->Next = {};
    block->Entities.push_back(base.Self);
  }
  else
  {
    common->Previous = block->Last;
    common->Next = {};
    if (before != nullptr)
    {
      before->Next = base.Self;
    }
    else
    {
      block->First = base.Self;
    }
    block->Last = base.Self;
  }
  const Handle self = base.Self;
  AppendObject(ledger, std::move(entity));
  return self;
}
} // namespace

auto Add(Ledger &ledger, Object entity, Space space) -> std::expected<Handle, Failure>
{
  if (space == Space::Owner)
  {
    return Refuse(FailureCode::Refused, "owner space refused");
  }
  const bool model = space == Space::Model;
  const Header &header = ledger.Target().Header;
  const Handle record = model ? header.BlockRecordModelSpace : header.BlockRecordPaperSpace;
  if (RecordAt(ledger, record) == nullptr)
  {
    return Refuse(FailureCode::Missing, std::format("{} space missing", model ? "model" : "paper"));
  }
  return Into(ledger, std::move(entity), record);
}

auto Add(Ledger &ledger, Object entity, std::string_view block) -> std::expected<Handle, Failure>
{
  const Object *record = ledger.FindRecord(Tables::Table::Block, block);
  if (record == nullptr)
  {
    return Refuse(FailureCode::Missing, std::format("block {} missing", block));
  }
  return Into(ledger, std::move(entity), BaseOf(*record).Self);
}

auto ListRecord(Ledger &ledger, Tables::Table table, Object record) -> std::expected<Handle, Failure>
{
  Tables::Control *control = ledger.Control(table);
  if (control == nullptr)
  {
    return Refuse(FailureCode::Missing, std::format("{} table missing", WordOf(table)));
  }
  Base &base = BaseOf(record);
  if (base.Self && ledger.Find(base.Self) != nullptr)
  {
    return Refuse(FailureCode::Exists, std::format("handle {} exists", HexOf(base.Self)));
  }
  base.Self = ClaimHandle(ledger, base.Self);
  base.Owner = control->Base.Self;
  const Handle self = base.Self;
  control->Entries.push_back(self);
  AppendObject(ledger, std::move(record));
  return self;
}

auto WordOf(Tables::Table table) -> std::string_view
{
  return Words.at(static_cast<std::size_t>(table));
}

auto AddRecord(Ledger &ledger, Object record) -> std::expected<Handle, Failure>
{
  const std::optional<Tables::Table> found = Making::TableIn(record);
  if (!found || *found == Tables::Table::Block)
  {
    return Refuse(FailureCode::Refused, std::format("{} is no record to add", TypeOf(record)));
  }
  const Tables::Table table = *found;
  const std::string_view name = Making::NameIn(record);
  if (ledger.FindRecord(table, name) != nullptr)
  {
    return Refuse(FailureCode::Exists, std::format("{} {} exists", WordOf(table), name));
  }
  return ListRecord(ledger, table, std::move(record));
}

namespace
{
/**
 *  The words a refusal names a sequence's head and its members by.
 */
template <class HeadT>
struct Sequenced;

template <>
struct Sequenced<Entities::Insert>
{
  static constexpr std::string_view Kind = "insert";
  static constexpr std::string_view Members = "attributes";
};

template <>
struct Sequenced<Entities::Polyline>
{
  static constexpr std::string_view Kind = "polyline";
  static constexpr std::string_view Members = "vertices";
};

auto ListOf(Entities::Insert &insert) -> std::vector<Handle> &
{
  return insert.Owned;
}

auto ListOf(Entities::Polyline &polyline) -> std::vector<Handle> &
{
  return polyline.Vertices;
}

/**
 *  Appends a member to the sequence of an insert or a polyline, closed by the head's SEQEND, made when it has none.
 */
template <class HeadT, class MemberT>
auto Follow(Ledger &ledger, Handle owner, MemberT member) -> std::expected<Handle, Failure>
{
  Drawing &drawing = ledger.Target();
  Object *object = ledger.Find(owner);
  auto *head = GetIf<HeadT>(object);
  if (object == nullptr)
  {
    return Refuse(FailureCode::Missing, std::format("handle {} missing", HexOf(owner)));
  }
  if (head == nullptr)
  {
    return Refuse(FailureCode::Refused,
                  std::format("{} {} is no {}", TypeOf(*object), HexOf(owner), Sequenced<HeadT>::Kind));
  }
  const bool listed = drawing.Release >= Version::R2004;
  Object *last = listed ? nullptr : ledger.Find(head->Last);
  Common *before = last == nullptr ? nullptr : CommonOf(*last);
  if (!listed && head->Last && before == nullptr)
  {
    return Refuse(FailureCode::Damaged,
                  std::format("{} of {} {} cut", Sequenced<HeadT>::Members, TypeOf(*object), HexOf(owner)));
  }
  Base &base = member.Base;
  if (base.Self && ledger.Find(base.Self) != nullptr)
  {
    return Refuse(FailureCode::Exists, std::format("handle {} exists", HexOf(base.Self)));
  }
  base.Self = ClaimHandle(ledger, base.Self);
  base.Owner = owner;
  Common &common = member.Common;
  // A member names its owner, as AutoCAD writes them, whatever space its head is in.
  common.Space = Space::Owner;
  common.Layer = common.Layer ? common.Layer : head->Common.Layer;
  common.Lineweight = common.Lineweight.value_or(Lineweight::ByLayer);
  common.Previous = listed ? Handle{} : head->Last;
  common.Next = {};
  if (listed)
  {
    // From R2004 the head lists its members and no chain is kept.
    ListOf(*head).push_back(base.Self);
  }
  else
  {
    if (before != nullptr)
    {
      before->Next = base.Self;
    }
    else
    {
      head->First = base.Self;
    }
    head->Last = base.Self;
  }
  if constexpr (std::is_same_v<HeadT, Entities::Insert>)
  {
    head->Follows = true;
  }
  const Handle self = base.Self;
  if (const Object *ending = ledger.Find(head->SeqEnd); ending != nullptr)
  {
    // The members come before their SEQEND in the file, as AutoCAD writes them.
    ledger.Put(static_cast<std::size_t>(std::distance(std::as_const(drawing).Objects.data(), ending)),
               Object{std::move(member)});
    return self;
  }
  Entities::SeqEnd seqend;
  seqend.Base = Making::MakeBase(ledger.MakeHandle(), owner);
  seqend.Common = Making::MakeBlockCommon(Space::Owner, head->Common.Layer);
  head->SeqEnd = seqend.Base.Self;
  AppendObject(ledger, Object{std::move(member)});
  AppendObject(ledger, Object{std::move(seqend)});
  return self;
}

/**
 *  The bits of a vertex that pick the type the file stores it under: none for a 2D polyline's.
 */
constexpr unsigned VertexKinds = 32U | 64U | 128U;

/**
 *  The type bits a vertex of the polyline takes when it gives none: a polyface mesh's are a vertex's, not a face's.
 */
auto KindOf(const Entities::Polyline &polyline) -> unsigned
{
  if (Has(polyline.Flags, Entities::PolylineFlags::Polyline3d))
  {
    return std::to_underlying(Entities::VertexFlags::Polyline3d);
  }
  if (Has(polyline.Flags, Entities::PolylineFlags::PolygonMesh))
  {
    return std::to_underlying(Entities::VertexFlags::PolygonMesh);
  }
  if (Has(polyline.Flags, Entities::PolylineFlags::PolyfaceMesh))
  {
    return static_cast<unsigned>(std::to_underlying(Entities::VertexFlags::PolygonMesh) |
                                 std::to_underlying(Entities::VertexFlags::PolyfaceMesh));
  }
  return 0;
}

/**
 *  Whether a vertex's type bits fit the polyline: a polyface mesh takes its vertices and its faces.
 */
auto Fits(const Entities::Polyline &polyline, unsigned kind) -> bool
{
  const unsigned wanted = KindOf(polyline);
  return kind == wanted || (Has(polyline.Flags, Entities::PolylineFlags::PolyfaceMesh) &&
                            kind == std::to_underlying(Entities::VertexFlags::PolyfaceMesh));
}
} // namespace

auto AddAttribute(Ledger &ledger, Handle insert, Entities::Attrib attribute) -> std::expected<Handle, Failure>
{
  if (!attribute.Lettering.Style)
  {
    attribute.Lettering.Style = ledger.Target().Header.Textstyle;
  }
  return Follow<Entities::Insert>(ledger, insert, std::move(attribute));
}

auto AddVertex(Ledger &ledger, Handle polyline, Entities::Vertex vertex) -> std::expected<Handle, Failure>
{
  if (const auto *head = GetIf<Entities::Polyline>(ledger.Find(polyline)))
  {
    const unsigned kind = static_cast<unsigned>(vertex.Flags) & VertexKinds;
    if (kind == 0)
    {
      vertex.Flags = static_cast<int>(static_cast<unsigned>(vertex.Flags) | KindOf(*head));
    }
    else if (!Fits(*head, kind))
    {
      return Refuse(FailureCode::Refused,
                    std::format("vertex flags {} do not fit POLYLINE {}", vertex.Flags, HexOf(polyline)));
    }
  }
  return Follow<Entities::Polyline>(ledger, polyline, std::move(vertex));
}

auto AddBlock(Ledger &ledger, std::string_view name) -> std::expected<Handle, Failure>
{
  if (ledger.FindRecord(Tables::Table::Block, name) != nullptr)
  {
    return Refuse(FailureCode::Exists, std::format("block {} exists", name));
  }
  if (ledger.Control(Tables::Table::Block) == nullptr)
  {
    return Refuse(FailureCode::Missing, "block table missing");
  }
  const Drawing &drawing = ledger.Target();
  Tables::BlockRecord record;
  record.Record.Base.Self = ledger.MakeHandle();
  record.Record.Name = std::string{name};
  record.Record.Referenced = true;
  record.Block = ledger.MakeHandle();
  record.EndBlk = ledger.MakeHandle();
  const std::expected<Handle, Failure> self = ListRecord(ledger, Tables::Table::Block, Object{record});
  AppendObject(ledger, Making::MakeBlock(record, Space::Owner, drawing.Header.Clayer));
  AppendObject(ledger, Making::MakeEndBlk(record, Space::Owner, drawing.Header.Clayer));
  return self;
}
} // namespace DwgIO::Edit
