/***********************************************************************************************************************
 *  @file      source/releases/table/style.cpp
 *  @brief     The tables and table styles translated across R2010.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/table/style.hpp"

#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/tablestyles/cellstyle.hpp"
#include "dwgio/objects/tablestyles/style.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Releases::Table
{
namespace
{
using Objects::TableStyles::Border;
using Objects::TableStyles::CellStyle;
using Objects::TableStyles::Edge;
using Objects::TableStyles::NamedCellStyle;
using Objects::TableStyles::RowStyle;

/**
 *  What AutoCAD's R2010 files hold where the cells say nothing: the table's text height, the spacing between contents,
 *  the spacing of a double border, the merge bits of the title row and of an upward table.
 */
constexpr double TableHeight = 0.18;
constexpr double Spacing = 4.5;
constexpr double Doubled = 1.125;
constexpr int MergedRow = 0x8000;
constexpr unsigned Upward = 0x10000U;
constexpr int TableKind = 5;

/**
 *  A row style's number, class, name and the index of its cell up to R2007, which lists data, title, header.
 */
struct Row
{
  int Id;
  int Class;
  std::string_view Name;
  std::size_t Cell;
};

constexpr std::array<Row, 3> Rows{{{.Id = 1, .Class = 1, .Name = "_TITLE", .Cell = 1},
                                   {.Id = 2, .Class = 1, .Name = "_HEADER", .Cell = 2},
                                   {.Id = 3, .Class = 2, .Name = "_DATA", .Cell = 0}}};

/**
 *  A cell's borders by side, R2010's order: top, right, bottom, left, inside vertical, inside horizontal.
 */
auto Sides(const RowStyle &cell) -> std::array<const Border *, 6>
{
  return {&cell.Top, &cell.Right, &cell.Bottom, &cell.Left, &cell.Center, &cell.Middle};
}

auto Sides(RowStyle &cell) -> std::array<Border *, 6>
{
  return {&cell.Top, &cell.Right, &cell.Bottom, &cell.Left, &cell.Center, &cell.Middle};
}

auto MakeCellStyle(const RowStyle &cell, const Objects::TableStyle &style, Handle byblock) -> CellStyle
{
  CellStyle made{.Type = TableKind, .Data = 1};
  made.Background = cell.Filled ? cell.Fill : CellStyle{}.Background;
  made.Content.Type = cell.Type;
  made.Content.Unit = cell.Unit;
  made.Content.Format = cell.Format;
  made.Content.Alignment = cell.Alignment;
  made.Content.Color = cell.Color;
  made.Content.Style = cell.Style;
  made.Content.Height = cell.Height;
  made.Margined = 1;
  made.Margins = {.Top = style.Vertical,
                  .Left = style.Horizontal,
                  .Bottom = style.Vertical,
                  .Right = style.Horizontal,
                  .HorizontalSpacing = Spacing,
                  .VerticalSpacing = Spacing};
  int side = 1;
  for (const Border *border : Sides(cell))
  {
    made.Borders.push_back(Edge{.Side = side,
                                .Color = border->Color,
                                .Lineweight = border->Lineweight,
                                .Linetype = byblock,
                                .Invisible = !border->Visible,
                                .Spacing = Doubled});
    side *= 2;
  }
  return made;
}

auto MakeRowStyle(const CellStyle &style) -> RowStyle
{
  RowStyle made;
  made.Style = style.Content.Style;
  made.Height = style.Content.Height;
  made.Alignment = style.Content.Alignment;
  made.Color = style.Content.Color;
  made.Filled = style.Background != CellStyle{}.Background;
  made.Fill = made.Filled ? style.Background : RowStyle{}.Fill;
  made.Type = style.Content.Type;
  made.Unit = style.Content.Unit;
  made.Format = style.Content.Format;
  const std::array<Border *, 6> sides = Sides(made);
  for (const Edge &edge : style.Borders)
  {
    for (std::size_t at = 0; at < sides.size(); ++at)
    {
      if (static_cast<unsigned>(edge.Side) == 1U << at)
      {
        *sides.at(at) = Border{.Lineweight = edge.Lineweight, .Visible = !edge.Invisible, .Color = edge.Color};
      }
    }
  }
  return made;
}
} // namespace

void Raise(Objects::TableStyle &style, Handle byblock)
{
  const Objects::TableStyle fresh{};
  style.Cells.resize(Rows.size());
  style.Overall =
      NamedCellStyle{.Id = 4, .Style = CellStyle{.Type = TableKind, .Data = 1}, .Class = 2, .Name = "Table"};
  CellStyle &overall = style.Overall.Style;
  overall.Merge = style.Flow != 0 ? static_cast<int>(Upward) : 0;
  overall.Content.Height = TableHeight;
  overall.Margined = 1;
  overall.Margins = {.Top = style.Vertical,
                     .Left = style.Horizontal,
                     .Bottom = style.Vertical,
                     .Right = style.Horizontal,
                     .HorizontalSpacing = TableHeight,
                     .VerticalSpacing = TableHeight};
  // Cell styles a save took in from the style's cell style map stand; else the three rows' are made of its cells.
  for (const Row &row : style.Styles.empty() ? std::span<const Row>{Rows} : std::span<const Row>{})
  {
    NamedCellStyle named{
        .Id = row.Id, .Style = MakeCellStyle(style.Cells[row.Cell], style, byblock), .Class = row.Class};
    named.Name = row.Name;
    named.Style.Merge = row.Id == 1 ? MergedRow : 0;
    style.Styles.push_back(std::move(named));
  }
  style.Flow = fresh.Flow;
  style.Horizontal = fresh.Horizontal;
  style.Vertical = fresh.Vertical;
  style.TitleSuppressed = fresh.TitleSuppressed;
  style.HeaderSuppressed = fresh.HeaderSuppressed;
  style.Cells.clear();
}

void Lower(Objects::TableStyle &style)
{
  const Objects::TableStyle fresh{};
  const CellStyle &overall = style.Overall.Style;
  style.Flow = (static_cast<unsigned>(overall.Merge) & Upward) != 0 ? 1 : 0;
  style.Horizontal = (static_cast<unsigned>(overall.Margined) & 1U) != 0 ? overall.Margins.Left : fresh.Horizontal;
  style.Vertical = (static_cast<unsigned>(overall.Margined) & 1U) != 0 ? overall.Margins.Top : fresh.Vertical;
  style.Cells.assign(Rows.size(), RowStyle{});
  for (const NamedCellStyle &named : style.Styles)
  {
    for (const Row &row : Rows)
    {
      if (named.Id == row.Id)
      {
        style.Cells[row.Cell] = MakeRowStyle(named.Style);
      }
    }
  }
  style.Revision = fresh.Revision;
  style.Next = fresh.Next;
  style.Unknown1 = fresh.Unknown1;
  style.Overall = fresh.Overall;
  style.Styles.clear();
}

} // namespace DwgIO::Releases::Table
