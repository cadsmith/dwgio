/***********************************************************************************************************************
 *  @file      source/tables/shapes/view.hpp
 *  @brief     How a VIEW record lies in the file, chapter 20.4.60.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_VIEW_HPP
#define DWGIO_TABLES_SHAPES_VIEW_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/axes.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Tables::Shapes
{
constexpr unsigned Perspective = 1;

constexpr unsigned FrontClip = 2;

constexpr unsigned BackClip = 4;

constexpr unsigned FrontAtEye = 16;

/**
 *  Four bits: the 1, 2 and 4 of group 71, then the opposite of its 16.
 */
inline auto ReadMode(View &view, const Fields::Input &in) -> bool
{
  const auto first = static_cast<unsigned>(in.In->B());
  const auto second = static_cast<unsigned>(in.In->B());
  const auto third = static_cast<unsigned>(in.In->B());
  const auto fourth = static_cast<unsigned>(in.In->B());
  view.Mode = static_cast<int>((first * Perspective) | (second * FrontClip) | (third * BackClip) |
                               ((fourth ^ 1U) * FrontAtEye));
  return !in.In->HasFailed();
}

inline auto WriteMode(const View &view, const Fields::Output &to) -> bool
{
  const auto mode = static_cast<unsigned>(view.Mode);
  to.To->B((mode & Perspective) != 0);
  to.To->B((mode & FrontClip) != 0);
  to.To->B((mode & BackClip) != 0);
  to.To->B((mode & FrontAtEye) == 0);
  return true;
}

inline auto IsAssociated(const View &view) -> bool
{
  return view.Associated;
}
} // namespace DwgIO::Tables::Shapes

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.60; the UCS axes only when the associated bit is set, the UCS handles always.
 */
template <>
struct Shape<Tables::View>
{
  static constexpr std::array<std::uint16_t, 1> Types{61};
  static constexpr std::string_view Name = "VIEW";

  static auto Layout()
  {
    using Tables::Axes;
    using Tables::View;
    using namespace Tables::Shapes;
    return Fields::Layout(
        Fields::Part(&View::Record, RecordLayout()), Fields::Double(&View::ViewHeight),
        Fields::Double(&View::ViewWidth), Fields::RawPoint2(&View::ViewCenter), Fields::Point3(&View::Target),
        Fields::Point3(&View::Direction), Fields::Double(&View::Twist), Fields::Double(&View::LensLength),
        Fields::Double(&View::Front), Fields::Double(&View::Back), Fields::Custom(ReadMode, WriteMode),
        Fields::Since(Version::R2000, Fields::Byte(&View::Render)),
        Fields::Since(Version::R2007, Fields::Bit(&View::DefaultLighting), Fields::Byte(&View::DefaultLightingType),
                      Fields::Double(&View::Brightness), Fields::Double(&View::Contrast),
                      Fields::CmColor(&View::Ambient)),
        Fields::Bit(&View::Paper),
        Fields::Since(
            Version::R2000, Fields::Bit(&View::Associated),
            Fields::When(
                IsAssociated,
                Fields::Part(&View::Axes, Fields::Layout(Fields::Point3(&Axes::Origin), Fields::Point3(&Axes::X),
                                                         Fields::Point3(&Axes::Y), Fields::Double(&Axes::Elevation),
                                                         Fields::Short(&Axes::Ortho))))),
        Fields::Since(Version::R2007, Fields::Bit(&View::Plottable), Fields::Link(&View::Background, SoftPointer),
                      Fields::Link(&View::VisualStyle, HardPointer), Fields::Link(&View::Sun, HardOwner)),
        Fields::Since(Version::R2000,
                      Fields::Part(&View::Axes, Fields::Layout(Fields::Link(&Axes::Basis, HardPointer),
                                                               Fields::Link(&Axes::Named, HardPointer)))),
        Fields::Since(Version::R2007, Fields::Link(&View::Section, SoftPointer)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_VIEW_HPP */
