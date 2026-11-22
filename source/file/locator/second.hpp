/***********************************************************************************************************************
 *  @file      source/file/locator/second.hpp
 *  @brief     The second header of chapter 26.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_LOCATOR_SECOND_HPP
#define DWGIO_FILE_LOCATOR_SECOND_HPP

#include "file/locator/head.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace DwgIO::File::Locator
{
/**
 *  One of the second header's handles: the handseed, then the control objects and dictionaries, each numbered.
 */
struct Named
{
  /**
   *  0 the handseed, 1 to 9 the control objects, 10 the viewport entity header control, 11 the named objects
   *  dictionary, 12 the multiline style, 13 the group dictionary.
   */
  std::uint8_t Number{};
  Handle Value{};

  auto operator==(const Named &) const -> bool = default;
};

/**
 *  The second header of chapter 26, which repeats the file header's records and the drawing's root handles.
 */
struct Second
{
  std::uint32_t Location{}; /**< The file offset of the second header's own sentinel, as stored. */
  std::string Version{};    /**< The six characters naming the release, such as "AC1015".        */
  /**
   *  The hundred bits between the version and the records, kept as read: zeros, the maintenance version and four bytes
   *  the specification only lists.
   */
  std::vector<std::byte> Constants{};
  std::vector<Record> Records{}; /**< The locator records as the second header repeats them. */
  std::vector<Named> Handles{};
  std::vector<std::byte> Junk{}; /**< The eight bytes after the CRC, kept as read. */

  auto operator==(const Second &) const -> bool = default;
};

/**
 *  Reads a second header block, sentinels included; a wrong sentinel or a block cut short gives nothing, a wrong CRC is
 *  noted. The offset is the block's place in the file, for the issues.
 */
[[nodiscard]] auto ReadSecond(std::span<const std::byte> block, std::size_t offset, std::vector<Issue> &issues)
    -> std::optional<Second>;

/**
 *  Writes a second header block, sentinels included, each handle in the fewest bytes, the bits before the CRC filled
 *  with ones as AutoCAD fills them.
 */
[[nodiscard]] auto WriteSecond(const Second &second) -> std::vector<std::byte>;
} // namespace DwgIO::File::Locator

#endif /* DWGIO_FILE_LOCATOR_SECOND_HPP */
