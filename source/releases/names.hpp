/***********************************************************************************************************************
 *  @file      source/releases/names.hpp
 *  @brief     The names R14 cannot hold, changed by its save and kept in a record.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_NAMES_HPP
#define DWGIO_RELEASES_NAMES_HPP

#include "releases/remake.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/version.hpp"

#include <optional>
#include <string>
#include <string_view>

/**
 *  R14's save names a dictionary's entries, and a table's records, in upper case, every character but a letter, a
 *  digit, $, -, _ and * an underscore, a name of more than 31 characters cut to 26; and keeps each name it changed, in
 *  the entries' order, in an XRECORD under ACAD_XREC_ROUNDTRIP in the extension dictionary of the dictionary or record:
 *  a 102 of EXTNAMES, then for each name a 1 of the name R14 holds and a 2 of the name it was.
 */
namespace DwgIO::Releases
{
/**
 *  The name R14 gives one it cannot hold, as AutoCAD's save names it.
 */
[[nodiscard]] auto Narrow(std::string_view name) -> std::string;

/**
 *  The drawing a save of an R14 drawing to a later release writes: each dictionary's entries and each table's records
 *  named again as their EXTNAMES record says, as AutoCAD reads it, and the record's names taken from it, the record
 *  taken away when it holds nothing else and the extension dictionary with it. Empty when nothing changes.
 */
void WidenNames(Remake &remade, Version to);

/**
 *  The drawing a save of a later drawing to R14 writes: each dictionary's entries named as R14 holds them and the names
 *  changed kept in its EXTNAMES record, made under the handle seed with an extension dictionary where it has none, as
 *  AutoCAD's save does.
 *
 *  AutoCAD keeps as they are the plot style names of the dictionary with a default and the entries of the dictionary a
 *  dynamic block keeps under ACDBBLOCKTABLERECORDDATA; and the table records keep their names, which AutoCAD's R14
 *  opens as they are. Empty when nothing changes.
 */
void NarrowNames(Remake &remade, Version to);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_NAMES_HPP */
