/***********************************************************************************************************************
 *  @file      source/file/locator/head.hpp
 *  @brief     The file header of an R14 or R2000 file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_LOCATOR_HEAD_HPP
#define DWGIO_FILE_LOCATOR_HEAD_HPP

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

namespace DwgIO::File::Locator
{
/**
 *  One section locator record of chapter 3: which block, where it starts and how long it is.
 */
struct Record
{
  /**
   *  0 header variables, 1 classes, 2 object map, 3 unused, 4 measurement, 5 auxiliary header.
   */
  std::uint8_t Number{};
  std::uint32_t Seeker{}; /**< The file offset. */
  std::uint32_t Size{};   /**< In bytes.        */

  auto operator==(const Record &) const -> bool = default;
};

/**
 *  The file header of an R14 or R2000 file: the release, a few bytes, and the locator records.
 */
struct Head
{
  Version Release{};           /**< The release the first six bytes name.           */
  std::uint8_t Maintenance{};  /**< The maintenance version at byte 0x0B.           */
  std::uint8_t Flag{};         /**< The byte at 0x0C, 1 in AutoCAD's files.         */
  std::uint32_t Image{};       /**< The seeker of the preview images at 0x0D.       */
  std::uint8_t DwgVersion{};   /**< The byte at 0x11, 0x21 in AutoCAD's files.      */
  std::uint8_t MaintRelease{}; /**< The byte at 0x12.                               */
  std::uint16_t Codepage{};    /**< The code page number at 0x13, 30 for ANSI_1252. */
  std::vector<Record> Records; /**< The locator records, in file order.             */

  auto operator==(const Head &) const -> bool = default;
};

/**
 *  Reads the file header; a wrong CRC or sentinel is noted, a file that is no DWG, too short, without records or of a
 *  release that is no locator file fails.
 */
[[nodiscard]] auto ReadHead(std::span<const std::byte> file, std::vector<Issue> &issues)
    -> std::expected<Head, Failure>;

/**
 *  Writes the file header, its CRC and its sentinel.
 */
[[nodiscard]] auto WriteHead(const Head &head) -> std::vector<std::byte>;

/**
 *  How many bytes the written header takes: up to and including its sentinel.
 */
[[nodiscard]] auto SizeOf(const Head &head) -> std::size_t;
} // namespace DwgIO::File::Locator

#endif /* DWGIO_FILE_LOCATOR_HEAD_HPP */
