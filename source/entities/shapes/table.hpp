/***********************************************************************************************************************
 *  @file      source/entities/shapes/table.hpp
 *  @brief     How an ACAD_TABLE lies in the file: its grid up to R2007, its content from R2010.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_TABLE_HPP
#define DWGIO_ENTITIES_SHAPES_TABLE_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/table/cellstyle.hpp"
#include "objects/shapes/table/content.hpp"
#include "objects/shapes/table/value.hpp"

#include "dwgio/entities/table.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Entities::Shapes
{
namespace Tabled = Objects::Shapes::Table;

/**
 *  From R2000 packed as an insert's: two bits, 3 for unit scale, 1 for X at one with Y and Z as deltas against it, 2
 *  for one double for the three, 0 for X as a double with Y and Z as deltas against it.
 */
inline auto ReadTableScale(AcadTable &table, const Fields::Input &in) -> bool
{
  const int form = in.In->BB();
  Vector &scale = table.Scale;
  scale = Vector{.X = 1, .Y = 1, .Z = 1};
  if (form == 0 || form == 2)
  {
    scale.X = in.In->RD();
  }
  scale.Y = form == 2 ? scale.X : scale.Y;
  scale.Z = form == 2 ? scale.X : scale.Z;
  if (form == 0 || form == 1)
  {
    scale.Y = in.In->DD(scale.X);
    scale.Z = in.In->DD(scale.X);
  }
  return !in.In->HasFailed();
}

inline auto WriteTableScale(const AcadTable &table, const Fields::Output &to) -> bool
{
  const Vector &scale = table.Scale;
  const int unitX = static_cast<int>(scale.X == 1);
  const int even = static_cast<int>(scale.Y == scale.X) * static_cast<int>(scale.Z == scale.X);
  constexpr std::array<int, 4> Forms{0, 2, 1, 3}; // By X at one, then the three alike.
  const int form = Forms.at((2 * static_cast<std::size_t>(unitX)) + static_cast<std::size_t>(even));
  to.To->BB(form);
  if (form == 0 || form == 2)
  {
    to.To->RD(scale.X);
  }
  if (form == 0 || form == 1)
  {
    to.To->DD(scale.Y, scale.X);
    to.To->DD(scale.Z, scale.X);
  }
  return true;
}

/**
 *  Up to R2007. False where the cell holds what the library does not read.
 */
inline auto ReadTableCell(AcadTables::Cell &cell, const Fields::Input &in) -> bool
{
  cell.Type = in.In->BS();
  cell.Edges = in.In->RC();
  cell.Merged = in.In->B();
  cell.Autofit = in.In->B();
  cell.Across = in.In->BL();
  cell.Down = in.In->BL();
  cell.Rotation = in.In->BD();
  if (cell.Type != 1)
  {
    return false;
  }
  if (in.With.Release < Version::R2007)
  {
    cell.Text = Fields::TextCode::Read(in);
  }
  cell.Overridden = in.In->B();
  if (cell.Overridden)
  {
    cell.Overrides = in.In->BL();
    cell.Virtual = in.In->RC();
  }
  if ((static_cast<unsigned>(cell.Overrides) & ~static_cast<unsigned>(AcadTables::Texted)) != 0)
  {
    return false;
  }
  if (in.With.Release >= Version::R2007)
  {
    cell.Unknown1 = in.In->BL();
    if (!Tabled::ReadValue(cell.Value, in))
    {
      return false;
    }
  }
  return Fields::ReadLink(cell.Content, Tabled::HardPointer, in);
}

inline void WriteTableCell(const AcadTables::Cell &cell, const Fields::Output &to)
{
  to.To->BS(cell.Type);
  to.To->RC(static_cast<std::uint8_t>(cell.Edges));
  to.To->B(cell.Merged);
  to.To->B(cell.Autofit);
  to.To->BL(cell.Across);
  to.To->BL(cell.Down);
  to.To->BD(cell.Rotation);
  if (to.With.Release < Version::R2007)
  {
    Fields::TextCode::Write(cell.Text, to);
  }
  to.To->B(cell.Overridden);
  if (cell.Overridden)
  {
    to.To->BL(cell.Overrides);
    to.To->RC(static_cast<std::uint8_t>(cell.Virtual));
  }
  if (to.With.Release >= Version::R2007)
  {
    to.To->BL(cell.Unknown1);
    Tabled::WriteValue(cell.Value, to);
  }
  static_cast<void>(Fields::WriteLink(cell.Content, Tabled::HardPointer, to));
}

inline void ReadTableDoubles(std::vector<double> &values, const Fields::Input &in)
{
  for (double &value : values)
  {
    value = in.In->BD();
  }
}

/**
 *  Up to R2007: the column and row counts, the widths and heights, the cells row by row, then four bits that say the
 *  table overrides its style or its borders, which stop the read when set.
 */
inline auto ReadTableGrid(AcadTable &table, const Fields::Input &in) -> bool
{
  const std::int32_t columns = in.In->BL();
  const std::int32_t rows = in.In->BL();
  const std::size_t room = Fields::Room(in);
  if (in.In->HasFailed() || columns < 0 || rows < 0 || std::cmp_greater(columns, room) ||
      static_cast<std::size_t>(rows) > room / std::max<std::size_t>(static_cast<std::size_t>(columns), 1))
  {
    in.In->Seek(in.In->Size() + 1);
    return false;
  }
  table.Widths.resize(static_cast<std::size_t>(columns));
  table.Heights.resize(static_cast<std::size_t>(rows));
  ReadTableDoubles(table.Widths, in);
  ReadTableDoubles(table.Heights, in);
  table.Cells.resize(table.Widths.size() * table.Heights.size());
  if (!std::ranges::all_of(table.Cells, [&in](AcadTables::Cell &cell) -> bool { return ReadTableCell(cell, in); }))
  {
    return false;
  }
  for (bool *bit : {&table.Overridden, &table.Colored, &table.Weighted, &table.Shown})
  {
    *bit = in.In->B();
    if (*bit)
    {
      return false;
    }
  }
  return !in.In->HasFailed();
}

/**
 *  A cell missing from the widths and heights is written as a new one's.
 */
inline auto WriteTableGrid(const AcadTable &table, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(table.Widths.size()));
  to.To->BL(static_cast<std::int32_t>(table.Heights.size()));
  for (const double width : table.Widths)
  {
    to.To->BD(width);
  }
  for (const double height : table.Heights)
  {
    to.To->BD(height);
  }
  const std::size_t count = table.Widths.size() * table.Heights.size();
  for (std::size_t at = 0; at < count; ++at)
  {
    WriteTableCell(at < table.Cells.size() ? table.Cells[at] : AcadTables::Cell{}, to);
  }
  for (const bool bit : {table.Overridden, table.Colored, table.Weighted, table.Shown})
  {
    to.To->B(bit);
  }
  return true;
}

/**
 *  A bit in R2010, a long from R2013.
 */
inline auto ReadTableMore(AcadTable &table, const Fields::Input &in) -> bool
{
  table.Unknown3 = in.With.Release >= Version::R2013 ? in.In->BL() : static_cast<std::int32_t>(in.In->B());
  return !in.In->HasFailed();
}

inline auto WriteTableMore(const AcadTable &table, const Fields::Output &to) -> bool
{
  if (to.With.Release >= Version::R2013)
  {
    to.To->BL(table.Unknown3);
  }
  else
  {
    to.To->B(table.Unknown3 != 0);
  }
  return true;
}

inline auto ReadTableContent(AcadTable &table, const Fields::Input &in) -> bool
{
  return Tabled::ReadTableContent(table.Content, in);
}

inline auto WriteTableContent(const AcadTable &table, const Fields::Output &to) -> bool
{
  Tabled::WriteTableContent(table.Content, Tabled::SoftPointer, to);
  return true;
}

/**
 *  Break data stops the read.
 */
inline auto ReadTableParts(AcadTable &table, const Fields::Input &in) -> bool
{
  table.Broken = in.In->BL();
  if (table.Broken != 0)
  {
    return false;
  }
  Tabled::ReadSize(table.Parts, in);
  for (AcadTables::Part &part : table.Parts)
  {
    part.Position = in.In->Point3();
    part.Start = in.In->BL();
    part.End = in.In->BL();
  }
  return !in.In->HasFailed();
}

inline auto WriteTableParts(const AcadTable &table, const Fields::Output &to) -> bool
{
  to.To->BL(table.Broken);
  to.To->BL(static_cast<std::int32_t>(table.Parts.size()));
  for (const AcadTables::Part &part : table.Parts)
  {
    to.To->Point3(part.Position);
    to.To->BL(part.Start);
    to.To->BL(part.End);
  }
  return true;
}

inline auto IsReadableTable(const AcadTable &table) -> bool
{
  const auto cell = [](const AcadTables::Cell &one) -> bool
  {
    return one.Type == 1 && (static_cast<unsigned>(one.Overrides) & ~static_cast<unsigned>(AcadTables::Texted)) == 0 &&
           Tabled::IsKnownValue(one.Value);
  };
  return !table.Follows && !table.Overridden && !table.Colored && !table.Weighted && !table.Shown &&
         table.Broken == 0 && std::ranges::all_of(table.Cells, cell) && Tabled::IsReadableContent(table.Content);
}
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::AcadTable>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "ACAD_TABLE";

  static auto IsReadable(const Entities::AcadTable &table) -> bool { return Entities::Shapes::IsReadableTable(table); }

  static auto Layout()
  {
    using Entities::AcadTable;
    namespace Shapes = Entities::Shapes;
    return Fields::Layout(
        Fields::Point3(&AcadTable::Position), Fields::Until(Version::R14, Fields::Point3(&AcadTable::Scale)),
        Fields::Since(Version::R2000, Fields::Custom(Shapes::ReadTableScale, Shapes::WriteTableScale)),
        Fields::Double(&AcadTable::Rotation), Fields::Point3(&AcadTable::Extrusion), Fields::Bit(&AcadTable::Follows),
        Fields::Until(Version::R2007, Fields::Short(&AcadTable::Flags), Fields::Point3(&AcadTable::Direction),
                      Fields::Link(&AcadTable::Block, Shapes::Tabled::HardPointer),
                      Fields::Link(&AcadTable::Style, Shapes::Tabled::HardPointer),
                      Fields::Custom(Shapes::ReadTableGrid, Shapes::WriteTableGrid)),
        Fields::Since(Version::R2010, Fields::Byte(&AcadTable::Unknown1),
                      Fields::Link(&AcadTable::Block, Shapes::Tabled::HardPointer), Fields::Long(&AcadTable::Unknown2),
                      Fields::Custom(Shapes::ReadTableMore, Shapes::WriteTableMore),
                      Fields::Custom(Shapes::ReadTableContent, Shapes::WriteTableContent),
                      Fields::Short(&AcadTable::Flags), Fields::Point3(&AcadTable::Direction),
                      Fields::Custom(Shapes::ReadTableParts, Shapes::WriteTableParts),
                      Fields::Link(&AcadTable::Style, Shapes::Tabled::HardPointer)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_TABLE_HPP */
