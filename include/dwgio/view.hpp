/***********************************************************************************************************************
 *  @file      dwgio/view.hpp
 *  @brief     A drawing read by its structure: tables, blocks, layouts and dictionaries.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_VIEW_HPP
#define DWGIO_VIEW_HPP

#include "dwgio/api.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/index.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tables/ucs.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tables/viewportheader.hpp"
#include "dwgio/tables/vport.hpp"

#include <string_view>
#include <utility>
#include <vector>

/**
 *  A drawing read by its structure: the tables through their controls, a block's entities in the order they are drawn,
 *  the layouts by their tabs, the dictionaries by name.
 *
 *  Every function gives pointers into `Drawing::Objects`, good until the vector changes, or null. Each takes the
 *  drawing or an `Index` of it: given the drawing, a function indexes it on each call, in a time that grows with its
 *  size; given an index, it takes a time that does not, which walking a large drawing needs.
 */
namespace DwgIO::View
{
/**
 *  Whether two names agree as AutoCAD compares them: ASCII letters in any case.
 */
[[nodiscard]] DWGIO_API auto Matches(std::string_view one, std::string_view other) -> bool;

[[nodiscard]] DWGIO_API auto Control(const Index &index, Tables::Table table) -> const Tables::Control *;

[[nodiscard]] DWGIO_API auto Control(const Drawing &drawing, Tables::Table table) -> const Tables::Control *;

[[nodiscard]] DWGIO_API auto Block(const Index &index, std::string_view name) -> const Tables::BlockRecord *;

[[nodiscard]] DWGIO_API auto Block(const Drawing &drawing, std::string_view name) -> const Tables::BlockRecord *;

/**
 *  The block record of model space, as the header names it.
 */
[[nodiscard]] DWGIO_API auto ModelSpace(const Index &index) -> const Tables::BlockRecord *;

[[nodiscard]] DWGIO_API auto ModelSpace(const Drawing &drawing) -> const Tables::BlockRecord *;

/**
 *  The block record of the active paper space, as the header names it.
 */
[[nodiscard]] DWGIO_API auto PaperSpace(const Index &index) -> const Tables::BlockRecord *;

[[nodiscard]] DWGIO_API auto PaperSpace(const Drawing &drawing) -> const Tables::BlockRecord *;

/**
 *  A block's entities in the order they are drawn: the record's list when it keeps one, as R2004 and later do, else the
 *  chain from the record's First through each Next, or, where the chain is cut, loops or is not kept, every entity the
 *  record owns in file order; never its BLOCK and ENDBLK.
 */
[[nodiscard]] DWGIO_API auto Entities(const Index &index, const Tables::BlockRecord &record)
    -> std::vector<const Object *>;

[[nodiscard]] DWGIO_API auto Entities(const Drawing &drawing, const Tables::BlockRecord &record)
    -> std::vector<const Object *>;

/**
 *  An insert's attributes in their order, whichever release's form it keeps them in: its list from R2004, the chain
 *  from First to Last before; an entry that is no ATTRIB, and the chain past one, are left out.
 */
[[nodiscard]] DWGIO_API auto Attributes(const Index &index, const Entities::Insert &insert)
    -> std::vector<const Entities::Attrib *>;

[[nodiscard]] DWGIO_API auto Attributes(const Drawing &drawing, const Entities::Insert &insert)
    -> std::vector<const Entities::Attrib *>;

/**
 *  A POLYLINE's vertices in their order, as Attributes gives an insert's.
 */
[[nodiscard]] DWGIO_API auto Vertices(const Index &index, const Entities::Polyline &polyline)
    -> std::vector<const Entities::Vertex *>;

[[nodiscard]] DWGIO_API auto Vertices(const Drawing &drawing, const Entities::Polyline &polyline)
    -> std::vector<const Entities::Vertex *>;

/**
 *  In tab order, Model first.
 */
[[nodiscard]] DWGIO_API auto Layouts(const Index &index) -> std::vector<const Objects::Layout *>;

[[nodiscard]] DWGIO_API auto Layouts(const Drawing &drawing) -> std::vector<const Objects::Layout *>;

[[nodiscard]] DWGIO_API auto Layout(const Index &index, std::string_view name) -> const Objects::Layout *;

[[nodiscard]] DWGIO_API auto Layout(const Drawing &drawing, std::string_view name) -> const Objects::Layout *;

/**
 *  The named objects dictionary, the root the header names.
 */
[[nodiscard]] DWGIO_API auto Root(const Index &index) -> const Objects::Dictionary *;

[[nodiscard]] DWGIO_API auto Root(const Drawing &drawing) -> const Objects::Dictionary *;

[[nodiscard]] DWGIO_API auto Entry(const Index &index, const Objects::Dictionary &dictionary, std::string_view name)
    -> const Object *;

[[nodiscard]] DWGIO_API auto Entry(const Drawing &drawing, const Objects::Dictionary &dictionary, std::string_view name)
    -> const Object *;

/**
 *  The dictionary the root's entry of that name leads to, such as ACAD_LAYOUT.
 */
[[nodiscard]] DWGIO_API auto Dictionary(const Index &index, std::string_view name) -> const Objects::Dictionary *;

[[nodiscard]] DWGIO_API auto Dictionary(const Drawing &drawing, std::string_view name) -> const Objects::Dictionary *;

template <class RecordT>
  requires requires(const RecordT &record) { record.Record; }
inline constexpr Tables::Table TableOf = Tables::Table::Block;

template <>
inline constexpr Tables::Table TableOf<Tables::Layer> = Tables::Table::Layer;
template <>
inline constexpr Tables::Table TableOf<Tables::Style> = Tables::Table::Style;
template <>
inline constexpr Tables::Table TableOf<Tables::LType> = Tables::Table::LType;
template <>
inline constexpr Tables::Table TableOf<Tables::View> = Tables::Table::View;
template <>
inline constexpr Tables::Table TableOf<Tables::Ucs> = Tables::Table::Ucs;
template <>
inline constexpr Tables::Table TableOf<Tables::VPort> = Tables::Table::VPort;
template <>
inline constexpr Tables::Table TableOf<Tables::AppId> = Tables::Table::AppId;
template <>
inline constexpr Tables::Table TableOf<Tables::DimStyle> = Tables::Table::DimStyle;
template <>
inline constexpr Tables::Table TableOf<Tables::ViewportEntityHeader> = Tables::Table::ViewportEntityHeader;

/**
 *  The records of a kind's table, in the order its control lists them, then the records the control keeps apart from
 *  its entries: the model and paper space block records, the ByBlock and ByLayer linetypes; an entry that is no such
 *  record is left out.
 */
template <class RecordT>
[[nodiscard]] auto Records(const Index &index) -> std::vector<const RecordT *>
{
  std::vector<const RecordT *> records;
  const Tables::Control *control = Control(index, TableOf<RecordT>);
  if (control == nullptr)
  {
    return records;
  }
  const auto take = [&](Handle entry) -> void
  {
    if (const auto *record = GetIf<RecordT>(Find(index, entry)))
    {
      records.push_back(record);
    }
  };
  for (const Handle entry : control->Entries)
  {
    take(entry);
  }
  take(control->ModelSpace);
  take(control->PaperSpace);
  take(control->ByBlock);
  take(control->ByLayer);
  return records;
}

template <class RecordT>
[[nodiscard]] auto Records(const Drawing &drawing) -> std::vector<const RecordT *>
{
  return Records<RecordT>(Index{drawing});
}

/**
 *  As the const Records, to change them.
 */
template <class RecordT>
[[nodiscard]] auto Records(Drawing &drawing) -> std::vector<RecordT *>
{
  std::vector<RecordT *> records;
  for (const RecordT *record : Records<RecordT>(std::as_const(drawing)))
  {
    records.push_back(const_cast<RecordT *>(record)); // NOLINT(cppcoreguidelines-pro-type-const-cast)
  }
  return records;
}

/**
 *  Matched in any case: first among the records the control keeps apart, so *Paper_Space is the active paper space
 *  though AutoCAD gives every other layout's block that name too, then among the entries in their order.
 */
template <class RecordT>
[[nodiscard]] auto Record(const Index &index, std::string_view name) -> const RecordT *
{
  const Tables::Control *control = Control(index, TableOf<RecordT>);
  if (control == nullptr)
  {
    return nullptr;
  }
  std::vector<Handle> order{control->ModelSpace, control->PaperSpace, control->ByBlock, control->ByLayer};
  order.insert(order.end(), control->Entries.begin(), control->Entries.end());
  for (const Handle self : order)
  {
    const auto *record = GetIf<RecordT>(Find(index, self));
    if (record != nullptr && Matches(record->Record.Name, name))
    {
      return record;
    }
  }
  return nullptr;
}

template <class RecordT>
[[nodiscard]] auto Record(const Drawing &drawing, std::string_view name) -> const RecordT *
{
  return Record<RecordT>(Index{drawing}, name);
}

/**
 *  As the const Record, to change it.
 */
template <class RecordT>
[[nodiscard]] auto Record(Drawing &drawing, std::string_view name) -> RecordT *
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<RecordT *>(Record<RecordT>(std::as_const(drawing), name));
}
} // namespace DwgIO::View

#endif /* DWGIO_VIEW_HPP */
