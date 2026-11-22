/***********************************************************************************************************************
 *  @file      dwgio/tables/control.hpp
 *  @brief     A table's control object, which owns its records.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_CONTROL_HPP
#define DWGIO_TABLES_CONTROL_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/table.hpp"

#include <vector>

namespace DwgIO::Tables
{
/**
 *  A table's control object: the records it owns.
 */
struct DWGIO_API Control
{
  DwgIO::Base Base{};
  Tables::Table Table{};
  std::vector<Handle> Entries{}; /**< In the file's order; a slot may be null. */
  /**
   *  The block control's *Model_Space record, listed after the entries; null in other controls.
   */
  Handle ModelSpace{};
  /**
   *  The block control's *Paper_Space record, listed after *Model_Space; null in other controls.
   */
  Handle PaperSpace{};
  /**
   *  The linetype control's ByBlock record, listed after the entries; null in other controls.
   */
  Handle ByBlock{};
  /**
   *  The linetype control's ByLayer record, listed after ByBlock, the reverse of the specification's order; null in
   *  other controls.
   */
  Handle ByLayer{};
  /**
   *  From R2000, the dimension style control's hard pointers after its entries: AutoCAD lists its styles again. Empty
   *  in other controls.
   */
  std::vector<Handle> RepeatedEntries{};

  auto operator==(const Control &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_CONTROL_HPP */
