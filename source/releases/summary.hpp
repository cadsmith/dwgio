/***********************************************************************************************************************
 *  @file      source/releases/summary.hpp
 *  @brief     The drawing's summary as R14 and R2000 keep it, in the DWGPROPS record.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_SUMMARY_HPP
#define DWGIO_RELEASES_SUMMARY_HPP

#include "releases/remake.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/summary.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <optional>
#include <vector>

/**
 *  As AutoCAD writes it: a 1 of DWGPROPS COOKIE; the title, subject, author, comments, keywords, last saver and
 *  revision as 2, 3, 4, 6, 7, 8 and 9; the first ten custom properties, a name, an equals sign and a value, as 300 to
 *  309, an equals sign alone for none; the editing time and the dates as Julian days, 40, 41 and 42, the dates to the
 *  second; the hyperlink base as a 1; the count of properties as a 90, and the properties past the tenth, each a 300.
 */
namespace DwgIO::Releases
{
/**
 *  The summary a DWGPROPS record holds, read as AutoCAD reads it: a property of no name left out; empty when the record
 *  does not open with the cookie.
 */
[[nodiscard]] auto SummaryOf(const Objects::XRecord &record) -> std::optional<SummaryInfo>;

/**
 *  The groups of the DWGPROPS record of a summary, as AutoCAD writes them.
 */
[[nodiscard]] auto MakePropertyGroups(const SummaryInfo &summary) -> std::vector<Tag>;

/**
 *  The DWGPROPS record of the drawing's named objects dictionary, or null.
 */
[[nodiscard]] auto FindProperties(const Drawing &drawing) -> const Objects::XRecord *;

/**
 *  The drawing a save to a release writes when the summary moves between the record and the section: below R2004 the
 *  record filled from the summary, or made under the handle seed where the dictionary has none; upgrading from below
 *  R2004 to R2004 and later the record taken away, as AutoCAD reads it into the summary on opening.
 *
 *  Empty when nothing changes: the record already holds the summary, or there is none to hold.
 */
void MoveSummary(Remake &remade, Version to);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_SUMMARY_HPP */
