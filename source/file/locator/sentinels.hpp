/***********************************************************************************************************************
 *  @file      source/file/locator/sentinels.hpp
 *  @brief     The sentinels that frame a locator file's blocks.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_LOCATOR_SENTINELS_HPP
#define DWGIO_FILE_LOCATOR_SENTINELS_HPP

#include "file/names.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace DwgIO::File::Locator
{
/**
 *  The sixteen bytes that open or close a block, for finding it again when a file is damaged.
 */
using Mark = std::array<std::uint8_t, 16>;

inline constexpr std::size_t Sentinel = std::tuple_size_v<Mark>;

/**
 *  The two marks of a framed block, its name for the issues, and how much its frame adds to what its RL counts.
 */
struct Marks
{
  std::string_view Name; /**< The block's name, such as Names::Header. */
  Mark Begin{};
  Mark End{};
  /**
   *  The bytes the block takes beyond its RL: both marks and the RL itself for the preview, both marks for the second
   *  header whose RL counts itself, both marks, the RL and the CRC for a sectioned block.
   */
  std::size_t Framed{};
  /**
   *  Whether a second RL, which the size does not count, follows the size: the header and classes of R2010 and later,
   *  chapter 9.
   */
  bool Wide{};
};

/**
 *  The marks of a block framed with the second RL.
 */
[[nodiscard]] constexpr auto Widen(Marks marks) -> Marks
{
  marks.Wide = true;
  return marks;
}

[[nodiscard]] auto Matches(std::span<const std::byte> bytes, const Mark &mark) -> bool;

void Put(std::vector<std::byte> &out, const Mark &mark);

namespace Sentinels
{
/**
 *  After the file header's CRC, chapter 3.
 */
inline constexpr Mark HeaderEnd{0x95, 0xA0, 0x4E, 0x28, 0x99, 0x82, 0x1A, 0xE5,
                                0x5E, 0x41, 0xE0, 0x5F, 0x9D, 0x3A, 0x4D, 0x00};

/**
 *  Around the header variables, chapter 9.
 */
inline constexpr Marks Header{
    .Name = Names::Header,
    .Begin = {0xCF, 0x7B, 0x1F, 0x23, 0xFD, 0xDE, 0x38, 0xA9, 0x5F, 0x7C, 0x68, 0xB8, 0x4E, 0x6D, 0x33, 0x5F},
    .End = {0x30, 0x84, 0xE0, 0xDC, 0x02, 0x21, 0xC7, 0x56, 0xA0, 0x83, 0x97, 0x47, 0xB1, 0x92, 0xCC, 0xA0},
    .Framed = 38,
};

/**
 *  Around the classes, chapter 10.
 */
inline constexpr Marks Classes{
    .Name = Names::Classes,
    .Begin = {0x8D, 0xA1, 0xC4, 0xB8, 0xC4, 0xA9, 0xF8, 0xC5, 0xC0, 0xDC, 0xF4, 0x5F, 0xE7, 0xCF, 0xB6, 0x8A},
    .End = {0x72, 0x5E, 0x3B, 0x47, 0x3B, 0x56, 0x07, 0x3A, 0x3F, 0x23, 0x0B, 0xA0, 0x18, 0x30, 0x49, 0x75},
    .Framed = 38,
};

/**
 *  Around the preview images, chapter 14.
 */
inline constexpr Marks Preview{
    .Name = Names::Preview,
    .Begin = {0x1F, 0x25, 0x6D, 0x07, 0xD4, 0x36, 0x28, 0x28, 0x9D, 0x57, 0xCA, 0x3F, 0x9D, 0x44, 0x10, 0x2B},
    .End = {0xE0, 0xDA, 0x92, 0xF8, 0x2B, 0xC9, 0xD7, 0xD7, 0x62, 0xA8, 0x35, 0xC0, 0x62, 0xBB, 0xEF, 0xD4},
    .Framed = 36,
};

/**
 *  Around the second header, chapter 26.
 */
inline constexpr Marks Second{
    .Name = "second header",
    .Begin = {0xD4, 0x7B, 0x21, 0xCE, 0x28, 0x93, 0x9F, 0xBF, 0x53, 0x24, 0x40, 0x09, 0x12, 0x3C, 0xAA, 0x01},
    .End = {0x2B, 0x84, 0xDE, 0x31, 0xD7, 0x6C, 0x60, 0x40, 0xAC, 0xDB, 0xBF, 0xF6, 0xED, 0xC3, 0x55, 0xFE},
    .Framed = 32,
};
} // namespace Sentinels
} // namespace DwgIO::File::Locator

#endif /* DWGIO_FILE_LOCATOR_SENTINELS_HPP */
