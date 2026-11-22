/***********************************************************************************************************************
 *  @file      source/entities/shapes/line.hpp
 *  @brief     How a LINE lies in the file, chapter 20.4.21, packed from R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_LINE_HPP
#define DWGIO_ENTITIES_SHAPES_LINE_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/line.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  From R2000: a bit set when both Z are zero, the start's X and Y as RDs with the end's as DDs against them, and the
 *  two Z the same way when the bit is clear.
 */
inline auto ReadLine(Line &line, const Fields::Input &in) -> bool
{
  const bool flat = in.In->B();
  line.Start.X = in.In->RD();
  line.End.X = in.In->DD(line.Start.X);
  line.Start.Y = in.In->RD();
  line.End.Y = in.In->DD(line.Start.Y);
  line.Start.Z = 0;
  line.End.Z = 0;
  if (!flat)
  {
    line.Start.Z = in.In->RD();
    line.End.Z = in.In->DD(line.Start.Z);
  }
  return !in.In->HasFailed();
}

inline auto WriteLine(const Line &line, const Fields::Output &to) -> bool
{
  const bool flat = line.Start.Z == 0 && line.End.Z == 0;
  to.To->B(flat);
  to.To->RD(line.Start.X);
  to.To->DD(line.End.X, line.Start.X);
  to.To->RD(line.Start.Y);
  to.To->DD(line.End.Y, line.Start.Y);
  if (!flat)
  {
    to.To->RD(line.Start.Z);
    to.To->DD(line.End.Z, line.Start.Z);
  }
  return true;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Line>
{
  static constexpr std::array<std::uint16_t, 1> Types{19};
  static constexpr std::string_view Name = "LINE";

  static auto Layout()
  {
    using Entities::Line;
    return Fields::Layout(
        Fields::Until(Version::R14, Fields::Point3(&Line::Start), Fields::Point3(&Line::End)),
        Fields::Since(Version::R2000, Fields::Custom(Entities::Shapes::ReadLine, Entities::Shapes::WriteLine)),
        Fields::Thickness(&Line::Thickness), Fields::Extrusion(&Line::Extrusion));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_LINE_HPP */
