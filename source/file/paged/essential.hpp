/***********************************************************************************************************************
 *  @file      source/file/paged/essential.hpp
 *  @brief     The sections AutoCAD refuses a paged file without.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_PAGED_ESSENTIAL_HPP
#define DWGIO_FILE_PAGED_ESSENTIAL_HPP

#include "dwgio/header.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <vector>

/**
 *  The sections AutoCAD refuses a paged file without, made for a drawing that lacks them: the revision history, the
 *  object free space and the auxiliary header, each as AutoCAD 2026 writes it where the text is silent.
 */
namespace DwgIO::File::Paged
{
/**
 *  AcDb:RevHistory: four longs, 0, 0, 1, 0.
 */
[[nodiscard]] auto WriteRevHistory() -> std::vector<std::byte>;

/**
 *  AcDb:ObjFreeSpace: AutoCAD's constants around the header's update time, from R2010 in 64 bits with the count of
 *  objects.
 */
[[nodiscard]] auto WriteObjFreeSpace(const Header &header, Version release, std::size_t objects)
    -> std::vector<std::byte>;

/**
 *  AcDb:Template, the measurement block: no description and MEASUREMENT 1, the description counted from R2007 as
 *  AutoCAD writes it, one unit of zero.
 */
[[nodiscard]] auto WriteTemplate(Version release) -> std::vector<std::byte>;

/**
 *  AcDb:AuxHeader of chapter 27, also R2000's auxiliary header: the release's version code and the maintenance, one
 *  save, the header's creation and update times and its handle seed, the rest as the text has it; in R2018 a zero short
 *  after each of the three version and maintenance pairs, as AutoCAD writes them.
 */
[[nodiscard]] auto WriteAuxHeader(const Header &header, Version release, int maintenance) -> std::vector<std::byte>;
} // namespace DwgIO::File::Paged

#endif /* DWGIO_FILE_PAGED_ESSENTIAL_HPP */
