/***********************************************************************************************************************
 *  @file      source/file/sections/summary.hpp
 *  @brief     The summary info section, chapter 13.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_SECTIONS_SUMMARY_HPP
#define DWGIO_FILE_SECTIONS_SUMMARY_HPP

#include "encoding/encoding.hpp"

#include "dwgio/summary.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

/**
 *  AcDb:SummaryInfo: eight texts, each a short of its characters and the NUL after it, in the code page below R2007 and
 *  in UTF-16 from R2007; the editing time and the two dates, each a long of days and one of milliseconds; a short count
 *  of the properties, each a name and a value; two zero longs.
 */
namespace DwgIO::File::Sections
{
/**
 *  The summary the section holds, read as the release keeps it; empty when its bytes are not one that writes back to
 *  the same bytes.
 */
[[nodiscard]] auto ReadSummary(std::span<const std::byte> bytes, Version release, const Encoding::Scheme &text)
    -> std::optional<SummaryInfo>;

/**
 *  The section of the summary as the release keeps it.
 */
[[nodiscard]] auto WriteSummary(const SummaryInfo &summary, Version release, const Encoding::Scheme &text)
    -> std::vector<std::byte>;
} // namespace DwgIO::File::Sections

#endif /* DWGIO_FILE_SECTIONS_SUMMARY_HPP */
