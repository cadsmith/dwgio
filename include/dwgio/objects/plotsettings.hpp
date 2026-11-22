/***********************************************************************************************************************
 *  @file      dwgio/objects/plotsettings.hpp
 *  @brief     The PLOTSETTINGS object: a named page setup.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_PLOTSETTINGS_HPP
#define DWGIO_OBJECTS_PLOTSETTINGS_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/plot.hpp"

namespace DwgIO::Objects
{
/**
 *  A PLOTSETTINGS: a named page setup a layout can take its plot settings from. Its fields are those a LAYOUT opens
 *  with.
 */
struct DWGIO_API PlotSettings
{
  DwgIO::Base Base{};
  Objects::Plot Plot{};
  /**
   *  The VIEW plotted when the plot type is 3, a hard pointer; R2000 names it in the plot instead. Group 6, from R2004.
   */
  Handle PlotView{};
  /**
   *  The VISUALSTYLE the shade plot takes when its mode is 4, a soft pointer. Group 333, from R2007.
   */
  Handle VisualStyle{};

  auto operator==(const PlotSettings &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_PLOTSETTINGS_HPP */
