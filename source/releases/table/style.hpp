/***********************************************************************************************************************
 *  @file      source/releases/table/style.hpp
 *  @brief     The tables and table styles translated across R2010.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_TABLE_STYLE_HPP
#define DWGIO_RELEASES_TABLE_STYLE_HPP

#include "dwgio/handle.hpp"
#include "dwgio/objects/tablestyles/style.hpp"

namespace DwgIO::Releases::Table
{
/**
 *  The bit of a table style's flags AutoCAD's save sets when it upgrades a style from below R2010 to R2018, and clears
 *  below R2018.
 */
inline constexpr unsigned UpgradeBit = 8;

/**
 *  A table style upgraded to R2010: the table's cell style and the title, header and data styles made from the three
 *  cells and the margins as AutoCAD's R2010 files have them, the cells cleared; the borders take the ByBlock linetype.
 *
 *  Cell styles the style already holds, those of its cell style map, stand instead.
 */
void Raise(Objects::TableStyle &style, Handle byblock);

/**
 *  A table style downgraded below R2010: the three cells from the title, header and data styles, the margins and the
 *  flow from the table's cell style, the cell styles cleared.
 */
void Lower(Objects::TableStyle &style);

} // namespace DwgIO::Releases::Table

#endif /* DWGIO_RELEASES_TABLE_STYLE_HPP */
