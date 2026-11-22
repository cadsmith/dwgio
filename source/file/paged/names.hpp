/***********************************************************************************************************************
 *  @file      source/file/paged/names.hpp
 *  @brief     The sections AutoCAD writes in a paged file, and their names.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_PAGED_NAMES_HPP
#define DWGIO_FILE_PAGED_NAMES_HPP

#include "dwgio/version.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace DwgIO::File::Paged
{
/**
 *  The size of a compressed section's pages, decompressed.
 */
constexpr std::uint32_t UsualPage = 0x7400;

/**
 *  A section AutoCAD writes: its name in the file, the name dwgio gives its block, its id in the section map, whether
 *  it is compressed and the size of its pages.
 */
struct Known
{
  std::string_view Acdb;
  std::string_view Name;
  std::int32_t Id;
  bool Compressed;
  std::uint32_t PageSize;
};

/**
 *  Null for a section AutoCAD does not write.
 */
[[nodiscard]] auto Find(std::string_view acdb) -> const Known *;

/**
 *  The page size AutoCAD writes a known section with in a release: its own, but 0x100 for the summary from R2010.
 */
[[nodiscard]] auto PageOf(const Known &known, Version release) -> std::uint32_t;

/**
 *  dwgio's name for a section: header, classes, objects, map, preview, auxiliary and measurement for the seven the
 *  locator container also has, else the AcDb name itself.
 */
[[nodiscard]] auto SectionNameOf(std::string_view acdb) -> std::string;

/**
 *  The AcDb name of a section dwgio names, the name itself when no known section has it.
 */
[[nodiscard]] auto AcdbOf(std::string_view name) -> std::string;
} // namespace DwgIO::File::Paged

#endif /* DWGIO_FILE_PAGED_NAMES_HPP */
