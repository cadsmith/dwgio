/***********************************************************************************************************************
 *  @file      source/entities/shapes/raster.hpp
 *  @brief     How an IMAGE and a WIPEOUT lie in the file, chapter 20.4.80.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_RASTER_HPP
#define DWGIO_ENTITIES_SHAPES_RASTER_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/raster.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace DwgIO::Entities::Shapes
{
/**
 *  A rectangular clipping boundary is stored as its two corners with no count.
 */
constexpr int Rectangle = 1;

/**
 *  Two RDs.
 */
constexpr std::size_t PixelBits = 128;

/**
 *  The kind, then a rectangle's two corners or a polygon's BL count and vertices, each 2RD. A count beyond what is left
 *  is noted and the boundary kept empty.
 */
inline auto ReadBoundary(Raster &raster, const Fields::Input &in) -> bool
{
  raster.ClipShape = in.In->BS();
  std::int32_t count = 2;
  if (raster.ClipShape != Rectangle)
  {
    count = in.In->BL();
  }
  if (in.In->HasFailed())
  {
    return false;
  }
  const std::size_t room = (in.In->Size() - in.In->Position()) / PixelBits;
  if (count < 0 || std::cmp_greater(count, room))
  {
    Fields::Note(in, IssueCode::Damaged, std::format("clipping boundary count {} out of range", count));
    return false;
  }
  raster.Corners.resize(static_cast<std::size_t>(count));
  for (Pixel &corner : raster.Corners)
  {
    corner.Position.X = in.In->RD();
    corner.Position.Y = in.In->RD();
  }
  return !in.In->HasFailed();
}

/**
 *  A rectangle of other than two corners is written with two, the missing ones at the origin, and noted.
 */
inline auto WriteBoundary(const Raster &raster, const Fields::Output &to) -> bool
{
  to.To->BS(raster.ClipShape);
  std::size_t count = raster.Corners.size();
  if (raster.ClipShape == Rectangle && count != 2)
  {
    Fields::Note(to, IssueCode::OutOfRange, std::format("clipping rectangle of {} corners written with 2", count));
    count = 2;
  }
  else if (raster.ClipShape != Rectangle)
  {
    to.To->BL(static_cast<std::int32_t>(count));
  }
  for (std::size_t at = 0; at < count; ++at)
  {
    const Vector corner = at < raster.Corners.size() ? raster.Corners[at].Position : Vector{};
    to.To->RD(corner.X);
    to.To->RD(corner.Y);
  }
  return true;
}

/**
 *  Chapter 20.4.80 after the common entity data.
 */
inline auto RasterLayout()
{
  return Fields::Layout(Fields::Long(&Raster::Version), Fields::Point3(&Raster::Origin),
                        Fields::Point3(&Raster::UVector), Fields::Point3(&Raster::VVector),
                        Fields::RawPoint2(&Raster::Size), Fields::Short(&Raster::Flags), Fields::Bit(&Raster::Clipped),
                        Fields::Byte(&Raster::Brightness), Fields::Byte(&Raster::Contrast), Fields::Byte(&Raster::Fade),
                        Fields::Since(Version::R2010, Fields::Bit(&Raster::Outside)),
                        Fields::Custom(ReadBoundary, WriteBoundary), Fields::Link(&Raster::Definition, 5),
                        Fields::Link(&Raster::Reactor, 3));
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.80; stored under its class.
 */
template <>
struct Shape<Entities::Image>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "IMAGE";

  static auto Layout()
  {
    return Fields::Layout(Fields::Part(&Entities::Image::Raster, Entities::Shapes::RasterLayout()));
  }
};

/**
 *  Laid out as an image with a null definition and reactor; stored under its class.
 */
template <>
struct Shape<Entities::Wipeout>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "WIPEOUT";

  static auto Layout()
  {
    return Fields::Layout(Fields::Part(&Entities::Wipeout::Raster, Entities::Shapes::RasterLayout()));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_RASTER_HPP */
