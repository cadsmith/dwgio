/***********************************************************************************************************************
 *  @file      dwgio/failure.hpp
 *  @brief     Why a drawing could not be read or written.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FAILURE_HPP
#define DWGIO_FAILURE_HPP

#include "dwgio/api.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace DwgIO
{
/**
 *  What kind of failure stopped a load, a save or an edit.
 */
enum class FailureCode : std::uint8_t
{
  Io,                 /**< A stream or a file that could not be read or written.  */
  NotDwg,             /**< Bytes that are no DWG file.                            */
  UnsupportedRelease, /**< A release dwgio does not read.                         */
  Encrypted,          /**< A drawing saved with a password.                       */
  Damaged,            /**< A file whose structure is broken past reading.         */
  Missing,            /**< What an edit needs, such as a table or a handle.       */
  Exists,             /**< A name or a handle an edit would give is taken.        */
  InUse,              /**< An object a removal would take that others need.       */
  Refused,            /**< An edit the drawing cannot take, such as a non-entity. */
};

/**
 *  Why a drawing could not be read or written, or an edit could not be made.
 */
struct DWGIO_API Failure
{
  FailureCode Code{};
  std::optional<std::size_t> Offset{}; /**< Byte offset in the file, when one is to blame. */
  std::string What{};

  auto operator==(const Failure &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_FAILURE_HPP */
