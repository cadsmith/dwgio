/***********************************************************************************************************************
 *  @file      source/file/paged/magic.hpp
 *  @brief     The magic sequence of chapter 4.1.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_PAGED_MAGIC_HPP
#define DWGIO_FILE_PAGED_MAGIC_HPP

#include <cstddef>
#include <vector>

namespace DwgIO::File::Paged
{
/**
 *  The magic sequence of chapter 4.1, from the seed 1: the file header's record is XORed with its first 0x6c bytes, the
 *  0x14 bytes before 0x100 are its bytes 0xEC to 0x100, and the padding after a page starts it over.
 */
[[nodiscard]] auto MakeMagic(std::size_t count) -> std::vector<std::byte>;
} // namespace DwgIO::File::Paged

#endif /* DWGIO_FILE_PAGED_MAGIC_HPP */
