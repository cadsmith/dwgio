/***********************************************************************************************************************
 *  @file      source/releases/viewport.hpp
 *  @brief     A paper space viewport's view as R14 keeps it in extended data.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_VIEWPORT_HPP
#define DWGIO_RELEASES_VIEWPORT_HPP

#include "releases/passage.hpp"

#include "dwgio/entities/viewport.hpp"

namespace DwgIO::Releases
{
/**
 *  A viewport's view across R2000: R14 keeps it in ACAD's MVIEW extended data, made from the fields when downgrading
 *  and read into them, then dropped, when upgrading, as AutoCAD's R2000 files keep none.
 *
 *  AutoCAD refuses an R14 file whose paper space viewport lacks it. A clip entity R14 cannot keep is noted.
 */
void ConvertView(Entities::Viewport &viewport, const Passage &passage, const Crossings &crossing);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_VIEWPORT_HPP */
