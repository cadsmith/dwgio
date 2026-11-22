/***********************************************************************************************************************
 *  @file      source/releases/table/convert.hpp
 *  @brief     The tables a save writes in the form of each release, and the records they keep.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_TABLE_CONVERT_HPP
#define DWGIO_RELEASES_TABLE_CONVERT_HPP

#include "releases/remake.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/version.hpp"

#include <optional>

namespace DwgIO::Releases::Table
{
/**
 *  The drawing a save to another release writes when it holds tables, as AutoCAD's own save does.
 *
 *  Each table is read as AutoCAD reads it, the content of its TABLECONTENT with the extents of its TABLEGEOMETRY, both
 *  kept under ACAD_XREC_ROUNDTRIP in its extension dictionary in every release, and written in the release's form: from
 *  R2010 as its content, before as its grid. Below R2010 a table style keeps its cell styles in a CELLSTYLEMAP, below
 *  R2007 a table its values in a DATATABLE and a style its cells' data types in a record, all made when downgrading,
 *  under the handle seed, and taken away when upgrading. A content's values change form across R2007. Empty when the
 *  drawing holds no table, table style or content, or the release is its own.
 */
void ConvertTables(Remake &remade, Version to);
} // namespace DwgIO::Releases::Table

#endif /* DWGIO_RELEASES_TABLE_CONVERT_HPP */
