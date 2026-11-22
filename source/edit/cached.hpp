/***********************************************************************************************************************
 *  @file      source/edit/cached.hpp
 *  @brief     The ledger that keeps its answers for a run of edits.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_CACHED_HPP
#define DWGIO_EDIT_CACHED_HPP

#include "edit/ledger.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace DwgIO::Edit
{
/**
 *  A ledger that walks the drawing once and keeps its answers current through each object put: where each handle is,
 *  the seed past them all, where each table's control is and the names its records bear. It answers for a drawing that
 *  changes only through it.
 */
class CachedLedger final : public Ledger
{
public:
  explicit CachedLedger(Drawing &drawing);

  [[nodiscard]] auto Find(Handle self) -> Object * override;
  [[nodiscard]] auto MakeHandle() -> Handle override;
  [[nodiscard]] auto FindRecord(Tables::Table table, std::string_view name) -> Object * override;
  [[nodiscard]] auto Control(Tables::Table table) -> Tables::Control * override;
  void Put(std::size_t at, Object object) override;

private:
  /**
   *  Takes in the handle of the object at a place.
   */
  void Register(std::size_t at);

  /**
   *  The place of the first object with each handle but the null one.
   */
  std::unordered_map<std::uint64_t, std::size_t> m_Places;
  Handle m_Top{};                                                     /**< Past every handle the drawing holds.     */
  std::array<std::optional<std::size_t>, Tables::Count> m_Controls{}; /**< The place of each table's first control. */
  /**
   *  Each table's records by their names in capitals, the first in View::Record's order.
   */
  std::array<std::unordered_map<std::string, Handle>, Tables::Count> m_Names{};
};
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_CACHED_HPP */
