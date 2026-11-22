/***********************************************************************************************************************
 *  @file      source/acis/convert.hpp
 *  @brief     An ACIS body converted to the release a DWG release holds.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_CONVERT_HPP
#define DWGIO_ACIS_CONVERT_HPP

#include "dwgio/entities/geometry.hpp"
#include "dwgio/version.hpp"

#include <expected>
#include <string>

namespace DwgIO::Acis
{
/**
 *  The ACIS release AutoCAD writes bodies of a DWG release in.
 */
[[nodiscard]] auto ReleaseOf(Version release) -> int;

/**
 *  The body as AutoCAD writes it in that DWG release: SAT of ACIS 1.06 in R14 and 4.00 in R2000, SAB from R2004, of the
 *  release's ACIS. A body already of that ACIS comes back as it is, as does one of R2013's ACIS for R2018, which
 *  AutoCAD keeps.
 *
 *  Why not when the body holds a record or a value AutoCAD's files never hold, or an edge whose parameters cannot be
 *  found. A remade body is one AutoCAD remakes on reading, as it does a procedural surface's.
 */
[[nodiscard]] auto Convert(const Entities::Acis &acis, Version to, bool remade)
    -> std::expected<Entities::Acis, std::string>;
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_CONVERT_HPP */
