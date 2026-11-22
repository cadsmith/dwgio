/***********************************************************************************************************************
 *  @file      source/releases/dimstyle.hpp
 *  @brief     A dimension style, and the header's dimension variables, as another release holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_DIMSTYLE_HPP
#define DWGIO_RELEASES_DIMSTYLE_HPP

#include "releases/passage.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/dimstyle.hpp"

#include <string>
#include <string_view>

namespace DwgIO::Releases
{
/**
 *  DIMLUNIT and DIMFRAC, which R2000 split from R14's DIMUNIT: 1 scientific, 2 decimal, 3 engineering, 4 and 5
 *  architectural and fractional stacked, 6 and 7 the same not stacked, 8 as Windows sets it; else decimal.
 */
struct Units
{
  int Lunit;
  int Frac;
};

/**
 *  DIMATFIT and DIMTMOVE, which R2000 split from R14's DIMFIT: 0 to 3 what moves outside when text and arrows do not
 *  fit, 4 and 5 the best fit with the text on a leader or moved freely; else the best fit.
 */
struct Fit
{
  int Atfit;
  int Tmove;
};

/**
 *  The name a dimension arrow block goes by before R2000: its record's name without the leading underscore of AutoCAD's
 *  own arrows; empty for no block.
 */
[[nodiscard]] auto ArrowName(const Drawing &drawing, Handle block) -> std::string;

/**
 *  The block record a dimension arrow name stands for from R2000: the record of that name, or of the name under
 *  AutoCAD's underscore; null and noted for a name with no record, null for no name.
 */
[[nodiscard]] auto ArrowRecord(std::string_view name, const Passage &passage, Handle self) -> Handle;

/**
 *  DIMLUNIT and DIMFRAC from R14's DIMUNIT.
 */
[[nodiscard]] auto SplitUnits(int dimunit) -> Units;

/**
 *  R14's DIMUNIT from DIMLUNIT and DIMFRAC.
 */
[[nodiscard]] auto JoinUnits(Units units) -> int;

/**
 *  DIMATFIT and DIMTMOVE from R14's DIMFIT.
 */
[[nodiscard]] auto SplitFit(int dimfit) -> Fit;

/**
 *  R14's DIMFIT from DIMATFIT and DIMTMOVE.
 */
[[nodiscard]] auto JoinFit(Fit fit) -> int;

/**
 *  A dimension style across the releases: its arrows and units across R2000, its colors faded when downgrading to it.
 */
void ConvertDimStyle(Tables::DimStyle &kind, const Passage &passage, const Crossings &crossing);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_DIMSTYLE_HPP */
