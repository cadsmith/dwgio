/***********************************************************************************************************************
 *  @file      dwgio/tables/appid.hpp
 *  @brief     The APPID record: an application that attaches extended data.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_APPID_HPP
#define DWGIO_TABLES_APPID_HPP

#include "dwgio/api.hpp"
#include "dwgio/tables/record.hpp"

namespace DwgIO::Tables
{
/**
 *  An APPID record: registers an application that attaches extended data.
 */
struct DWGIO_API AppId
{
  Tables::Record Record{};
  int Unknown1{}; /**< Unnamed in the specification; 0 as AutoCAD writes it. Group 71. */

  auto operator==(const AppId &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_APPID_HPP */
