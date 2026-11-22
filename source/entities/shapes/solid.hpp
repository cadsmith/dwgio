/***********************************************************************************************************************
 *  @file      source/entities/shapes/solid.hpp
 *  @brief     How a SOLID lies in the file, chapter 20.4.35.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_SOLID_HPP
#define DWGIO_ENTITIES_SHAPES_SOLID_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/solid.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  The Z the corners share as a BD, then each corner's X and Y as RDs.
 */
template <typename KindT>
auto ReadCorners(KindT &kind, const Fields::Input &in) -> bool
{
  const double elevation = in.In->BD();
  for (Vector &corner : kind.Corners)
  {
    corner.X = in.In->RD();
    corner.Y = in.In->RD();
    corner.Z = elevation;
  }
  return !in.In->HasFailed();
}

template <typename KindT>
auto WriteCorners(const KindT &kind, const Fields::Output &to) -> bool
{
  to.To->BD(kind.Corners.front().Z);
  for (const Vector &corner : kind.Corners)
  {
    to.To->RD(corner.X);
    to.To->RD(corner.Y);
  }
  return true;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Solid>
{
  static constexpr std::array<std::uint16_t, 1> Types{31};
  static constexpr std::string_view Name = "SOLID";

  static auto Layout()
  {
    using Entities::Solid;
    return Fields::Layout(Fields::Thickness(&Solid::Thickness),
                          Fields::Custom(Entities::Shapes::ReadCorners<Solid>, Entities::Shapes::WriteCorners<Solid>),
                          Fields::Extrusion(&Solid::Extrusion));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_SOLID_HPP */
