/***********************************************************************************************************************
 *  @file      source/file/locator/section.hpp
 *  @brief     A sectioned block cut from its frame, and wrapped in one.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_LOCATOR_SECTION_HPP
#define DWGIO_FILE_LOCATOR_SECTION_HPP

#include "file/locator/head.hpp"
#include "file/locator/sentinels.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <vector>

namespace DwgIO::File::Locator
{
/**
 *  A sectioned block out of its frame: the body between the size and the CRC, and what the frame had to note.
 */
struct Framed
{
  std::vector<std::byte> Body;
  std::vector<Issue> Issues; /**< A sentinel or CRC that did not match, at its file offset. */
};

/**
 *  Cuts a sectioned block out: begin sentinel, RL size, when the marks are wide another RL the size does not count,
 *  body, CRC over the size, that RL and the body, end sentinel.
 *
 *  A mismatch is noted; a block beyond the file or running past its end fails.
 */
[[nodiscard]] auto Cut(std::span<const std::byte> file, const Record &where, const Marks &marks)
    -> std::expected<Framed, Failure>;

/**
 *  Wraps a body in its frame, with the second RL zero when the marks are wide.
 */
[[nodiscard]] auto Wrap(std::span<const std::byte> body, const Marks &marks) -> std::vector<std::byte>;
} // namespace DwgIO::File::Locator

#endif /* DWGIO_FILE_LOCATOR_SECTION_HPP */
