/***********************************************************************************************************************
 *  @file      source/releases/table/content.hpp
 *  @brief     A table as AutoCAD holds it once read, whatever the release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_TABLE_CONTENT_HPP
#define DWGIO_RELEASES_TABLE_CONTENT_HPP

#include "encoding/encoding.hpp"

#include "dwgio/entities/table.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/objects/datatable.hpp"
#include "dwgio/objects/tablecontent.hpp"
#include "dwgio/objects/tablegeometry.hpp"
#include "dwgio/objects/tablestyles/value.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <string>
#include <vector>

/**
 *  The content of its TABLECONTENT, each cell with the extents of its TABLEGEOMETRY, which a save writes back as the
 *  table's content from R2010 and as its grid before.
 */
namespace DwgIO::Releases::Table
{
struct Held
{
  const Objects::TableContents::Data *Content{}; /**< The table's TABLECONTENT. */
  const Objects::TableGeometry *Geometry{};
  Version From{};
};

struct Span
{
  Version From{};
  Version To{};
};

/**
 *  The checksum AutoCAD keeps of a cell up to R2007 in its content's ACAD_ROUNDTRIP_2008_CELL_CHECKSUM: each UTF-16
 *  unit of its text times its place, counted from 1, summed; 0 for a cell with no text.
 */
[[nodiscard]] auto Checksum(const std::string &text) -> double;

/**
 *  As R2007 keeps it: its own, else the string its UTF-16 bytes hold.
 */
[[nodiscard]] auto TextOf(const Objects::TableStyles::Value &value) -> std::string;

/**
 *  Across R2007 a string's bytes move between the code page with its NUL and UTF-16 with its NUL; the flags, unit,
 *  format and text R2007 added are dropped when downgrading and left empty when upgrading, as AutoCAD's save does.
 */
void Recode(Objects::TableContents::Data &content, Version to, const Encoding::Scheme &scheme);

/**
 *  The content AutoCAD holds of a table it read: its TABLECONTENT's, each cell given the extents of its TABLEGEOMETRY;
 *  read from a grid, a cell the grid holds otherwise made as AutoCAD makes it.
 */
[[nodiscard]] auto ReadContent(const Entities::AcadTable &table, const Held &held, const Encoding::Scheme &scheme)
    -> Objects::TableContents::Data;

/**
 *  The table in the form of R2010 and later, its other fields as AutoCAD's save writes them.
 */
void ToContentForm(Entities::AcadTable &table, Objects::TableContents::Data content, Span span);

/**
 *  The table in the form up to R2007, its grid made from the content given: from R2007 each cell's value, before its
 *  text.
 */
void ToGridForm(Entities::AcadTable &table, const Objects::TableContents::Data &content, Span span);

/**
 *  The content AutoCAD makes of a table's grid where it has no TABLECONTENT to read: the widths, heights and merged
 *  ranges of the grid, a cell's value from R2007 and its text before, the first row the title and the second the
 *  header.
 */
[[nodiscard]] auto MakeContent(const Entities::AcadTable &table, Version from) -> Objects::TableContents::Data;

/**
 *  The content of the TABLECONTENT AutoCAD writes of a table holding the content given: each cell's checksum among its
 *  custom data, the cells a merged range covers emptied, the extents left out, the table style named.
 */
[[nodiscard]] auto MakeStoredContent(const Objects::TableContents::Data &content, Handle style)
    -> Objects::TableContents::Data;

/**
 *  The TABLEGEOMETRY AutoCAD writes of a table holding the content given: each cell's extents.
 */
[[nodiscard]] auto MakeGeometry(const Objects::TableContents::Data &content) -> Objects::TableGeometry;

/**
 *  The cells of a table's DATATABLE, which AutoCAD writes below R2007: per cell, row by row, its checksum, two longs of
 *  0, and its value as R2007 keeps it, which an XRECORD of the groups returned holds.
 */
[[nodiscard]] auto MakeDataCells(const Objects::TableContents::Data &content) -> std::vector<std::vector<Tag>>;
} // namespace DwgIO::Releases::Table

#endif /* DWGIO_RELEASES_TABLE_CONTENT_HPP */
