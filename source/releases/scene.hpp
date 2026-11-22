/***********************************************************************************************************************
 *  @file      source/releases/scene.hpp
 *  @brief     The helix, light and visual style as the release a save writes holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_SCENE_HPP
#define DWGIO_RELEASES_SCENE_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <vector>

namespace DwgIO::Releases
{
struct Way
{
  const DwgIO::Drawing *Drawing{}; /**< Its ACAD application names the marker. */
  Version From{};
  Version To{};
};

/**
 *  A kind of the rendered scene as the release written holds it: a helix's curve takes the spline flags when upgrading
 *  to R2013; a light's color becomes its index below R2004, a true color dropped and noted, and its color of no method
 *  is R2000's index 257.
 *
 *  A visual style's properties are the release's already, made so with its record before the save
 *  (releases/roundtrip.hpp). A light and a visual style below R2018 carry ACAD's extended data AcDbSavedByObjectVersion
 *  0, as AutoCAD's files do: AutoCAD refuses an R2000 file whose light lacks it, and an R2004 one whose visual style
 *  does, so a save below R2018 adds it where an older AutoCAD's file had none; R2018 drops it. Any other object is left
 *  as it is.
 */
void ConvertScene(Object &object, Way way, std::vector<Issue> &issues);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_SCENE_HPP */
