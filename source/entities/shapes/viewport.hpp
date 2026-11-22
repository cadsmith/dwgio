/***********************************************************************************************************************
 *  @file      source/entities/shapes/viewport.hpp
 *  @brief     How a VIEWPORT lies in the file, chapter 20.4.38.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_VIEWPORT_HPP
#define DWGIO_ENTITIES_SHAPES_VIEWPORT_HPP

#include "entities/shapes/count.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/viewport.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
inline auto ReadFrozenCount(Viewport &viewport, const Fields::Input &in) -> bool
{
  const std::optional<std::size_t> count = ReadCount(in);
  if (!count)
  {
    return false;
  }
  viewport.Frozen.assign(*count, Handle{});
  return true;
}

inline auto WriteFrozenCount(const Viewport &viewport, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(viewport.Frozen.size()));
  return true;
}

/**
 *  A hard pointer in R2000, a soft pointer from R2004.
 */
inline auto FrozenCode(Version release) -> int
{
  return release >= Version::R2004 ? 4 : 5;
}

inline auto ReadFrozen(Viewport &viewport, const Fields::Input &in) -> bool
{
  const int code = FrozenCode(in.With.Release);
  return std::ranges::all_of(viewport.Frozen,
                             [&in, code](Handle &layer) -> bool { return Fields::ReadLink(layer, code, in); });
}

inline auto WriteFrozen(const Viewport &viewport, const Fields::Output &to) -> bool
{
  const int code = FrozenCode(to.With.Release);
  return std::ranges::all_of(viewport.Frozen,
                             [&to, code](const Handle layer) -> bool { return Fields::WriteLink(layer, code, to); });
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
/**
 *  AutoCAD writes the clip entity as a hard pointer where the specification says soft.
 */
template <>
struct Shape<Entities::Viewport>
{
  static constexpr std::array<std::uint16_t, 1> Types{34};
  static constexpr std::string_view Name = "VIEWPORT";

  static auto Layout()
  {
    using Entities::Viewport;
    using Tables::Axes;
    using namespace Entities::Shapes;
    return Fields::Layout(
        Fields::Point3(&Viewport::Center), Fields::Double(&Viewport::Width), Fields::Double(&Viewport::Height),
        Fields::Since(
            Version::R2000, Fields::Point3(&Viewport::Target), Fields::Point3(&Viewport::Direction),
            Fields::Double(&Viewport::Twist), Fields::Double(&Viewport::ViewHeight),
            Fields::Double(&Viewport::LensLength), Fields::Double(&Viewport::Front), Fields::Double(&Viewport::Back),
            Fields::Double(&Viewport::SnapAngle), Fields::RawPoint2(&Viewport::ViewCenter),
            Fields::RawPoint2(&Viewport::SnapBase), Fields::RawPoint2(&Viewport::Snap),
            Fields::RawPoint2(&Viewport::Grid), Fields::Short(&Viewport::Zoom),
            Fields::Since(Version::R2007, Fields::Short(&Viewport::GridMajor)),
            Fields::Custom(ReadFrozenCount, WriteFrozenCount), Fields::Long(&Viewport::Flags),
            Fields::Text(&Viewport::Sheet), Fields::Byte(&Viewport::Render), Fields::Bit(&Viewport::UcsIconAtOrigin),
            Fields::Bit(&Viewport::UcsPerViewport),
            Fields::Part(&Viewport::Axes, Fields::Layout(Fields::Point3(&Axes::Origin), Fields::Point3(&Axes::X),
                                                         Fields::Point3(&Axes::Y), Fields::Double(&Axes::Elevation),
                                                         Fields::Short(&Axes::Ortho)))),
        Fields::Since(Version::R2004, Fields::Short(&Viewport::ShadePlotMode)),
        Fields::Since(Version::R2007, Fields::Bit(&Viewport::DefaultLighting),
                      Fields::Byte(&Viewport::DefaultLightingType), Fields::Double(&Viewport::Brightness),
                      Fields::Double(&Viewport::Contrast), Fields::CmColor(&Viewport::Ambient)),
        Fields::Until(Version::R14, Fields::Link(&Viewport::Header, 5)),
        Fields::Since(Version::R2000, Fields::Custom(ReadFrozen, WriteFrozen), Fields::Link(&Viewport::Clip, 5),
                      Fields::Until(Version::R2000, Fields::Link(&Viewport::Header, 5)),
                      Fields::Part(&Viewport::Axes,
                                   Fields::Layout(Fields::Link(&Axes::Named, 5), Fields::Link(&Axes::Basis, 5)))),
        Fields::Since(Version::R2007, Fields::Link(&Viewport::Background, 4), Fields::Link(&Viewport::VisualStyle, 5),
                      Fields::Link(&Viewport::ShadePlotObject, 4), Fields::Link(&Viewport::Sun, 3)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_VIEWPORT_HPP */
