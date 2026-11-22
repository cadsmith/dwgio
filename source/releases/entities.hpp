/***********************************************************************************************************************
 *  @file      source/releases/entities.hpp
 *  @brief     The entities whose fields another release holds differently.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_ENTITIES_HPP
#define DWGIO_RELEASES_ENTITIES_HPP

#include "releases/passage.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/leader.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/entities/spline.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

namespace DwgIO::Releases
{
/**
 *  A spline upgraded to R2013, which says its form in its flags; one with no fit points takes the custom knots, as
 *  AutoCAD's files have them: 9 and the chord for one by fit points, 0 and 15 for one by control points.
 */
void ConvertSpline(Entities::Spline &spline, const Crossings &crossing);

/**
 *  A multileader across R2010: its version, and its arrowheads, which only the releases before keep, dropped and noted
 *  when upgrading.
 */
void ConvertAnnotation(Entities::MultiLeader &leader, const Passage &passage, const Crossings &crossing);

/**
 *  A leader across R2000, whose R14 fields AutoCAD derives at each save to R14, and below R2010, which keeps the box of
 *  its annotation that AutoCAD derives at each save before R2010.
 */
void ConvertAnnotation(Entities::Leader &leader, const Passage &passage, const Crossings &crossing);

/**
 *  An MTEXT as the release holds it, what an older one keeps in extended data moved there or back; noted where
 *  AutoCAD's save would split the text into an MTEXT per column, which takes laying the text out.
 */
void ConvertAnnotation(Entities::MText &text, const Passage &passage, [[maybe_unused]] const Crossings &crossing);

/**
 *  An untyped object as the release holds it: its own bits recast where its class crosses, its bytes, which are the
 *  other release's, dropped so that the save writes its header again; as it is where it does not cross, since the save
 *  leaves it out.
 */
void ConvertUntyped(Unknown &unknown, const Drawing &drawing, Version to);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_ENTITIES_HPP */
