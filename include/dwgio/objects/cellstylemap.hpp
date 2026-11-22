/***********************************************************************************************************************
 *  @file      dwgio/objects/cellstylemap.hpp
 *  @brief     The CELLSTYLEMAP object: a table style's cell styles.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_CELLSTYLEMAP_HPP
#define DWGIO_OBJECTS_CELLSTYLEMAP_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/objects/tablestyles/style.hpp"

#include <vector>

namespace DwgIO::Objects
{
/**
 *  A CELLSTYLEMAP: a table style's cell styles.
 *
 *  Up to R2007 AutoCAD keeps it under ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP in the style's extension dictionary,
 *  the style itself holding three cells; from R2010 the style holds them all and there is none.
 */
struct DWGIO_API CellStyleMap
{
  DwgIO::Base Base{};
  /**
   *  The table's style, then the title, header and data styles, as AutoCAD writes them.
   */
  std::vector<TableStyles::NamedCellStyle> Styles{};

  auto operator==(const CellStyleMap &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_CELLSTYLEMAP_HPP */
