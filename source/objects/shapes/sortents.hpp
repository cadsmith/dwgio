/***********************************************************************************************************************
 *  @file      source/objects/shapes/sortents.hpp
 *  @brief     How a SORTENTSTABLE lies in the file, chapter 20.4.93.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_SORTENTS_HPP
#define DWGIO_OBJECTS_SHAPES_SORTENTS_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/sortents.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace DwgIO::Objects::Shapes
{
constexpr int SortSoftPointer = 4;

/**
 *  Each entry's key is a handle in the data, its entity a soft pointer in the handle stream after the block. A count
 *  beyond what is left is noted, the table kept empty.
 */
inline auto ReadSortents(SortEntsTable &table, const Fields::Input &in) -> bool
{
  const std::int32_t count = in.In->BL();
  if (in.In->HasFailed())
  {
    return false;
  }
  const std::size_t room = (in.In->Size() - in.In->Position()) / 8;
  if (count < 0 || std::cmp_greater(count, room))
  {
    Fields::Note(in, IssueCode::Damaged, std::format("sort table count {} out of range", count));
    return false;
  }
  table.Entries.resize(static_cast<std::size_t>(count));
  for (SortEntsTables::Entry &entry : table.Entries)
  {
    entry.Key = in.In->H().Value;
  }
  if (in.In->HasFailed() || !Fields::ReadLink(table.Block, SortSoftPointer, in))
  {
    return false;
  }
  for (SortEntsTables::Entry &entry : table.Entries)
  {
    if (!Fields::ReadLink(entry.Entity, SortSoftPointer, in))
    {
      return false;
    }
  }
  return true;
}

inline auto WriteSortents(const SortEntsTable &table, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(table.Entries.size()));
  for (const SortEntsTables::Entry &entry : table.Entries)
  {
    to.To->H(Bits::Reference{.Code = 0, .Value = entry.Key});
  }
  Fields::WriteLink(table.Block, SortSoftPointer, to);
  for (const SortEntsTables::Entry &entry : table.Entries)
  {
    Fields::WriteLink(entry.Entity, SortSoftPointer, to);
  }
  return true;
}
} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.93; stored under its class.
 */
template <>
struct Shape<Objects::SortEntsTable>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "SORTENTSTABLE";

  static auto Layout() { return Fields::Layout(Fields::Custom(Shapes::ReadSortents, Shapes::WriteSortents)); }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_SORTENTS_HPP */
