/***********************************************************************************************************************
 *  @file      source/releases/lookup.hpp
 *  @brief     A drawing's objects by handle, for a pass over all of them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_LOOKUP_HPP
#define DWGIO_RELEASES_LOOKUP_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/index.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/record.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/view.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace DwgIO::Releases
{
/**
 *  A drawing's objects by handle, built once for a pass over all of them, where Find for each would take time growing
 *  with the square of their number.
 *
 *  Good while the drawing's objects are neither added to, erased nor moved.
 */
class Lookup
{
public:
  explicit Lookup(const DwgIO::Drawing &drawing);

  [[nodiscard]] auto Drawing() const -> const DwgIO::Drawing & { return m_Index.Source(); }

  /**
   *  As Find has it: the first that bears the handle; null for no handle or one none bears.
   */
  [[nodiscard]] auto Of(Handle self) const -> const Object *;
  /**
   *  Whether the object is the one its handle finds: not one of no handle, nor one whose handle another bears first,
   *  which a broken file holds.
   */
  [[nodiscard]] auto First(const Object &object) const -> bool;
  /**
   *  How many entities the drawing holds with an owner as their owner, whatever their chain says: not the BLOCK, ENDBLK
   *  and SEQEND that bracket or close them, which no list keeps, nor the objects it owns that are no entities.
   */
  [[nodiscard]] auto CountOwned(Handle owner) const -> std::size_t;
  /**
   *  Where an entity first stands in the list its owner keeps, or nothing when the list names it nowhere. The places
   *  are counted once for each owner.
   */
  [[nodiscard]] auto Place(Handle owner, const std::vector<Handle> &list, Handle self) const
      -> std::optional<std::size_t>;

  /**
   *  The record of a kind's table with that name, in any case, or null, as View::Record finds it.
   */
  template <class RecordT>
  [[nodiscard]] auto Record(std::string_view name) const -> const RecordT *
  {
    const Object *object = FindNamed(View::TableOf<RecordT>, name,
                                     [](const Object &one) -> const Tables::Record *
                                     {
                                       const auto *record = GetIf<RecordT>(&one);
                                       return record == nullptr ? nullptr : &record->Record;
                                     });
    return object == nullptr ? nullptr : &Get<RecordT>(*object);
  }

private:
  DwgIO::Index m_Index;
  std::unordered_map<std::uint64_t, std::size_t> m_Owned;
  mutable std::unordered_map<std::uint64_t, std::unordered_map<std::uint64_t, std::size_t>> m_Places;

  /**
   *  The object a table's control names, among its entries and then the records it keeps apart from them (the model and
   *  paper space block records, the ByBlock and ByLayer linetypes), whose record goes by that name in any case; null
   *  when none does, or the drawing has no such table.
   */
  [[nodiscard]] auto FindNamed(Tables::Table table, std::string_view name,
                               auto (*recordOf)(const Object &)->const Tables::Record *) const -> const Object *;
};
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_LOOKUP_HPP */
