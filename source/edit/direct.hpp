/***********************************************************************************************************************
 *  @file      source/edit/direct.hpp
 *  @brief     The ledger that walks the drawing for each answer.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_DIRECT_HPP
#define DWGIO_EDIT_DIRECT_HPP

#include "edit/ledger.hpp"

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
 *  A ledger that walks the drawing's objects for each answer, as a single edit needs: nothing to build first.
 */
class DirectLedger final : public Ledger
{
public:
  using Ledger::Ledger;

  [[nodiscard]] auto Find(Handle self) -> Object * override;
  [[nodiscard]] auto MakeHandle() -> Handle override;
  [[nodiscard]] auto FindRecord(Tables::Table table, std::string_view name) -> Object * override;
  [[nodiscard]] auto Control(Tables::Table table) -> Tables::Control * override;
  void Put(std::size_t at, Object object) override;
};
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_DIRECT_HPP */
