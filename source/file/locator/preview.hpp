/***********************************************************************************************************************
 *  @file      source/file/locator/preview.hpp
 *  @brief     The preview block of chapter 14.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_LOCATOR_PREVIEW_HPP
#define DWGIO_FILE_LOCATOR_PREVIEW_HPP

#include "dwgio/issue.hpp"
#include "dwgio/preview.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace DwgIO::File::Locator
{
/**
 *  Reads the preview block of chapter 14: its sentinel, overall size, the directory of images with their file offsets,
 *  and their bytes.
 *
 *  An image outside the block is left out and noted; a wrong sentinel gives nothing. The offset is the block's place in
 *  the file, which the directory's offsets count from.
 */
[[nodiscard]] auto ReadPreview(std::span<const std::byte> block, std::size_t offset, std::vector<Issue> &issues)
    -> std::vector<PreviewImage>;

/**
 *  Writes the preview block for images to be placed at that file offset.
 */
[[nodiscard]] auto WritePreview(std::span<const PreviewImage> images, std::size_t offset) -> std::vector<std::byte>;
} // namespace DwgIO::File::Locator

#endif /* DWGIO_FILE_LOCATOR_PREVIEW_HPP */
