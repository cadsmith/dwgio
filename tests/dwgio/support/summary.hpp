/***********************************************************************************************************************
 *  @file      tests/dwgio/support/summary.hpp
 *  @brief     The fields of a summary a save keeps, to compare a summary before and after one.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_SUMMARY_HPP
#define DWGIO_TESTS_SUPPORT_SUMMARY_HPP

#include "dwgio/summary.hpp"

namespace DwgIO
{
/**
 *  A summary without what each save stamps: the saver, the time of the change and the editing time.
 */
inline auto ClearSaveStamps(SummaryInfo summary) -> SummaryInfo
{
  summary.LastSavedBy.clear();
  summary.ModifiedDateTimeDays = 0;
  summary.ModifiedDateTimeMilliseconds = 0;
  summary.TotalEditingTimeDays = 0;
  summary.TotalEditingTimeMilliseconds = 0;
  return summary;
}
} // namespace DwgIO

#endif /* DWGIO_TESTS_SUPPORT_SUMMARY_HPP */
