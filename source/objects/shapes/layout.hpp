/***********************************************************************************************************************
 *  @file      source/objects/shapes/layout.hpp
 *  @brief     How a LAYOUT lies in the file, chapter 20.4.84.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_LAYOUT_HPP
#define DWGIO_OBJECTS_SHAPES_LAYOUT_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/layout.hpp"
#include "dwgio/tables/axes.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <string_view>

namespace DwgIO::Objects::Shapes
{
constexpr int LayoutSoftPointer = 4;

constexpr int LayoutHardPointer = 5;

inline auto ReadOrder(Layout &layout, const Fields::Input &in) -> bool
{
  layout.Order = in.In->BL();
  return !in.In->HasFailed();
}

inline auto WriteOrder(const Layout &layout, const Fields::Output &to) -> bool
{
  to.To->BL(layout.Order);
  return true;
}

/**
 *  A BL in AutoCAD's files, where the text says RL.
 */
inline auto ReadViewportCount(Layout &layout, const Fields::Input &in) -> bool
{
  // A handle takes a byte at least, so a count beyond the handle stream's bytes is garbage, not a list to size; a
  // reader that failed gives zero, and the frame notes the cut.
  const auto count = static_cast<std::uint32_t>(in.In->BL());
  if (count > in.Handles->Size())
  {
    Fields::Note(in, IssueCode::Damaged, std::format("viewport count {} beyond the handles", count));
    return false;
  }
  layout.Viewports.assign(count, Handle{});
  return true;
}

inline auto WriteViewportCount(const Layout &layout, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(layout.Viewports.size()));
  return true;
}

inline auto ReadViewports(Layout &layout, const Fields::Input &in) -> bool
{
  return std::ranges::all_of(layout.Viewports, [&in](Handle &viewport) -> bool
                             { return Fields::ReadLink(viewport, LayoutSoftPointer, in); });
}

inline auto WriteViewports(const Layout &layout, const Fields::Output &to) -> bool
{
  return std::ranges::all_of(layout.Viewports, [&to](const Handle viewport) -> bool
                             { return Fields::WriteLink(viewport, LayoutSoftPointer, to); });
}

inline auto PlotLayout()
{
  return Fields::Layout(Fields::Text(&Plot::Name), Fields::Text(&Plot::Device), Fields::Short(&Plot::Flags),
                        Fields::Double(&Plot::Left), Fields::Double(&Plot::Bottom), Fields::Double(&Plot::Right),
                        Fields::Double(&Plot::Top), Fields::Double(&Plot::Width), Fields::Double(&Plot::Height),
                        Fields::Text(&Plot::Size), Fields::Point2(&Plot::Origin), Fields::Short(&Plot::Units),
                        Fields::Short(&Plot::Rotation), Fields::Short(&Plot::Type), Fields::Point2(&Plot::Lower),
                        Fields::Point2(&Plot::Upper), Fields::Until(Version::R2000, Fields::Text(&Plot::View)),
                        Fields::Double(&Plot::PaperUnits), Fields::Double(&Plot::DrawingUnits),
                        Fields::Text(&Plot::Style), Fields::Short(&Plot::Standard), Fields::Double(&Plot::Factor),
                        Fields::Point2(&Plot::Image),
                        Fields::Since(Version::R2004, Fields::Short(&Plot::Shading), Fields::Short(&Plot::Resolution),
                                      Fields::Short(&Plot::Dots)));
}
} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Objects::Layout>
{
  static constexpr std::array<std::uint16_t, 1> Types{82};
  /**
   *  AutoCAD's files store it under its class before R2004, under the type from it.
   */
  static constexpr Version Fixed = Version::R2004;
  static constexpr std::string_view Name = "LAYOUT";

  static auto Layout()
  {
    using Objects::Layout;
    using Tables::Axes;
    return Fields::Layout(
        Fields::Part(&Layout::Plot, Shapes::PlotLayout()), Fields::Text(&Layout::Name),
        Fields::Custom(Shapes::ReadOrder, Shapes::WriteOrder), Fields::Short(&Layout::Flags),
        Fields::Part(&Layout::Axes, Fields::Layout(Fields::Point3(&Axes::Origin))), Fields::RawPoint2(&Layout::Limmin),
        Fields::RawPoint2(&Layout::Limmax), Fields::Point3(&Layout::Insbase),
        Fields::Part(&Layout::Axes, Fields::Layout(Fields::Point3(&Axes::X), Fields::Point3(&Axes::Y),
                                                   Fields::Double(&Axes::Elevation), Fields::Short(&Axes::Ortho))),
        Fields::Point3(&Layout::Extmin), Fields::Point3(&Layout::Extmax),
        Fields::Since(Version::R2004, Fields::Custom(Shapes::ReadViewportCount, Shapes::WriteViewportCount),
                      Fields::Link(&Layout::PlotView, Shapes::LayoutHardPointer)),
        Fields::Since(Version::R2007, Fields::Link(&Layout::VisualStyle, Shapes::LayoutSoftPointer)),
        Fields::Link(&Layout::Block, Shapes::LayoutSoftPointer),
        Fields::Link(&Layout::Viewport, Shapes::LayoutSoftPointer),
        Fields::Part(&Layout::Axes, Fields::Layout(Fields::Link(&Axes::Basis, Shapes::LayoutHardPointer),
                                                   Fields::Link(&Axes::Named, Shapes::LayoutHardPointer))),
        Fields::Since(Version::R2004, Fields::Custom(Shapes::ReadViewports, Shapes::WriteViewports)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_LAYOUT_HPP */
