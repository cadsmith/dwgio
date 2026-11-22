/***********************************************************************************************************************
 *  @file      source/objects/shapes/table/style.hpp
 *  @brief     How a TABLESTYLE lies in the file, its row cells up to R2007.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_TABLE_STYLE_HPP
#define DWGIO_OBJECTS_SHAPES_TABLE_STYLE_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/table/cellstyle.hpp"

#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects::Shapes::Table
{
using Objects::TableStyles::Border;
using Objects::TableStyles::NamedCellStyle;
using Objects::TableStyles::RowStyle;

/**
 *  Up to R2007: the data, title and header rows' cells.
 */
constexpr std::size_t Rows = 3;

constexpr int HardOwner = 3;

inline void ReadBorder(Border &border, const Fields::Input &in)
{
  border.Lineweight = in.In->BS();
  border.Visible = in.In->B();
  border.Color = ReadTrue(in);
}

inline void WriteBorder(const Border &border, const Fields::Output &to)
{
  to.To->BS(border.Lineweight);
  to.To->B(border.Visible);
  WriteTrue(border.Color, to);
}

inline void ReadCell(RowStyle &cell, const Fields::Input &in)
{
  static_cast<void>(Fields::ReadLink(cell.Style, HardPointer, in));
  cell.Height = in.In->BD();
  cell.Alignment = in.In->BS();
  cell.Color = ReadTrue(in);
  cell.Fill = ReadTrue(in);
  cell.Filled = in.In->B();
  for (Border *border : {&cell.Top, &cell.Middle, &cell.Bottom, &cell.Left, &cell.Center, &cell.Right})
  {
    ReadBorder(*border, in);
  }
  if (in.With.Release >= Version::R2007)
  {
    cell.Type = in.In->BL();
    cell.Unit = in.In->BL();
    cell.Format = Fields::TextCode::Read(in);
  }
}

inline void WriteCell(const RowStyle &cell, const Fields::Output &to)
{
  static_cast<void>(Fields::WriteLink(cell.Style, HardPointer, to));
  to.To->BD(cell.Height);
  to.To->BS(cell.Alignment);
  WriteTrue(cell.Color, to);
  WriteTrue(cell.Fill, to);
  to.To->B(cell.Filled);
  for (const Border *border : {&cell.Top, &cell.Middle, &cell.Bottom, &cell.Left, &cell.Center, &cell.Right})
  {
    WriteBorder(*border, to);
  }
  if (to.With.Release >= Version::R2007)
  {
    to.To->BL(cell.Type);
    to.To->BL(cell.Unit);
    Fields::TextCode::Write(cell.Format, to);
  }
}

inline auto ReadCells(TableStyle &style, const Fields::Input &in) -> bool
{
  style.Cells.assign(Rows, RowStyle{});
  for (RowStyle &cell : style.Cells)
  {
    ReadCell(cell, in);
  }
  return !in.In->HasFailed();
}

/**
 *  A missing cell is written as a new style's, any beyond the third is left out.
 */
inline auto WriteCells(const TableStyle &style, const Fields::Output &to) -> bool
{
  for (std::size_t at = 0; at < Rows; ++at)
  {
    WriteCell(at < style.Cells.size() ? style.Cells[at] : RowStyle{}, to);
  }
  return true;
}

inline void ReadNaming(NamedCellStyle &named, const Fields::Input &in)
{
  named.Id = in.In->BL();
  named.Class = in.In->BL();
  named.Name = Fields::TextCode::Read(in);
}

inline void WriteNaming(const NamedCellStyle &named, const Fields::Output &to)
{
  to.To->BL(named.Id);
  to.To->BL(named.Class);
  Fields::TextCode::Write(named.Name, to);
}

inline auto ReadOverall(TableStyle &style, const Fields::Input &in) -> bool
{
  ReadCellStyle(style.Overall.Style, in);
  ReadNaming(style.Overall, in);
  return !in.In->HasFailed();
}

inline auto WriteOverall(const TableStyle &style, const Fields::Output &to) -> bool
{
  WriteCellStyle(style.Overall.Style, to);
  WriteNaming(style.Overall, to);
  return true;
}

inline auto ReadStyles(TableStyle &style, const Fields::Input &in) -> bool
{
  std::size_t count = 0;
  static_cast<void>(Fields::ReadHandleCount(in, count));
  style.Styles.resize(count);
  for (NamedCellStyle &named : style.Styles)
  {
    static_cast<void>(in.In->BL()); // The number again, before the style.
    ReadCellStyle(named.Style, in);
    ReadNaming(named, in);
  }
  return !in.In->HasFailed();
}

inline auto WriteStyles(const TableStyle &style, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(style.Styles.size()));
  for (const NamedCellStyle &named : style.Styles)
  {
    to.To->BL(named.Id);
    WriteCellStyle(named.Style, to);
    WriteNaming(named, to);
  }
  return true;
}
} // namespace DwgIO::Objects::Shapes::Table

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.101; up to R2007 the cells' colors are in the true color form even before R2004.
 */
template <>
struct Shape<Objects::TableStyle>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "TABLESTYLE";

  static auto Layout()
  {
    using Objects::TableStyle;
    namespace Table = Shapes::Table;
    return Fields::Layout(
        Fields::Since(Version::R2010, Fields::Byte(&TableStyle::Revision)), Fields::Text(&TableStyle::Description),
        Fields::Until(Version::R2007, Fields::Short(&TableStyle::Flow), Fields::Short(&TableStyle::Flags),
                      Fields::Double(&TableStyle::Horizontal), Fields::Double(&TableStyle::Vertical),
                      Fields::Bit(&TableStyle::TitleSuppressed), Fields::Bit(&TableStyle::HeaderSuppressed),
                      Fields::Custom(Table::ReadCells, Table::WriteCells)),
        Fields::Since(Version::R2010, Fields::Long(&TableStyle::Flags), Fields::Long(&TableStyle::Next),
                      Fields::Link(&TableStyle::Unknown1, Table::HardOwner),
                      Fields::Custom(Table::ReadOverall, Table::WriteOverall),
                      Fields::Custom(Table::ReadStyles, Table::WriteStyles)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_TABLE_STYLE_HPP */
