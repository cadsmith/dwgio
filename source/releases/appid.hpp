/***********************************************************************************************************************
 *  @file      source/releases/appid.hpp
 *  @brief     The applications a save to a release registers for the extended data it writes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_APPID_HPP
#define DWGIO_RELEASES_APPID_HPP

#include "releases/remake.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/version.hpp"

#include <optional>

namespace DwgIO::Releases
{
/**
 *  The drawing a save to a release writes when it needs an application it does not register: downgraded below R2010, a
 *  multileader or its style keeps its version in ACAD_MLEADERVER extended data, and a drawing of R2010 or later may
 *  have no such application.
 *
 *  AutoCAD refuses the style without its version, and its own save registers the application under a new handle.
 *  Downgraded below R2013, an ACIS entity keeps its GUID in ACAD_OBJECT_CHANGE_GUID extended data, which AutoCAD
 *  registers the same way. The drawing returned is the one given with each record added under the handle seed. Empty
 *  when nothing is missing.
 */
void RegisterApplications(Remake &remade, Version to);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_APPID_HPP */
