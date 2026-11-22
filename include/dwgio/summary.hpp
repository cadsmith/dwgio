/***********************************************************************************************************************
 *  @file      dwgio/summary.hpp
 *  @brief     The drawing properties AutoCAD's DWGPROPS shows.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_SUMMARY_HPP
#define DWGIO_SUMMARY_HPP

#include "dwgio/api.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO
{
struct DWGIO_API Property
{
  std::string Name{};  /**< AutoCAD requires one. */
  std::string Value{}; /**< May be empty.         */

  auto operator==(const Property &) const -> bool = default;
};

/**
 *  The drawing properties AutoCAD's DWGPROPS shows: from R2004 the AcDb:SummaryInfo section, in R14 and R2000 an
 *  XRECORD named DWGPROPS in the named objects dictionary.
 *
 *  A date is a Julian day and the milliseconds into it, as the header's TDCREATE.
 */
struct DWGIO_API SummaryInfo
{
  std::string Title{};
  std::string Subject{};
  std::string Author{};
  std::string Keywords{};
  std::string Comments{};
  std::string LastSavedBy{}; /**< AutoCAD sets it at each save. */
  std::string RevisionNumber{};
  std::string HyperlinkBase{}; /**< AutoCAD leaves it empty: its own is the header's $HYPERLINKBASE. */
  std::int32_t TotalEditingTimeDays{};
  std::int32_t TotalEditingTimeMilliseconds{}; /**< Into the last day. */
  std::int32_t CreateDateTimeDays{};
  std::int32_t CreateDateTimeMilliseconds{};
  std::int32_t ModifiedDateTimeDays{};
  std::int32_t ModifiedDateTimeMilliseconds{};
  std::vector<Property> Properties{};

  auto operator==(const SummaryInfo &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_SUMMARY_HPP */
