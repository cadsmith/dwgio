/***********************************************************************************************************************
 *  @file      source/entities/shapes/insert.hpp
 *  @brief     How an INSERT and a MINSERT lie in the file, with their scale and attributes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_INSERT_HPP
#define DWGIO_ENTITIES_SHAPES_INSERT_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/insert.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  From R2000 two bits: 3 for unit scale and nothing more, 1 for X at one with Y and Z as deltas against one, 2 for one
 *  double shared by the three, 0 for X as a double with Y and Z as deltas against it.
 */
inline auto ReadScale(Insert &insert, const Fields::Input &in) -> bool
{
  const int form = in.In->BB();
  Vector &scale = insert.Scale;
  scale = Vector{.X = 1, .Y = 1, .Z = 1};
  if (form == 1)
  {
    scale.Y = in.In->DD(1);
    scale.Z = in.In->DD(1);
  }
  else if (form == 2)
  {
    scale.X = in.In->RD();
    scale.Y = scale.X;
    scale.Z = scale.X;
  }
  else if (form == 0)
  {
    scale.X = in.In->RD();
    scale.Y = in.In->DD(scale.X);
    scale.Z = in.In->DD(scale.X);
  }
  return !in.In->HasFailed();
}

inline auto WriteScale(const Insert &insert, const Fields::Output &to) -> bool
{
  const Vector &scale = insert.Scale;
  if (scale.X == 1 && scale.Y == 1 && scale.Z == 1)
  {
    to.To->BB(3);
  }
  else if (scale.X == 1)
  {
    to.To->BB(1);
    to.To->DD(scale.Y, 1);
    to.To->DD(scale.Z, 1);
  }
  else if (scale.Y == scale.X && scale.Z == scale.X)
  {
    to.To->BB(2);
    to.To->RD(scale.X);
  }
  else
  {
    to.To->BB(0);
    to.To->RD(scale.X);
    to.To->DD(scale.Y, scale.X);
    to.To->DD(scale.Z, scale.X);
  }
  return true;
}

inline auto IsGrid(const Insert &insert) -> bool
{
  return insert.Columns.has_value();
}

inline auto ReadGrid(Insert &insert, const Fields::Input &in) -> bool
{
  insert.Columns = in.In->BS();
  insert.Rows = in.In->BS();
  insert.ColumnSpacing = in.In->BD();
  insert.RowSpacing = in.In->BD();
  return !in.In->HasFailed();
}

inline auto WriteGrid(const Insert &insert, const Fields::Output &to) -> bool
{
  to.To->BS(insert.Columns.value_or(1));
  to.To->BS(insert.Rows.value_or(1));
  to.To->BD(insert.ColumnSpacing);
  to.To->BD(insert.RowSpacing);
  return true;
}

inline auto HasAttributes(const Insert &insert) -> bool
{
  return insert.Follows;
}

inline auto ReadOwnedCount(Insert &insert, const Fields::Input &in) -> bool
{
  std::size_t count = 0;
  if (!Fields::ReadHandleCount(in, count))
  {
    return false;
  }
  insert.Owned.assign(count, Handle{});
  return true;
}

inline auto WriteOwnedCount(const Insert &insert, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(insert.Owned.size()));
  return true;
}

/**
 *  Hard owners of an INSERT, soft pointers of a MINSERT.
 */
inline auto ReadOwned(Insert &insert, const Fields::Input &in) -> bool
{
  const int code = IsGrid(insert) ? 4 : 3;
  return std::ranges::all_of(insert.Owned,
                             [&in, code](Handle &owned) -> bool { return Fields::ReadLink(owned, code, in); });
}

inline auto WriteOwned(const Insert &insert, const Fields::Output &to) -> bool
{
  const int code = IsGrid(insert) ? 4 : 3;
  return std::ranges::all_of(insert.Owned,
                             [&to, code](const Handle owned) -> bool { return Fields::WriteLink(owned, code, to); });
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Insert>
{
  static constexpr std::array<std::uint16_t, 2> Types{7, 8};
  static constexpr std::string_view Name = "INSERT";

  static void Open(Entities::Insert &insert, std::uint16_t type)
  {
    if (type == 8)
    {
      insert.Columns = 0;
      insert.Rows = 0;
    }
  }

  static auto Code(const Entities::Insert &insert) -> std::uint16_t { return insert.Columns ? 8 : 7; }

  static auto NameOf(const Entities::Insert &insert) -> std::string_view
  {
    return insert.Columns ? "MINSERT" : "INSERT";
  }

  static auto Layout()
  {
    using Entities::Insert;
    return Fields::Layout(
        Fields::Point3(&Insert::Position), Fields::Until(Version::R14, Fields::Point3(&Insert::Scale)),
        Fields::Since(Version::R2000, Fields::Custom(Entities::Shapes::ReadScale, Entities::Shapes::WriteScale)),
        Fields::Double(&Insert::Rotation), Fields::Point3(&Insert::Extrusion), Fields::Bit(&Insert::Follows),
        Fields::When(Entities::Shapes::HasAttributes,
                     Fields::Since(Version::R2004, Fields::Custom(Entities::Shapes::ReadOwnedCount,
                                                                  Entities::Shapes::WriteOwnedCount))),
        Fields::When(Entities::Shapes::IsGrid, Fields::Custom(Entities::Shapes::ReadGrid, Entities::Shapes::WriteGrid)),
        Fields::Link(&Insert::Block, 5),
        Fields::Until(Version::R2000, Fields::When(Entities::Shapes::HasAttributes, Fields::Link(&Insert::First, 4),
                                                   Fields::Link(&Insert::Last, 4))),
        Fields::When(
            Entities::Shapes::HasAttributes,
            Fields::Since(Version::R2004, Fields::Custom(Entities::Shapes::ReadOwned, Entities::Shapes::WriteOwned)),
            Fields::Link(&Insert::SeqEnd, 3)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_INSERT_HPP */
