/***********************************************************************************************************************
 *  @file      source/releases/layouts.cpp
 *  @brief     The layouts and page setups as another release holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/layouts.hpp"

#include "releases/passage.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/plotsettings.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/view.hpp"

#include <format>
#include <string>

namespace DwgIO::Releases
{
namespace
{
/**
 *  Whether a plot shades differently from a new layout, which R2000 cannot keep; the shade plot made a new layout's.
 */
auto ResetShadePlot(Objects::Plot &plot) -> bool
{
  const Objects::Plot fresh;
  const bool shaded = plot.Shading != fresh.Shading || plot.Resolution != fresh.Resolution || plot.Dots != fresh.Dots;
  plot.Shading = fresh.Shading;
  plot.Resolution = fresh.Resolution;
  plot.Dots = fresh.Dots;
  return shaded;
}

/**
 *  A page setup's plot view across R2004, which names it by its VIEW record where R2000 names it in the plot: the
 *  record of the name when upgrading, the name of the record when downgrading; a view the table lacks dropped and
 *  noted.
 */
void ConvertPlotView(Objects::PlotSettings &settings, const Passage &passage, const Crossings &crossing)
{
  const Handle self = settings.Base.Self;
  if (crossing.R2004Up && !settings.Plot.View.empty())
  {
    const auto *view = View::Record<Tables::View>(*passage.Drawing, settings.Plot.View);
    if (view == nullptr)
    {
      Note(passage, self, std::format("plot view {} of {} not in its table", settings.Plot.View, passage.What));
    }
    settings.PlotView = view == nullptr ? Handle{} : view->Record.Base.Self;
    settings.Plot.View.clear();
  }
  if (crossing.R2004Down && settings.PlotView)
  {
    const Object *found = Find(*passage.Drawing, settings.PlotView);
    const auto *view = found == nullptr ? nullptr : GetIf<Tables::View>(found);
    if (view == nullptr)
    {
      Note(passage, self, std::format("plot view {} of {} not in its table", HexOf(settings.PlotView), passage.What));
    }
    settings.Plot.View = view == nullptr ? std::string{} : view->Record.Name;
    settings.PlotView = {};
  }
}
} // namespace

void LowerLayout(Objects::Layout &layout, const Passage &passage)
{
  const Handle self = layout.Base.Self;
  if (ResetShadePlot(layout.Plot))
  {
    Note(passage, self, std::format("shade plot of layout {} not kept by R2000", layout.Name));
  }
  if (layout.PlotView)
  {
    Note(passage, self, std::format("plot view of layout {} not kept by R2000", layout.Name));
    layout.PlotView = {};
  }
  if (!layout.Viewports.empty())
  {
    Note(passage, self, std::format("viewports of layout {} not kept by R2000", layout.Name));
    layout.Viewports.clear();
  }
}

void ConvertPlotSettings(Objects::PlotSettings &settings, const Passage &passage, const Crossings &crossing)
{
  const Handle self = settings.Base.Self;
  ConvertPlotView(settings, passage, crossing);
  if (crossing.R2004Down && ResetShadePlot(settings.Plot))
  {
    Note(passage, self, std::format("shade plot of {} not kept by R2000", passage.What));
  }
  if (crossing.R2007Down && settings.VisualStyle)
  {
    Note(passage, self, std::format("visual style of {} not kept by R2004", passage.What));
    settings.VisualStyle = {};
  }
}
} // namespace DwgIO::Releases
