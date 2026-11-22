/***********************************************************************************************************************
 *  @file      source/audit/common.hpp
 *  @brief     What the audit's checks share: findings, lookups, chains and erasure.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_AUDIT_COMMON_HPP
#define DWGIO_AUDIT_COMMON_HPP

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/tables/table.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace DwgIO::Audit
{
/**
 *  AutoCAD recovers or refuses the drawing for it.
 */
void NoteFailure(Report &report, std::string what, Handle subject);
/**
 *  AutoCAD opens the drawing as it is and makes what it needs itself.
 */
void NoteWarning(Report &report, std::string what, Handle subject);

/**
 *  A handle for a new object: the one the header kept for it when that names nothing, else a new one.
 */
[[nodiscard]] auto ReserveHandle(Drawing &drawing, Handle kept) -> Handle;
/**
 *  Null when the handle names no dictionary.
 */
[[nodiscard]] auto DictionaryAt(Drawing &drawing, Handle self) -> Objects::Dictionary *;
/**
 *  Enters an object under a name, taking the place of the entry of that name when there is one.
 */
void SetEntry(Objects::Dictionary &dictionary, std::string_view name, Handle object);
/**
 *  The record of a table under a name, listed or not, or null.
 */
[[nodiscard]] auto Present(const Drawing &drawing, Tables::Table table, std::string_view name) -> Handle;
/**
 *  Makes the layout of a space, entered in ACAD_LAYOUT, and gives its handle.
 */
[[nodiscard]] auto MakeLayout(Drawing &drawing, Handle record, bool model) -> Handle;
/**
 *  Erases the objects of the handles, closing the chain of each entity over it, or from R2004 its owner's list, and
 *  every dictionary and control entry naming one.
 */
void EraseObjects(Drawing &drawing, const std::unordered_set<std::uint64_t> &removed);

/**
 *  The owner the entities of a chain hang from, and the chain's ends: a block record's, a polyline's or an insert's.
 */
struct Chain
{
  Handle Owner{};
  Handle *First{};
  Handle *Last{};
  Handle *SeqEnd{}; /**< The SEQEND a sequence's head names; null for a block's chain, which has none. */
  /**
   *  The list the owner keeps in place of the chain from R2004; null before, when the chain is what is checked.
   */
  std::vector<Handle> *List{};
};

/**
 *  The objects of a drawing by handle, built once per pass; good until the drawing's objects are pushed to or erased
 *  from.
 */
class Index
{
public:
  explicit Index(Drawing &drawing);
  /**
   *  Null when no object has the handle.
   */
  [[nodiscard]] auto At(Handle handle) const -> Object *;

private:
  std::unordered_map<std::uint64_t, Object *> m_At;
};

/**
 *  The entities that belong in every chain, by owner, in file order: the members of a sequence, the vertices and
 *  attributes, or the entities of a block, which are the rest but its BLOCK, ENDBLK and sequence members.
 */
[[nodiscard]] auto CollectChains(Drawing &drawing, bool members)
    -> std::unordered_map<std::uint64_t, std::vector<Object *>>;
/**
 *  The entities of one block record's chain, in file order.
 */
[[nodiscard]] auto CollectOwned(Drawing &drawing, Handle owner) -> std::vector<Object *>;
/**
 *  The SEQEND each sequence's head owns, by head; the first when there are several.
 */
[[nodiscard]] auto Seqends(const Drawing &drawing) -> std::unordered_map<std::uint64_t, Handle>;
/**
 *  The handles the header and the controls keep to records: the current layer, styles, linetypes and spaces, and the
 *  block and linetype tables' spaces and ByBlock and ByLayer.
 */
[[nodiscard]] auto CollectRoots(Drawing &drawing) -> std::vector<Handle *>;
/**
 *  Whether a chain's links walk exactly its entities, each one once, from First to Last; from R2004, whether the
 *  owner's list names exactly them, each once.
 */
[[nodiscard]] auto Walks(const Index &index, const Chain &chain, const std::vector<Object *> &owned) -> bool;
/**
 *  Links a chain's entities in file order; from R2004, lists them in file order and clears the links.
 */
void Relink(const Chain &chain, const std::vector<Object *> &owned);

/**
 *  A table and the header's member naming its control.
 */
struct Root
{
  Tables::Table Table;
  Handle Header::*Member;
};

inline constexpr std::array<Root, Tables::Count> Controls{
    {{.Table = Tables::Table::Block, .Member = &Header::BlockControlObject},
     {.Table = Tables::Table::Layer, .Member = &Header::LayerControlObject},
     {.Table = Tables::Table::Style, .Member = &Header::StyleControlObject},
     {.Table = Tables::Table::LType, .Member = &Header::LTypeControlObject},
     {.Table = Tables::Table::View, .Member = &Header::ViewControlObject},
     {.Table = Tables::Table::Ucs, .Member = &Header::UcsControlObject},
     {.Table = Tables::Table::VPort, .Member = &Header::VPortControlObject},
     {.Table = Tables::Table::AppId, .Member = &Header::AppIdControlObject},
     {.Table = Tables::Table::DimStyle, .Member = &Header::DimStyleControlObject},
     {.Table = Tables::Table::ViewportEntityHeader, .Member = &Header::ViewportEntityHeaderControlObject}}};
} // namespace DwgIO::Audit

#endif /* DWGIO_AUDIT_COMMON_HPP */
