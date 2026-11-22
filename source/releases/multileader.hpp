/***********************************************************************************************************************
 *  @file      source/releases/multileader.hpp
 *  @brief     What a MULTILEADER holds of R2010, kept in a record below it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_MULTILEADER_HPP
#define DWGIO_RELEASES_MULTILEADER_HPP

#include "dwgio/entities/multileader.hpp"
#include "dwgio/tag.hpp"

#include <vector>

namespace DwgIO::Releases
{
/**
 *  What a MULTILEADER holds of R2010, as AutoCAD's save before R2010 keeps it in the leader's ACAD_XREC_ROUNDTRIP
 *  record, in this order.
 *
 *  When its text attaches vertically, a marker "ROUNDTRIP_MLCONTEXT" with the context's direction (272), its
 *  attachments beside the text (174 left, 175 right), above (273) and below (274) the text, and for each leader its
 *  index (91) and its dogleg's length (40), which the save makes 0. Then for each line that overrides anything a marker
 *  "Leaderline" and its index, group 91 the index again, the values it overrides (170 the type, 92 the color's long,
 *  340 the linetype, 171 the lineweight, 40 the arrow size, 341 the arrowhead) and 93 the override bits. Then a marker
 *  "ACAD_ROUNDTRIP_2010_MLEADER" with the leader's own direction (272), attachments beside the text (173 left, 174
 *  right) and dogleg length (41) when its text attaches vertically, and its attachments below (273) and above (274) the
 *  text; an attachment above or below only when not at the text's center. The leader is left as R2007 reads it, what
 *  the save derives again in place.
 */
[[nodiscard]] auto MakeLinesRecord(Entities::MultiLeader &leader) -> std::vector<Tag>;

/**
 *  The leader given back what a record keeps for it, as AutoCAD reads it when upgrading to R2010; the groups of the
 *  leader taken from the record, the rest left.
 */
void RestoreLines(Entities::MultiLeader &leader, std::vector<Tag> &record);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_MULTILEADER_HPP */
