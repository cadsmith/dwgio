/***********************************************************************************************************************
 *  @file      dwgio/issue.hpp
 *  @brief     What the reader or writer kept, mended or left out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ISSUE_HPP
#define DWGIO_ISSUE_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace DwgIO
{
/**
 *  What kind of thing the reader or writer noted.
 */
enum class IssueCode : std::uint8_t
{
  Damaged,    /**< Structure the reader found broken and skipped or repaired.   */
  Truncated,  /**< Data cut short; what fit is kept.                            */
  Checksum,   /**< A CRC or a checksum that does not match.                     */
  Unknown,    /**< A value or a kind dwgio does not know, kept or skipped.      */
  OutOfRange, /**< A value the writer could not write as it is, clamped or cut. */
  NotKept,    /**< What the release or the file saved cannot hold, left out.    */
  Moved,      /**< A block the writer placed elsewhere than the order said.     */
};

/**
 *  Something the reader or writer had to note: what it kept, mended or left out, and where.
 */
struct DWGIO_API Issue
{
  IssueCode Code{};
  std::optional<std::size_t> Offset{}; /**< Byte offset in the file, when one is to blame; none in a save. */
  Handle Subject{};                    /**< Null when it concerns no one record.                           */
  std::string What{};

  auto operator==(const Issue &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_ISSUE_HPP */
