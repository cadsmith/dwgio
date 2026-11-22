/***********************************************************************************************************************
 *  @file      source/releases/history.hpp
 *  @brief     A 3DSOLID's history, kept in a record below R2007.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_HISTORY_HPP
#define DWGIO_RELEASES_HISTORY_HPP

#include "dwgio/entities/modeler.hpp"
#include "dwgio/tag.hpp"

#include <vector>

namespace DwgIO::Releases
{
/**
 *  A 3DSOLID's history as AutoCAD's save before R2007 keeps it in the solid's ACAD_XREC_ROUNDTRIP record: a marker
 *  "ACAD_SOLID_HIST" and the ACSH_HISTORY_CLASS object (360), a marker "ACAD_SOLID_HIST1" and the solid's R2007 long
 *  (90), a marker "ACAD_SOLID_HIST2" and the solid's own handle as a whole number (90).
 */
[[nodiscard]] auto MakeHistoryRecord(const Entities::Solid3d &solid) -> std::vector<Tag>;

/**
 *  The solid given back its history from a record, as AutoCAD reads it when upgrading to R2007; the groups of the
 *  history taken from the record, the rest left.
 */
void RestoreHistory(Entities::Solid3d &solid, std::vector<Tag> &record);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_HISTORY_HPP */
