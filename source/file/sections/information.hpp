/***********************************************************************************************************************
 *  @file      source/file/sections/information.hpp
 *  @brief     The application information section, chapter 16.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_SECTIONS_INFORMATION_HPP
#define DWGIO_FILE_SECTIONS_INFORMATION_HPP

#include <cstddef>
#include <vector>

/**
 *  AcDb:AppInfo, which names the application that wrote the file.
 */
namespace DwgIO::File::Sections
{
/**
 *  The section as AutoCAD 2026 lays it out in every release from R2004, naming dwgio: a long of 3, the name
 *  AppInfoDataList, a long count of its entries, and each entry's GUID and text, every text a short of its characters
 *  and the NUL after it in UTF-16.
 *
 *  AutoCAD gives a file it saves an entry of its product and, only when the file it opened was last saved by AutoCAD,
 *  one of the comment that the file is a Trusted DWG; dwgio gives the product's alone.
 */
[[nodiscard]] auto WriteInformation() -> std::vector<std::byte>;
} // namespace DwgIO::File::Sections

#endif /* DWGIO_FILE_SECTIONS_INFORMATION_HPP */
