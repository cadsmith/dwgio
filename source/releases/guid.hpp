/***********************************************************************************************************************
 *  @file      source/releases/guid.hpp
 *  @brief     An ACIS entity's GUID kept in extended data below R2013.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_GUID_HPP
#define DWGIO_RELEASES_GUID_HPP

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/geometry.hpp"

#include <string_view>

namespace DwgIO::Releases
{
/**
 *  The application AutoCAD keeps an ACIS entity's GUID under below R2013.
 */
inline constexpr std::string_view GuidApplication = "ACAD_OBJECT_CHANGE_GUID";

/**
 *  A null GUID is not kept.
 */
[[nodiscard]] auto HasGuid(const Entities::Geometry &geometry) -> bool;

/**
 *  Below R2013, the GUID as AutoCAD keeps it, in extended data of its application: a short of 1, the GUID's long and
 *  two shorts, then its eight bytes as shorts.
 *
 *  Nothing when it is null or the drawing does not register the application.
 */
void KeepGuid(Base &base, const Entities::Geometry &geometry, const Drawing &drawing);

/**
 *  Upgrading to R2013, takes the GUID back from that extended data, which is dropped; true when there was one.
 */
[[nodiscard]] auto TakeGuid(Base &base, Entities::Geometry &geometry, const Drawing &drawing) -> bool;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_GUID_HPP */
