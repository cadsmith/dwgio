/***********************************************************************************************************************
 *  @file      source/file/sections/dependencies.hpp
 *  @brief     The file dependencies section, chapter 17.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_SECTIONS_DEPENDENCIES_HPP
#define DWGIO_FILE_SECTIONS_DEPENDENCIES_HPP

#include "encoding/encoding.hpp"

#include "dwgio/dependencies.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

/**
 *  AcDb:FileDepList: a long count of the features, each a text; a long count of the files, each four texts, the
 *  feature's index, the time stamp and the size as longs, a short for the graphics and a long count of references.
 *
 *  Each text is a long count of its bytes and no NUL: the code page's below R2007, UTF-16 from R2007.
 */
namespace DwgIO::File::Sections
{
/**
 *  The file dependencies the section holds, read as the release keeps them; empty when its bytes are not a list that
 *  writes back to the same bytes.
 */
[[nodiscard]] auto ReadDependencies(std::span<const std::byte> bytes, Version release, const Encoding::Scheme &text)
    -> std::optional<FileDependencies>;

/**
 *  The section of the file dependencies as the release keeps it.
 */
[[nodiscard]] auto WriteDependencies(const FileDependencies &list, Version release, const Encoding::Scheme &text)
    -> std::vector<std::byte>;
} // namespace DwgIO::File::Sections

#endif /* DWGIO_FILE_SECTIONS_DEPENDENCIES_HPP */
