/***********************************************************************************************************************
 *  @file      dwgio/create.hpp
 *  @brief     A new drawing as AutoCAD makes one from nothing.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_CREATE_HPP
#define DWGIO_CREATE_HPP

#include "dwgio/api.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/header.hpp"
#include "dwgio/version.hpp"

namespace DwgIO
{
/**
 *  The header of a new drawing: every variable at AutoCAD's value for a new metric drawing, the dates at the time of
 *  the call, fresh GUIDs, and every handle null for Create to set.
 */
[[nodiscard]] DWGIO_API auto DefaultsOf(Version release) -> Header;

/**
 *  The drawing AutoCAD makes from nothing, as far as the library types it: the ten control objects, the layer 0, the
 *  linetypes ByBlock, ByLayer and Continuous, the text style Standard, the application ACAD, the dimension styles
 *  Standard and ISO-25, the viewport *Active, the block records *Model_Space and *Paper_Space with their BLOCK and
 *  ENDBLK, the layouts Model and Layout1, the named objects dictionary with ACAD_GROUP, ACAD_LAYOUT, ACAD_MLINESTYLE
 *  and its Standard style, ACAD_PLOTSETTINGS and ACAD_PLOTSTYLENAME with its Normal placeholder, the classes those are
 *  stored under, and every root of the header set.
 *
 *  Saved, it loads back whole and AutoCAD's AUDIT finds nothing in it. Another release gives the same drawing as a save
 *  to that release would hold it.
 */
[[nodiscard]] DWGIO_API auto Create(Version release = Version::R2000) -> Drawing;
} // namespace DwgIO

#endif /* DWGIO_CREATE_HPP */
