/***********************************************************************************************************************
 *  @file      source/objects/shapes/table/cellstyle.hpp
 *  @brief     The cell styles tables and table styles share from R2010, chapters 20.4.101.3 and 20.4.101.4.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_TABLE_CELLSTYLE_HPP
#define DWGIO_OBJECTS_SHAPES_TABLE_CELLSTYLE_HPP

#include "bits/reference.hpp"
#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/list.hpp"

#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/tablestyles/cellstyle.hpp"
#include "dwgio/objects/tablestyles/format.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace DwgIO::Objects::Shapes::Table
{
using Objects::TableStyles::CellStyle;
using Objects::TableStyles::ContentFormat;
using Objects::TableStyles::Edge;

constexpr int HardPointer = 5;

/**
 *  The true color form whatever the release, its names in the string stream from R2007.
 */
inline auto ReadTrue(const Fields::Input &in) -> Color
{
  return in.In->CMC(std::max(in.With.Release, Version::R2004), Fields::TextIn(in));
}

inline void WriteTrue(const Color &color, const Fields::Output &to)
{
  to.To->CMC(color, std::max(to.With.Release, Version::R2004), Fields::TextOut(to));
}

/**
 *  A cell's own content format names no text style with a null soft pointer.
 */
constexpr int SoftPointer = 4;

/**
 *  Any code: AutoCAD writes a null pointer as a hard or a soft one by where it stands.
 */
inline void ReadPointer(Handle &value, const Fields::Input &in)
{
  value = Bits::Resolve(in.Handles->H(), in.With.Record);
}

/**
 *  A hard pointer, or a null one with the code given.
 */
inline void WritePointer(Handle value, int null, const Fields::Output &to)
{
  int code = null;
  if (value)
  {
    code = HardPointer;
  }
  to.Handles->H(Bits::Reference{.Code = code, .Value = value});
}

inline void ReadContent(ContentFormat &content, const Fields::Input &in)
{
  content.Overrides = in.In->BL();
  content.Properties = in.In->BL();
  content.Type = in.In->BL();
  content.Unit = in.In->BL();
  content.Format = Fields::TextCode::Read(in);
  content.Rotation = in.In->BD();
  content.Scale = in.In->BD();
  content.Alignment = in.In->BL();
  content.Color = ReadTrue(in);
  ReadPointer(content.Style, in);
  content.Height = in.In->BD();
}

/**
 *  A null text style is written with the code given.
 */
inline void WriteContent(const ContentFormat &content, int null, const Fields::Output &to)
{
  to.To->BL(content.Overrides);
  to.To->BL(content.Properties);
  to.To->BL(content.Type);
  to.To->BL(content.Unit);
  Fields::TextCode::Write(content.Format, to);
  to.To->BD(content.Rotation);
  to.To->BD(content.Scale);
  to.To->BL(content.Alignment);
  WriteTrue(content.Color, to);
  WritePointer(content.Style, null, to);
  to.To->BD(content.Height);
}

/**
 *  The side, and only when it has one the rest.
 */
inline void ReadEdge(Edge &edge, const Fields::Input &in)
{
  edge.Side = in.In->BL();
  if (edge.Side == 0)
  {
    return;
  }
  edge.Overrides = in.In->BL();
  edge.Type = in.In->BL();
  edge.Color = ReadTrue(in);
  edge.Lineweight = in.In->BL();
  ReadPointer(edge.Linetype, in);
  edge.Invisible = in.In->BL() != 0;
  edge.Spacing = in.In->BD();
}

inline void WriteEdge(const Edge &edge, const Fields::Output &to)
{
  to.To->BL(edge.Side);
  if (edge.Side == 0)
  {
    return;
  }
  to.To->BL(edge.Overrides);
  to.To->BL(edge.Type);
  WriteTrue(edge.Color, to);
  to.To->BL(edge.Lineweight);
  WritePointer(edge.Linetype, HardPointer, to);
  to.To->BL(edge.Invisible ? 1 : 0);
  to.To->BD(edge.Spacing);
}

inline void ReadMargins(CellStyle &style, const Fields::Input &in)
{
  style.Margined = in.In->BS();
  if ((static_cast<unsigned>(style.Margined) & 1U) == 0)
  {
    return;
  }
  style.Margins.Top = in.In->BD();
  style.Margins.Left = in.In->BD();
  style.Margins.Bottom = in.In->BD();
  style.Margins.Right = in.In->BD();
  style.Margins.HorizontalSpacing = in.In->BD();
  style.Margins.VerticalSpacing = in.In->BD();
}

inline void WriteMargins(const CellStyle &style, const Fields::Output &to)
{
  to.To->BS(style.Margined);
  if ((static_cast<unsigned>(style.Margined) & 1U) == 0)
  {
    return;
  }
  to.To->BD(style.Margins.Top);
  to.To->BD(style.Margins.Left);
  to.To->BD(style.Margins.Bottom);
  to.To->BD(style.Margins.Right);
  to.To->BD(style.Margins.HorizontalSpacing);
  to.To->BD(style.Margins.VerticalSpacing);
}

/**
 *  The type and data bit, then the rest only when it has data. A border count beyond the data fails the reader.
 */
inline void ReadCellStyle(CellStyle &style, const Fields::Input &in)
{
  style.Type = in.In->BL();
  style.Data = in.In->BS();
  if (style.Data == 0)
  {
    return;
  }
  style.Overrides = in.In->BL();
  style.Merge = in.In->BL();
  style.Background = ReadTrue(in);
  style.Layout = in.In->BL();
  ReadContent(style.Content, in);
  ReadMargins(style, in);
  std::size_t count = 0;
  static_cast<void>(Fields::ReadHandleCount(in, count));
  style.Borders.resize(count);
  for (Edge &edge : style.Borders)
  {
    ReadEdge(edge, in);
  }
}

inline void WriteCellStyle(const CellStyle &style, const Fields::Output &to)
{
  to.To->BL(style.Type);
  to.To->BS(style.Data);
  if (style.Data == 0)
  {
    return;
  }
  to.To->BL(style.Overrides);
  to.To->BL(style.Merge);
  WriteTrue(style.Background, to);
  to.To->BL(style.Layout);
  WriteContent(style.Content, HardPointer, to);
  WriteMargins(style, to);
  to.To->BL(static_cast<std::int32_t>(style.Borders.size()));
  for (const Edge &edge : style.Borders)
  {
    WriteEdge(edge, to);
  }
}
} // namespace DwgIO::Objects::Shapes::Table

#endif /* DWGIO_OBJECTS_SHAPES_TABLE_CELLSTYLE_HPP */
