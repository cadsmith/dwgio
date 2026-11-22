/***********************************************************************************************************************
 *  @file      source/objects/shapes/plotsettings.hpp
 *  @brief     How a PLOTSETTINGS lies in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_PLOTSETTINGS_HPP
#define DWGIO_OBJECTS_SHAPES_PLOTSETTINGS_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/layout.hpp"

#include "dwgio/objects/plotsettings.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  What a LAYOUT opens with, chapter 20.4.84; stored under its class.
 */
template <>
struct Shape<Objects::PlotSettings>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "PLOTSETTINGS";

  static auto Layout()
  {
    using Objects::PlotSettings;
    return Fields::Layout(
        Fields::Part(&PlotSettings::Plot, Shapes::PlotLayout()),
        Fields::Since(Version::R2004, Fields::Link(&PlotSettings::PlotView, Shapes::LayoutHardPointer)),
        Fields::Since(Version::R2007, Fields::Link(&PlotSettings::VisualStyle, Shapes::LayoutSoftPointer)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_PLOTSETTINGS_HPP */
