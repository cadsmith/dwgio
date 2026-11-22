/***********************************************************************************************************************
 *  @file      dwgio/tables/record.hpp
 *  @brief     What every table record opens with.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_RECORD_HPP
#define DWGIO_TABLES_RECORD_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"

#include <string>

namespace DwgIO::Tables
{
/**
 *  What every table record opens with.
 */
struct DWGIO_API Record
{
  DwgIO::Base Base{};
  std::string Name{};
  bool Referenced{}; /**< Used by an entity at the last edit: the 64 bit of group 70.         */
  int XrefIndex{-1}; /**< Index of the xref's block record; -1 for the drawing's own records. */
  bool Dependent{};  /**< The 16 bit of group 70.                                             */
  Handle Source{};   /**< The xref block the record came from; null for none.                 */

  auto operator==(const Record &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_RECORD_HPP */
