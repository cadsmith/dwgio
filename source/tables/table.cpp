/***********************************************************************************************************************
 *  @file      source/tables/table.cpp
 *  @brief     The name of each table.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/table.hpp"

#include "objects/types.hpp"
#include "tables/codes.hpp"

namespace DwgIO::Tables
{
auto NameOf(Table table) -> std::string_view
{
  return Objects::TypeNameOf(ControlCode(table));
}
} // namespace DwgIO::Tables
