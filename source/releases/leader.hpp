/***********************************************************************************************************************
 *  @file      source/releases/leader.hpp
 *  @brief     A LEADER's R14 fields, kept by R2000 and derived again at a save to R14.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_LEADER_HPP
#define DWGIO_RELEASES_LEADER_HPP

#include "releases/lookup.hpp"

#include "dwgio/entities/leader.hpp"
#include "dwgio/handle.hpp"

namespace DwgIO::Releases
{
/**
 *  A LEADER's fields of R14 as R2000 keeps them, as AutoCAD's save of an R14 drawing writes it: the arrowhead's kind in
 *  the R2000 short, the rest dropped, since AutoCAD derives them again from the dimension style at each save to R14.
 */
void RaiseLeader(Entities::Leader &leader);

/**
 *  A LEADER's fields of R14 as AutoCAD's save to R14 derives them from its dimension style, its DSTYLE overrides and
 *  its color: the gap and arrow size times DIMSCALE (0 taken as 1), DIMCLRD's index (ByBlock as the leader's own color,
 *  ByBlock again as 7), 8 for an arrowhead of a block.
 *
 *  R14 draws the leader's arrow by DIMBLK, so the save writes the arrow block's name, upper case and without AutoCAD's
 *  underscore, as the first DSTYLE override in the leader's ACAD extended data, which goes last. False, the extended
 *  data left as it was, when acad is null.
 */
[[nodiscard]] auto LowerLeader(Entities::Leader &leader, const Lookup &found, Handle acad) -> bool;

/**
 *  A LEADER's annotation box as R2007 and before keep it, as AutoCAD's save derives it: the extents of an MTEXT
 *  annotation, else zero.
 */
void BoxLeader(Entities::Leader &leader, const Lookup &found);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_LEADER_HPP */
