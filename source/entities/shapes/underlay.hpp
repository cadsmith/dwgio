/***********************************************************************************************************************
 *  @file      source/entities/shapes/underlay.hpp
 *  @brief     How the PDF, DWF and DGN underlays lie in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_UNDERLAY_HPP
#define DWGIO_ENTITIES_SHAPES_UNDERLAY_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/underlay.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace DwgIO::Entities::Shapes
{
/**
 *  Two RDs.
 */
constexpr std::size_t BoundBits = 128;

/**
 *  A BL count, then each vertex as 2RD. A count beyond what is left is noted and the clip kept empty.
 */
inline auto ReadClip(UnderlayData &overlaid, const Fields::Input &in) -> bool
{
  const std::int32_t count = in.In->BL();
  if (in.In->HasFailed())
  {
    return false;
  }
  const std::size_t room = (in.In->Size() - in.In->Position()) / BoundBits;
  if (count < 0 || std::cmp_greater(count, room))
  {
    Fields::Note(in, IssueCode::Damaged, std::format("underlay clip count {} out of range", count));
    return false;
  }
  overlaid.Clip.resize(static_cast<std::size_t>(count));
  for (Entities::Bound &bound : overlaid.Clip)
  {
    bound.Position.X = in.In->RD();
    bound.Position.Y = in.In->RD();
  }
  return !in.In->HasFailed();
}

inline auto WriteClip(const UnderlayData &overlaid, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(overlaid.Clip.size()));
  for (const Entities::Bound &bound : overlaid.Clip)
  {
    to.To->RD(bound.Position.X);
    to.To->RD(bound.Position.Y);
  }
  return true;
}

inline auto OverlaidLayout()
{
  return Fields::Layout(Fields::Point3(&UnderlayData::Extrusion), Fields::Point3(&UnderlayData::Position),
                        Fields::Double(&UnderlayData::Rotation), Fields::Point3(&UnderlayData::Scale),
                        Fields::Byte(&UnderlayData::Flags), Fields::Byte(&UnderlayData::Contrast),
                        Fields::Byte(&UnderlayData::Fade), Fields::Custom(ReadClip, WriteClip),
                        Fields::Link(&UnderlayData::Definition, 5));
}

/**
 *  In the order of UnderlayKind.
 */
constexpr std::array<std::string_view, 3> UnderlayNames{"PDFUNDERLAY", "DWFUNDERLAY", "DGNUNDERLAY"};
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
/**
 *  The three kinds differ only in their class.
 */
template <Entities::UnderlayKind KindV>
struct Shape<Entities::UnderlayOf<KindV>>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = Entities::Shapes::UnderlayNames.at(static_cast<std::size_t>(KindV));

  static auto Layout()
  {
    return Fields::Layout(Fields::Part(&Entities::UnderlayOf<KindV>::Data, Entities::Shapes::OverlaidLayout()));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_UNDERLAY_HPP */
