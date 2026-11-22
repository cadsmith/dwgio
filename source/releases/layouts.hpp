/***********************************************************************************************************************
 *  @file      source/releases/layouts.hpp
 *  @brief     The layouts and page setups as another release holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_LAYOUTS_HPP
#define DWGIO_RELEASES_LAYOUTS_HPP

#include "releases/passage.hpp"

#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/plotsettings.hpp"

namespace DwgIO::Releases
{
/**
 *  A layout as R2000 holds it: no shade plot, plot view or viewports, each dropped and noted when set.
 */
void LowerLayout(Objects::Layout &layout, const Passage &passage);

/**
 *  A page setup across R2004 and R2007: its plot view between name and record, no shade plot below R2004 and no visual
 *  style below R2007, each dropped and noted when set.
 */
void ConvertPlotSettings(Objects::PlotSettings &settings, const Passage &passage, const Crossings &crossing);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_LAYOUTS_HPP */
