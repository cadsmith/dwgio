/***********************************************************************************************************************
 *  @file      source/releases/styles.hpp
 *  @brief     The layers, linetypes and styles as another release holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_STYLES_HPP
#define DWGIO_RELEASES_STYLES_HPP

#include "releases/passage.hpp"

#include "dwgio/base.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/version.hpp"

namespace DwgIO::Releases
{
/**
 *  Where a multileader or its style keeps its version.
 */
struct VersionPlace
{
  DwgIO::Base *Base{};
  int *Version{};
};

/**
 *  A layer as R14 holds it, from R2000's: no lineweight, no plot style.
 */
void LowerLayer(Tables::Layer &layer, const Passage &passage);

/**
 *  A linetype's strings area across R2007: 256 bytes of NUL-ended text in the code page up to R2004, 512 bytes of
 *  UTF-16 from R2007 when a dash has text and none otherwise; each text dash's text laid out again from the start, its
 *  index pointing at it. A text the area cannot hold is left out and noted.
 */
void ConvertLType(Tables::LType &ltype, const Passage &passage, const Crossings &crossing);

/**
 *  A multiline style's linetypes across R2018, which names each by its record where the releases before give its index
 *  among the linetype table's entries.
 */
void ConvertMLineStyle(Objects::MLineStyle &style, const Passage &passage, const Crossings &crossing);

/**
 *  A multileader's or its style's version across R2010, which keeps it in the data where the releases before keep it in
 *  ACAD_MLEADERVER extended data: the record read into the version and dropped when upgrading, made from it when
 *  downgrading, noted when the drawing has no such application.
 */
void ConvertVersion(const VersionPlace &versioned, const Passage &passage, const Crossings &crossing);

/**
 *  A table style across R2010, which keeps cell styles where the releases before keep three cells; AutoCAD's save from
 *  below R2010 to R2018 sets a bit of its flags, and its save below R2018 clears it.
 *
 *  The suppressed title and header rows, which R2010 has no place for and AutoCAD's save drops, are noted. A table and
 *  the objects it and its style own are made over before (releases/tabled.cpp).
 */
void ConvertTableStyle(Objects::TableStyle &style, const Passage &passage, const Crossings &crossing);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_STYLES_HPP */
