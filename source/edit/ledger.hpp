/***********************************************************************************************************************
 *  @file      source/edit/ledger.hpp
 *  @brief     What an addition asks of the drawing it changes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_LEDGER_HPP
#define DWGIO_EDIT_LEDGER_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"

#include <cstddef>
#include <string_view>

namespace DwgIO::Edit
{
/**
 *  What an addition asks of the drawing it changes besides its fields: an object by handle, a new handle, a record by
 *  name, a table's control, and a place for each object it brings.
 *
 *  DirectLedger asks the drawing each time, CachedLedger keeps the answers for a run of edits.
 */
class Ledger
{
public:
  explicit Ledger(Drawing &drawing) : m_Drawing{&drawing} {}

  Ledger(const Ledger &) = delete;
  Ledger(Ledger &&) = delete;
  auto operator=(const Ledger &) -> Ledger & = delete;
  auto operator=(Ledger &&) -> Ledger & = delete;
  virtual ~Ledger() = default;

  [[nodiscard]] auto Target() const -> Drawing & { return *m_Drawing; }

  /**
   *  The first in file order with that handle; null for none, and for the null handle.
   */
  [[nodiscard]] virtual auto Find(Handle self) -> Object * = 0;
  /**
   *  The handle seed, above every handle the drawing holds; the seed moves past it.
   */
  [[nodiscard]] virtual auto MakeHandle() -> Handle = 0;
  /**
   *  The record of a table bearing a name in any case, found as View::Record finds it; null for none.
   */
  [[nodiscard]] virtual auto FindRecord(Tables::Table table, std::string_view name) -> Object * = 0;
  /**
   *  The first control of a table in file order; null for none.
   */
  [[nodiscard]] virtual auto Control(Tables::Table table) -> Tables::Control * = 0;
  /**
   *  Puts an object into the drawing's objects before the one at that place, or last at their size.
   */
  virtual void Put(std::size_t at, Object object) = 0;

private:
  Drawing *m_Drawing;
};
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_LEDGER_HPP */
