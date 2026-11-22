/***********************************************************************************************************************
 *  @file      source/file/locator/write.hpp
 *  @brief     A locator file written: R14 and R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_LOCATOR_WRITE_HPP
#define DWGIO_FILE_LOCATOR_WRITE_HPP

#include "file/locator/head.hpp"
#include "file/locator/layout.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace DwgIO::File::Locator
{
/**
 *  The head with its records pointed at the header, classes, map, measurement and auxiliary blocks among those, and its
 *  image seeker at the preview.
 */
[[nodiscard]] auto PointHead(Head head, std::span<const Located> blocks) -> Head;

/**
 *  Writes a whole file: the head pointed at the blocks, then the blocks as they come, each already framed.
 */
[[nodiscard]] auto WriteLayout(const Head &head, std::span<const Located> blocks) -> std::vector<std::byte>;
} // namespace DwgIO::File::Locator

#endif /* DWGIO_FILE_LOCATOR_WRITE_HPP */
