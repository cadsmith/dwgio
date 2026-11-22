/***********************************************************************************************************************
 *  @file      source/releases/common.hpp
 *  @brief     An entity's common data as another release holds it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_COMMON_HPP
#define DWGIO_RELEASES_COMMON_HPP

#include "releases/passage.hpp"

#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"

namespace DwgIO::Releases
{
/**
 *  An entity's common data across R2000 and R2004: across R2000 the linetype, lineweight and plot style raised or
 *  lowered; across R2004 the links cleared when upgrading, and when downgrading the links taken from the owner's list
 *  and the color faded.
 */
void ConvertCommon(Common &common, Base &base, const Passage &passage);

/**
 *  A color as R2000 holds it: the index alone, a true color or a transparency dropped and noted.
 */
void FadeColor(Color &color, const Passage &passage, Handle self);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_COMMON_HPP */
