/***********************************************************************************************************************
 *  @file      source/saving/files.hpp
 *  @brief     The file a save writes in each container: paged, coded or by locator.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_SAVING_FILES_HPP
#define DWGIO_SAVING_FILES_HPP

#include "saving/assembly.hpp"

#include "dwgio/drawing.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace DwgIO::Saving
{
/**
 *  A paged file of the sections in the order given; written twice when there is a preview, whose records hold their own
 *  file address.
 */
[[nodiscard]] auto WritePaged(Assembly &writing, const Drawing &drawing, std::span<const std::string> order)
    -> std::vector<std::byte>;

/**
 *  A coded file of the sections in the order given; written twice when there is a preview, whose records hold their own
 *  file address.
 */
[[nodiscard]] auto WriteCoded(Assembly &writing, const Drawing &drawing, std::span<const std::string> order)
    -> std::vector<std::byte>;

/**
 *  A locator file: the head with its records, then each block of the order.
 */
[[nodiscard]] auto WriteLocator(Assembly &writing, const Drawing &drawing, std::span<const std::string> order)
    -> std::vector<std::byte>;
} // namespace DwgIO::Saving

#endif /* DWGIO_SAVING_FILES_HPP */
