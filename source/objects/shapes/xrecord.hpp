/***********************************************************************************************************************
 *  @file      source/objects/shapes/xrecord.hpp
 *  @brief     How an XRECORD lies in the file, chapter 20.4.104.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_XRECORD_HPP
#define DWGIO_OBJECTS_SHAPES_XRECORD_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/items.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/xrecord.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace DwgIO::Objects::Shapes
{
constexpr std::size_t XrecordByte = 8;

constexpr int SoftPointer = 4;

/**
 *  The byte count, the groups, from R2000 the cloning flag, then every handle the stream has left.
 */
inline auto ReadXRecord(XRecord &xrecord, const Fields::Input &in) -> bool
{
  const std::int32_t bytes = in.In->BL();
  if (in.In->HasFailed())
  {
    return false;
  }
  const std::size_t room = (in.In->Size() - in.In->Position()) / XrecordByte;
  if (bytes < 0 || std::cmp_greater(bytes, room))
  {
    Fields::Note(in, IssueCode::Damaged, std::format("XRECORD data of {} bytes out of range", bytes));
    return false;
  }
  auto [groups, rest] = ReadGroups(in, static_cast<std::size_t>(bytes));
  xrecord.Data = std::move(groups);
  xrecord.Rest = std::move(rest);
  if (in.With.Release >= Version::R2000)
  {
    xrecord.Cloning = in.In->BS();
  }
  if (in.In->HasFailed() || in.Handles == nullptr)
  {
    return !in.In->HasFailed();
  }
  while (in.Handles->Size() - in.Handles->Position() >= XrecordByte)
  {
    Handle handle;
    if (!Fields::ReadLink(handle, SoftPointer, in))
    {
      break;
    }
    xrecord.Objects.push_back(handle);
  }
  return true;
}

inline auto WriteXRecord(const XRecord &xrecord, const Fields::Output &to)
{
  Bits::Writer data;
  const Fields::Output inner{.To = &data, .Handles = to.Handles, .With = to.With};
  WriteGroups(xrecord.Data, xrecord.Rest, inner);
  to.To->BL(static_cast<std::int32_t>(data.Position() / XrecordByte));
  to.To->Raw(data.Bytes());
  if (to.With.Release >= Version::R2000)
  {
    to.To->BS(xrecord.Cloning);
  }
  for (const Handle handle : xrecord.Objects)
  {
    Fields::WriteLink(handle, SoftPointer, to);
  }
  return true;
}
} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Objects::XRecord>
{
  static constexpr std::array<std::uint16_t, 1> Types{79};
  static constexpr Version Fixed = Version::R2000; /**< A class before R2000, which gave it its type code. */
  static constexpr std::string_view Name = "XRECORD";

  static auto Layout() { return Fields::Layout(Fields::Custom(Shapes::ReadXRecord, Shapes::WriteXRecord)); }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_XRECORD_HPP */
