/***********************************************************************************************************************
 *  @file      source/releases/roundtrip.hpp
 *  @brief     What styles keep in a record below the release that gave them a field.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_ROUNDTRIP_HPP
#define DWGIO_RELEASES_ROUNDTRIP_HPP

#include "releases/remake.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/version.hpp"

#include <optional>

namespace DwgIO::Releases
{
/**
 *  The drawing a save to a release writes when its styles hold what the release keeps elsewhere: below R2013 a visual
 *  style keeps the properties of R2013's layout its own lacks, below R2010 their flags too, below R2018 a view style
 *  keeps its display name and flags, and below R2010 a multileader what its leader lines hold of their own
 *  (releases/lined.hpp), in an XRECORD under ACAD_XREC_ROUNDTRIP in its extension dictionary, which AutoCAD's save
 *  makes when downgrading and reads when upgrading.
 *
 *  The drawing returned is the one given with each visual style in the release's layout and each view style crossing
 *  R2018 in its bits, the records made, filled or taken away as the release has them, a record made under a dictionary
 *  made where the style has none, both under the handle seed, and a dictionary left empty taken away with it. Upgrading
 *  to R2013, a NURBS surface's record, which AutoCAD's save below R2013 makes, is taken away, as AutoCAD reads it: it
 *  refuses an R2013 file whose NURBS surface keeps it. Empty when nothing changes.
 */
void KeepRoundTrips(Remake &remade, Version to);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_ROUNDTRIP_HPP */
