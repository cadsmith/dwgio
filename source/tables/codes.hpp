/***********************************************************************************************************************
 *  @file      source/tables/codes.hpp
 *  @brief     The type code of each table's control object.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_CODES_HPP
#define DWGIO_TABLES_CODES_HPP

#include "dwgio/tables/table.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace DwgIO::Tables
{
/**
 *  In the order of Table.
 */
inline constexpr std::array<std::uint16_t, Count> ControlCodes{48, 50, 52, 56, 60, 62, 64, 66, 68, 70};

/**
 *  A value outside the enumeration gives the last code.
 */
[[nodiscard]] inline auto ControlCode(Table table) -> std::uint16_t
{
  return ControlCodes.at(std::min(static_cast<std::size_t>(table), ControlCodes.size() - 1));
}
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_CODES_HPP */
