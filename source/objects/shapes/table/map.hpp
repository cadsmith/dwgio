/***********************************************************************************************************************
 *  @file      source/objects/shapes/table/map.hpp
 *  @brief     How a CELLSTYLEMAP lies in the file, chapter 20.4.102.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_TABLE_MAP_HPP
#define DWGIO_OBJECTS_SHAPES_TABLE_MAP_HPP

#include "fields/context.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/table/cellstyle.hpp"
#include "objects/shapes/table/content.hpp"
#include "objects/shapes/table/style.hpp"

#include "dwgio/objects/cellstylemap.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects::Shapes::Table
{
inline auto ReadMap(CellStyleMap &map, const Fields::Input &in) -> bool
{
  ReadSize(map.Styles, in);
  for (NamedCellStyle &named : map.Styles)
  {
    ReadCellStyle(named.Style, in);
    ReadNaming(named, in);
  }
  return !in.In->HasFailed();
}

inline auto WriteMap(const CellStyleMap &map, const Fields::Output &to) -> bool
{
  to.To->BL(static_cast<std::int32_t>(map.Styles.size()));
  for (const NamedCellStyle &named : map.Styles)
  {
    WriteCellStyle(named.Style, to);
    WriteNaming(named, to);
  }
  return true;
}
} // namespace DwgIO::Objects::Shapes::Table

namespace DwgIO::Objects
{
/**
 *  Chapter 20.4.102; each cell style as a table style from R2010 keeps its row styles.
 */
template <>
struct Shape<Objects::CellStyleMap>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "CELLSTYLEMAP";

  static auto Layout()
  {
    namespace Table = Shapes::Table;
    return Fields::Layout(Fields::Custom(Table::ReadMap, Table::WriteMap));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_TABLE_MAP_HPP */
