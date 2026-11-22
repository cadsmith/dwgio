/***********************************************************************************************************************
 *  @file      source/releases/header.hpp
 *  @brief     The header as another release holds it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_HEADER_HPP
#define DWGIO_RELEASES_HEADER_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <vector>

namespace DwgIO::Releases
{
/**
 *  A header as another release holds it, and what that release could not keep.
 */
struct HeaderTranslation
{
  DwgIO::Header Header;
  std::vector<Issue> Issues;
};

/**
 *  The dimension arrow blocks go by name before R2000 and by block record handle from it, a name with no record noted;
 *  what only the release keeps takes the values of Defaults, the R2000 dictionaries the root's entries; its pointers of
 *  R2007 and later to an object the save leaves out are cleared and noted. A header of the release asked for comes back
 *  as it is.
 */
[[nodiscard]] auto ConvertHeader(const DwgIO::Header &header, const Drawing &drawing, Version to) -> HeaderTranslation;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_HEADER_HPP */
