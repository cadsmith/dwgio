/***********************************************************************************************************************
 *  @file      source/entities/shapes/ole2frame.hpp
 *  @brief     How an OLE2FRAME lies in the file, chapter 20.4.88.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_OLE2FRAME_HPP
#define DWGIO_ENTITIES_SHAPES_OLE2FRAME_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/ole2frame.hpp"
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
 *  A BL count, then that many bytes. A count beyond what is left is noted and the data kept empty.
 */
inline auto ReadOleData(Ole2Frame &frame, const Fields::Input &in) -> bool
{
  const std::int32_t count = in.In->BL();
  if (in.In->HasFailed())
  {
    return false;
  }
  const std::size_t room = (in.In->Size() - in.In->Position()) / 8;
  if (count < 0 || std::cmp_greater(count, room))
  {
    Fields::Note(in, IssueCode::Damaged, std::format("OLE data size {} out of range", count));
    return false;
  }
  frame.Data = in.In->Bytes(static_cast<std::size_t>(count));
  return !in.In->HasFailed();
}

inline auto WriteOleData(const Ole2Frame &frame, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(frame.Data.size()));
  to.To->Raw(frame.Data);
  return true;
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Ole2Frame>
{
  static constexpr std::array<std::uint16_t, 1> Types{74};
  static constexpr std::string_view Name = "OLE2FRAME";

  static auto Layout()
  {
    using Entities::Ole2Frame;
    return Fields::Layout(Fields::Short(&Ole2Frame::Version),
                          Fields::Since(Version::R2000, Fields::Short(&Ole2Frame::Mode)),
                          Fields::Custom(Entities::Shapes::ReadOleData, Entities::Shapes::WriteOleData),
                          Fields::Since(Version::R2000, Fields::Char(&Ole2Frame::Unknown1)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_OLE2FRAME_HPP */
