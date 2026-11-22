/***********************************************************************************************************************
 *  @file      source/file/locator/layout.hpp
 *  @brief     A locator file cut into its blocks.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_LOCATOR_LAYOUT_HPP
#define DWGIO_FILE_LOCATOR_LAYOUT_HPP

#include "file/locator/head.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace DwgIO::File::Locator
{
/**
 *  One block of the file: its name, its bytes, a sectioned block's without the frame, and where they start.
 */
struct Located
{
  /**
   *  auxiliary, preview, header, classes, pad, objects, map, second, measurement, or unknown.
   */
  std::string Name;
  std::size_t Offset{}; /**< The file offset of the bytes. */
  std::vector<std::byte> Bytes;
};

/**
 *  The whole file cut into blocks, in file order.
 */
struct Layout
{
  Locator::Head Head;
  std::vector<Located> Blocks; /**< The blocks after the head, none overlapping, together the rest of the file. */
};

/**
 *  Cuts a file into its blocks: the records locate most, the preview and the second header their sentinels, the objects
 *  the lowest map offset in no other block; the pad is what lies before the objects, and bytes no block claims are kept
 *  as unknown and noted.
 *
 *  A preview or second header over another block is dropped and noted; two located blocks over each other fail.
 */
[[nodiscard]] auto ReadLayout(std::span<const std::byte> file, std::vector<Issue> &issues)
    -> std::expected<Layout, Failure>;
} // namespace DwgIO::File::Locator

#endif /* DWGIO_FILE_LOCATOR_LAYOUT_HPP */
