/***********************************************************************************************************************
 *  @file      source/releases/mtext.hpp
 *  @brief     What an MTEXT holds that an older release keeps in its extended data.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_MTEXT_HPP
#define DWGIO_RELEASES_MTEXT_HPP

#include "releases/remake.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/version.hpp"

#include <optional>

/**
 *  AutoCAD's save appends sections to the ACAD record, each a 1000 text opening and closing it and each value after a
 *  1070 of its DXF code:
 *  - below R2018, its columns: ACAD_MTEXT_COLUMN_INFO_BEGIN, 75 the type, 79 auto height, 76 the count, 78 reversed, 48
 *    the width, 49 the gutter, for dynamic columns of their own heights 50 and the count of heights each a 1040, and
 *    ACAD_MTEXT_COLUMN_INFO_END; the MTEXT's width is then a column's. When the text fills more than one column
 *    AutoCAD's save writes each further column's text in an MTEXT of its own, listed after it in
 *    ACAD_MTEXT_COLUMNS_BEGIN, a 1070 47, the count of columns, a 1005 for each further one, ACAD_MTEXT_COLUMNS_END;
 *  - below R2004, its line spacing: MTEXTBEGIN, 73 the style, 44 the factor, 74 a 0, MTEXTEND;
 *  - below R2004, its background when it has one: ACAD_MTEXT_BBRT, 91 the flags, 46 the margin, 64 the color's long, 65
 *    its name, 66 its book, 442 the transparency, ACAD_MTEXT_BERT;
 *  - below R2007, its defined height when it has one: ACAD_MTEXT_DEFINED_HEIGHT_BEGIN, 46 the height,
 *    ACAD_MTEXT_DEFINED_HEIGHT_END.
 *
 *  AutoCAD reads each back when upgrading, and takes it away but for the defined height, which stays in the record.
 */
namespace DwgIO::Releases
{
struct TextContext
{
  Version From{};
  Version To{};
  Handle Acad{}; /**< The ACAD application's record. */
};

/**
 *  An MTEXT as the release it goes to holds it: what its extended data keeps for the release it comes from read into
 *  its fields and the sections taken away, those the release it goes to keeps appended, their fields cleared. dwgio
 *  does not lay text out, so a text keeps the text of every column.
 *
 *  False, the MTEXT left as it was, when a section is to be made and acad is null.
 */
[[nodiscard]] auto ConvertMText(Entities::MText &text, const TextContext &texting) -> bool;

/**
 *  The drawing a save to R2018 writes when it upgrades from below with MTEXTs AutoCAD split into columns: the text of
 *  each further column joined to the first's, in order, and the further MTEXTs taken away, as AutoCAD reads them. Empty
 *  when nothing changes.
 */
void JoinColumns(Remake &remade, Version to);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_MTEXT_HPP */
