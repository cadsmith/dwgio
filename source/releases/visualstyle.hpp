/***********************************************************************************************************************
 *  @file      source/releases/visualstyle.hpp
 *  @brief     A visual style's properties as each release lays them out, and their record.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_VISUALSTYLE_HPP
#define DWGIO_RELEASES_VISUALSTYLE_HPP

#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace DwgIO::Releases
{
/**
 *  A visual style's properties as one release keeps them: those of its layout, and below R2013 the groups of the record
 *  AutoCAD keeps the others in, an XRECORD under ACAD_XREC_ROUNDTRIP in the style's extension dictionary.
 */
struct StyleLayout
{
  /**
   *  The properties, in the order of the release's layout.
   */
  std::vector<Objects::VisualStyles::Property> Properties{};
  std::vector<Tag> Kept{}; /**< The record's groups; none from R2013, whose layout holds every property. */
};

/**
 *  A style's properties as R2013 lays them out, its 58 with their flags, from a style of a release and the groups of
 *  its record, as AutoCAD reads them: R2010's 28 are R2013's first, a property of the older layout is R2010's of the
 *  same DXF code, its opacity and specular amount negative where the face modifiers leave theirs off and its line width
 *  a whole number; the flags of the 28 below R2010 and the 30 properties beyond them below R2013 come from the record,
 *  and where it lacks them, as in a file of an older AutoCAD, are AutoCAD's defaults.
 */
[[nodiscard]] auto RaiseProperties(const Objects::VisualStyle &style, Version from, std::span<const Tag> kept)
    -> std::vector<Objects::VisualStyles::Property>;

/**
 *  The 58 properties as a release keeps them, as AutoCAD's save writes them: R2013's layout whole; R2010's 28 and the
 *  record of the rest; or below R2010 the older layout, R2007's last real and the short R2010 dropped as 0, a lighting
 *  quality of 3 as 2, colors below R2004 as R2000 holds them, and the record of the rest and of every property's flags,
 *  with the five flags beyond R2013's the style's record held, else 1.
 */
[[nodiscard]] auto FitProperties(std::span<const Objects::VisualStyles::Property> full, std::span<const Tag> kept,
                                 Version to) -> StyleLayout;

/**
 *  The value of the group after the record's marker of that name, or null.
 */
[[nodiscard]] auto FindMarked(std::span<const Tag> kept, std::string_view name) -> const Content *;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_VISUALSTYLE_HPP */
