/***********************************************************************************************************************
 *  @file      source/tables/shapes/vport.hpp
 *  @brief     How a VPORT record lies in the file, chapter 20.4.64.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_VPORT_HPP
#define DWGIO_TABLES_SHAPES_VPORT_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"
#include "tables/shapes/view.hpp"

#include "dwgio/tables/axes.hpp"
#include "dwgio/tables/vport.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Tables::Shapes
{
constexpr unsigned Follow = 8;

constexpr unsigned IconShown = 1;

constexpr unsigned IconAtOrigin = 2;

/**
 *  The view mode's four bits, as a view's; the 8 bit comes later, after the corners.
 */
inline auto ReadVportMode(VPort &vport, const Fields::Input &in) -> bool
{
  const auto first = static_cast<unsigned>(in.In->B());
  const auto second = static_cast<unsigned>(in.In->B());
  const auto third = static_cast<unsigned>(in.In->B());
  const auto fourth = static_cast<unsigned>(in.In->B());
  vport.Mode = static_cast<int>((first * Perspective) | (second * FrontClip) | (third * BackClip) |
                                ((fourth ^ 1U) * FrontAtEye));
  return !in.In->HasFailed();
}

inline auto WriteVportMode(const VPort &vport, const Fields::Output &to) -> bool
{
  const auto mode = static_cast<unsigned>(vport.Mode);
  to.To->B((mode & Perspective) != 0);
  to.To->B((mode & FrontClip) != 0);
  to.To->B((mode & BackClip) != 0);
  to.To->B((mode & FrontAtEye) == 0);
  return true;
}

/**
 *  UCSFOLLOW, the 8 bit of group 71.
 */
inline auto ReadFollow(VPort &vport, const Fields::Input &in) -> bool
{
  vport.Mode =
      static_cast<int>((static_cast<unsigned>(vport.Mode) & ~Follow) | (static_cast<unsigned>(in.In->B()) * Follow));
  return !in.In->HasFailed();
}

inline auto WriteFollow(const VPort &vport, const Fields::Output &to) -> bool
{
  to.To->B((static_cast<unsigned>(vport.Mode) & Follow) != 0);
  return true;
}

inline auto ReadIcon(VPort &vport, const Fields::Input &in) -> bool
{
  const auto shown = static_cast<unsigned>(in.In->B());
  const auto origin = static_cast<unsigned>(in.In->B());
  vport.UcsIcon = static_cast<int>((shown * IconShown) | (origin * IconAtOrigin));
  return !in.In->HasFailed();
}

inline auto WriteIcon(const VPort &vport, const Fields::Output &to) -> bool
{
  const auto icon = static_cast<unsigned>(vport.UcsIcon);
  to.To->B((icon & IconShown) != 0);
  to.To->B((icon & IconAtOrigin) != 0);
  return true;
}
} // namespace DwgIO::Tables::Shapes

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.64; the width is stored where the chapter puts the aspect ratio.
 */
template <>
struct Shape<Tables::VPort>
{
  static constexpr std::array<std::uint16_t, 1> Types{65};
  static constexpr std::string_view Name = "VPORT";

  static auto Layout()
  {
    using Tables::Axes;
    using Tables::VPort;
    using namespace Tables::Shapes;
    return Fields::Layout(
        Fields::Part(&VPort::Record, RecordLayout()), Fields::Double(&VPort::ViewHeight),
        Fields::Double(&VPort::ViewWidth), Fields::RawPoint2(&VPort::ViewCenter), Fields::Point3(&VPort::Target),
        Fields::Point3(&VPort::Direction), Fields::Double(&VPort::Twist), Fields::Double(&VPort::LensLength),
        Fields::Double(&VPort::Front), Fields::Double(&VPort::Back), Fields::Custom(ReadVportMode, WriteVportMode),
        Fields::Since(Version::R2000, Fields::Byte(&VPort::Render)),
        Fields::Since(Version::R2007, Fields::Bit(&VPort::DefaultLighting), Fields::Byte(&VPort::DefaultLightingType),
                      Fields::Double(&VPort::Brightness), Fields::Double(&VPort::Contrast),
                      Fields::CmColor(&VPort::Ambient)),
        Fields::RawPoint2(&VPort::Lower), Fields::RawPoint2(&VPort::Upper), Fields::Custom(ReadFollow, WriteFollow),
        Fields::Short(&VPort::Zoom), Fields::Bit(&VPort::FastZoom), Fields::Custom(ReadIcon, WriteIcon),
        Fields::Bit(&VPort::GridOn), Fields::RawPoint2(&VPort::Grid), Fields::Bit(&VPort::SnapOn),
        Fields::Bit(&VPort::Isometric), Fields::Short(&VPort::Isoplane), Fields::Double(&VPort::SnapAngle),
        Fields::RawPoint2(&VPort::SnapBase), Fields::RawPoint2(&VPort::Snap),
        Fields::Since(
            Version::R2000, Fields::Bit(&VPort::Unknown1), Fields::Bit(&VPort::UcsPerViewport),
            Fields::Part(&VPort::Axes, Fields::Layout(Fields::Point3(&Axes::Origin), Fields::Point3(&Axes::X),
                                                      Fields::Point3(&Axes::Y), Fields::Double(&Axes::Elevation),
                                                      Fields::Short(&Axes::Ortho)))),
        Fields::Since(Version::R2007, Fields::Short(&VPort::GridDisplay), Fields::Short(&VPort::GridMajor),
                      Fields::Link(&VPort::Background, SoftPointer), Fields::Link(&VPort::VisualStyle, HardPointer),
                      Fields::Link(&VPort::Sun, HardOwner)),
        Fields::Since(Version::R2000,
                      Fields::Part(&VPort::Axes, Fields::Layout(Fields::Link(&Axes::Named, HardPointer),
                                                                Fields::Link(&Axes::Basis, HardPointer)))));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_VPORT_HPP */
