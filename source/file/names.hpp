/***********************************************************************************************************************
 *  @file      source/file/names.hpp
 *  @brief     The names of a file's blocks and sections.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_NAMES_HPP
#define DWGIO_FILE_NAMES_HPP

#include <string_view>

/**
 *  dwgio names the blocks a locator file has and the sections of a paged file that hold the same by its own word, any
 *  other section by its AcDb name.
 */
namespace DwgIO::File::Names
{
constexpr std::string_view Header = "header";
constexpr std::string_view Auxiliary = "auxiliary";
constexpr std::string_view Classes = "classes";
constexpr std::string_view Map = "map";
constexpr std::string_view Measurement = "measurement";
constexpr std::string_view Objects = "objects";
constexpr std::string_view Preview = "preview";
constexpr std::string_view Pad = "pad"; /**< The four bytes after the classes of a locator file. */
constexpr std::string_view Second = "second";
constexpr std::string_view Unknown = "unknown"; /**< Bytes of a locator file no block holds. */

constexpr std::string_view ObjFreeSpace = "AcDb:ObjFreeSpace";
constexpr std::string_view RevHistory = "AcDb:RevHistory";
constexpr std::string_view SummaryInfo = "AcDb:SummaryInfo";
constexpr std::string_view AppInfo = "AcDb:AppInfo";
constexpr std::string_view AppInfoHistory = "AcDb:AppInfoHistory";
constexpr std::string_view DataStore = "AcDb:AcDsPrototype_1b";
constexpr std::string_view Security = "AcDb:Security";
constexpr std::string_view FileDepList = "AcDb:FileDepList";
constexpr std::string_view VbaProject = "AcDb:VBAProject";
constexpr std::string_view Signature = "AcDb:Signature";

/**
 *  The AcDb names of the sections dwgio names by its own word.
 */
constexpr std::string_view AcdbHeader = "AcDb:Header";
constexpr std::string_view AcdbAuxHeader = "AcDb:AuxHeader";
constexpr std::string_view AcdbClasses = "AcDb:Classes";
constexpr std::string_view AcdbHandles = "AcDb:Handles";
constexpr std::string_view AcdbTemplate = "AcDb:Template";
constexpr std::string_view AcdbObjects = "AcDb:AcDbObjects";
constexpr std::string_view AcdbPreview = "AcDb:Preview";
} // namespace DwgIO::File::Names

#endif /* DWGIO_FILE_NAMES_HPP */
