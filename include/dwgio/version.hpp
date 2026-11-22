/***********************************************************************************************************************
 *  @file      dwgio/version.hpp
 *  @brief     The DWG releases from R14 to R2018 and their codes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_VERSION_HPP
#define DWGIO_VERSION_HPP

#include "dwgio/api.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace DwgIO
{
/**
 *  A DWG release, oldest first so that releases compare by age.
 */
enum class Version : std::uint8_t
{
  R14,   /**< AutoCAD Release 14, AC1014.                                                            */
  R2000, /**< AutoCAD 2000 to 2002, AC1015.                                                          */
  R2004, /**< AutoCAD 2004 to 2006, AC1018: the first with compressed, encrypted sections.           */
  R2007, /**< AutoCAD 2007 to 2009, AC1021: the first with Reed-Solomon coded pages and UTF-16 text. */
  R2010, /**< AutoCAD 2010 to 2012, AC1024.                                                          */
  R2013, /**< AutoCAD 2013 to 2017, AC1027.                                                          */
  R2018, /**< AutoCAD 2018 and later, AC1032.                                                        */
};

/**
 *  The code a file opens with, such as "AC1015" for R2000.
 */
[[nodiscard]] DWGIO_API auto AcadverOf(Version release) -> std::string_view;

/**
 *  Such as "R2000".
 */
[[nodiscard]] DWGIO_API auto NameOf(Version release) -> std::string_view;

/**
 *  The maintenance version AutoCAD 2026 stamps a file of the release with: 10 in R14, 15 in R2000, 104 in R2004, 50 in
 *  R2007, 226 in R2010, 125 in R2013 and 0 in R2018.
 */
[[nodiscard]] DWGIO_API auto MaintenanceOf(Version release) -> int;

/**
 *  The release a code such as "AC1015" stands for.
 *
 *  @return Empty unless the code is exactly one of the seven, since a file's code is never padded or cased otherwise.
 */
[[nodiscard]] DWGIO_API auto ReleaseOf(std::string_view code) -> std::optional<Version>;
} // namespace DwgIO

#endif /* DWGIO_VERSION_HPP */
