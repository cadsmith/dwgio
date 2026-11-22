/***********************************************************************************************************************
 *  @file      source/entities/shapes/face.hpp
 *  @brief     How a 3DFACE lies in the file, chapter 20.4.32, packed from R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_FACE_HPP
#define DWGIO_ENTITIES_SHAPES_FACE_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/face.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
inline auto ReadOlderFace(Face3d &face, const Fields::Input &in) -> bool
{
  for (Vector &corner : face.Corners)
  {
    corner = in.In->Point3();
  }
  face.Flags = in.In->BS();
  return !in.In->HasFailed();
}

inline auto WriteOlderFace(const Face3d &face, const Fields::Output &to) -> bool
{
  for (const Vector &corner : face.Corners)
  {
    to.To->Point3(corner);
  }
  to.To->BS(face.Flags);
  return true;
}

/**
 *  From R2000: a bit set when the flags are zero and left out, one set when the first corner's Z is zero and left out,
 *  the first corner as RDs, each other corner as DDs against the one before, then the flags.
 */
inline auto ReadFace(Face3d &face, const Fields::Input &in) -> bool
{
  const bool unflagged = in.In->B();
  const bool flat = in.In->B();
  Vector &first = face.Corners.front();
  first.X = in.In->RD();
  first.Y = in.In->RD();
  first.Z = flat ? 0.0 : in.In->RD();
  for (std::size_t at = 1; at < face.Corners.size(); ++at)
  {
    const Vector &before = face.Corners.at(at - 1);
    Vector &corner = face.Corners.at(at);
    corner.X = in.In->DD(before.X);
    corner.Y = in.In->DD(before.Y);
    corner.Z = in.In->DD(before.Z);
  }
  face.Flags = unflagged ? 0 : in.In->BS();
  return !in.In->HasFailed();
}

inline auto WriteFace(const Face3d &face, const Fields::Output &to) -> bool
{
  const bool unflagged = face.Flags == 0;
  const Vector &first = face.Corners.front();
  const bool flat = first.Z == 0;
  to.To->B(unflagged);
  to.To->B(flat);
  to.To->RD(first.X);
  to.To->RD(first.Y);
  if (!flat)
  {
    to.To->RD(first.Z);
  }
  for (std::size_t at = 1; at < face.Corners.size(); ++at)
  {
    const Vector &before = face.Corners.at(at - 1);
    const Vector &corner = face.Corners.at(at);
    to.To->DD(corner.X, before.X);
    to.To->DD(corner.Y, before.Y);
    to.To->DD(corner.Z, before.Z);
  }
  if (!unflagged)
  {
    to.To->BS(face.Flags);
  }
  return true;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Face3d>
{
  static constexpr std::array<std::uint16_t, 1> Types{28};
  static constexpr std::string_view Name = "3DFACE";

  static auto Layout()
  {
    return Fields::Layout(
        Fields::Until(Version::R14, Fields::Custom(Entities::Shapes::ReadOlderFace, Entities::Shapes::WriteOlderFace)),
        Fields::Since(Version::R2000, Fields::Custom(Entities::Shapes::ReadFace, Entities::Shapes::WriteFace)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_FACE_HPP */
