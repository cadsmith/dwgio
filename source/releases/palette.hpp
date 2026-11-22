/***********************************************************************************************************************
 *  @file      source/releases/palette.hpp
 *  @brief     AutoCAD's color palette, and the index nearest a true color.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_PALETTE_HPP
#define DWGIO_RELEASES_PALETTE_HPP

#include "dwgio/color.hpp"

#include <cstdint>

namespace DwgIO::Releases
{
/**
 *  The long R2004 and later write for a color of no method, where R2000 keeps the index 257.
 */
inline constexpr std::int32_t Unmethodical = -939524096;

/**
 *  The index R2000 keeps a color of no method by.
 */
inline constexpr int UnmethodicalIndex = 257;

/**
 *  The index of AutoCAD's palette nearest a true color, given as 0xRRGGBB, as AutoCAD picks it: the entry from 1 to 255
 *  at the least distance in red, green and blue, the first of those as near.
 */
[[nodiscard]] auto NearestIndexOf(std::int32_t rgb) -> int;

/**
 *  A color as R2000 holds it, as AutoCAD's save writes one of R2004 or later: no method as the index 257, a true color
 *  as its nearest index, its names dropped; any other as it is.
 */
[[nodiscard]] auto LowerColor(const Color &color) -> Color;

/**
 *  A color as R2004 holds it: R2000's index 257 as no method, any other as it is.
 */
[[nodiscard]] auto RaiseColor(const Color &color) -> Color;

/**
 *  The long of a color as R2004 and later store it, the method in the top byte: 0xC0 by layer, 0xC1 by block, 0xC3 an
 *  index in the low byte, else a true color as it is.
 */
[[nodiscard]] auto LongOf(const Color &color) -> std::int64_t;

/**
 *  The color of such a long; its names are not in it.
 */
[[nodiscard]] auto FromLong(std::int64_t value) -> Color;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_PALETTE_HPP */
