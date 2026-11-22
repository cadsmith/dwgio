/***********************************************************************************************************************
 *  @file      dwgio/tables/table.hpp
 *  @brief     The tables a drawing keeps under control objects, and the names of their controls.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_TABLE_HPP
#define DWGIO_TABLES_TABLE_HPP

#include "dwgio/api.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace DwgIO::Tables
{
/**
 *  The tables a file keeps, each under a control object.
 */
enum class Table : std::uint8_t
{
  Block,
  Layer,
  Style, /**< Text styles and shape files. */
  LType,
  View,
  Ucs,
  VPort, /**< The tiled viewports. */
  AppId,
  DimStyle,
  ViewportEntityHeader, /**< Up to R2000. */
};

/**
 *  How many tables Table names.
 */
inline constexpr std::size_t Count = 10;

/**
 *  As the file names the control object, such as "LAYER_CONTROL".
 */
[[nodiscard]] DWGIO_API auto NameOf(Table table) -> std::string_view;
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_TABLE_HPP */
