/***********************************************************************************************************************
 *  @file      dwgio/tables/viewportheader.hpp
 *  @brief     The viewport entity header a paper space VIEWPORT is listed under.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_VIEWPORTHEADER_HPP
#define DWGIO_TABLES_VIEWPORTHEADER_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/record.hpp"

namespace DwgIO::Tables
{
/**
 *  A viewport entity header: the record a paper space VIEWPORT is listed under, chained to the one before it. DXF does
 *  not export it.
 */
struct DWGIO_API ViewportEntityHeader
{
  Tables::Record Record{}; /**< The name is empty as AutoCAD writes it. */
  bool Flag{};             /**< The 1 bit of group 70.                  */
  Handle Viewport{};
  Handle Previous{}; /**< Null at the chain's end. */

  auto operator==(const ViewportEntityHeader &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_VIEWPORTHEADER_HPP */
