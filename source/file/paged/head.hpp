/***********************************************************************************************************************
 *  @file      source/file/paged/head.hpp
 *  @brief     The file header of a paged file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_PAGED_HEAD_HPP
#define DWGIO_FILE_PAGED_HEAD_HPP

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

namespace DwgIO::File::Paged
{
/**
 *  The record of chapter 4.1 that the file header keeps encrypted at 0x80 and repeats at the file's end, less the
 *  constants it opens with. The page map address is as stored, 0x100 short of the file address.
 */
struct Record
{
  std::uint32_t RootGap{};
  std::uint32_t LeftGap{};
  std::uint32_t RightGap{};
  std::uint32_t Unknown1{1}; /**< Unnamed in the specification; 1 as AutoCAD writes it. */
  std::uint32_t LastPageId{};
  std::uint64_t LastPageEnd{};
  std::uint64_t SecondHeader{};
  std::uint32_t GapCount{};
  std::uint32_t PageCount{};
  std::uint32_t PageMapId{};
  std::uint64_t PageMapAddress{};
  std::uint32_t SectionMapId{};
  std::uint32_t PageArraySize{};
  std::uint32_t GapArraySize{};
  std::uint32_t Crc{};

  auto operator==(const Record &) const -> bool = default;
};

/**
 *  The file header of a paged file: the plain fields of its first 0x80 bytes and the record.
 */
struct Head
{
  Version Release{};               /**< The release the first six bytes name.                                      */
  std::uint8_t Maintenance{};      /**< The maintenance byte at 0x0B.                                              */
  std::uint8_t Flag{3};            /**< The byte at 0x0C, 3 in AutoCAD's files.                                    */
  std::uint32_t Image{};           /**< The file address of the preview page's data at 0x0D, 0 when there is none. */
  std::uint8_t DwgVersion{0x21};   /**< The application version at 0x11, 33 in AutoCAD 2026's files.               */
  std::uint8_t MaintRelease{0xFF}; /**< The application maintenance at 0x12.                                       */
  std::uint16_t Codepage{30};      /**< The code page number at 0x13.                                              */
  /**
   *  The three bytes at 0x15, which the text says are zero and AutoCAD 2026 fills with a zero and the application
   *  version and maintenance again; kept as read.
   */
  std::array<std::uint8_t, 3> Repeat{0, 0x21, 0xFF};
  std::uint32_t Security{}; /**< The security flags at 0x18; a file with any set is not read. */
  std::uint32_t Unknown1{}; /**< Unnamed in the specification: the long at 0x1C.              */
  /**
   *  The file address of the summary info page's data at 0x20, 0 when there is none.
   */
  std::uint32_t Summary{};
  std::uint32_t Vba{}; /**< The VBA project address at 0x24. */
  /**
   *  The 0x54 bytes from 0x2C, which the text says are zero and AutoCAD 2026 fills with the addresses of the AppInfo
   *  pages among other things; kept as read, zero in a file dwgio makes.
   */
  std::array<std::byte, 0x54> Extra{};
  Paged::Record Record{};

  auto operator==(const Head &) const -> bool = default;
};

/**
 *  Where the record lies and how long it is.
 */
constexpr std::size_t RecordAt = 0x80;
constexpr std::size_t RecordSize = 0x6C;
/**
 *  Where the pages start.
 */
constexpr std::size_t Pages = 0x100;

/**
 *  Whether the header and classes sections carry a second RL after their size: R2010 and R2013 when the maintenance
 *  version is above 3, as chapter 9 says, and every R2018 file.
 */
[[nodiscard]] constexpr auto IsWide(Version release, int maintenance) -> bool
{
  constexpr int Plain = 3;
  return release >= Version::R2018 || (release >= Version::R2010 && maintenance > Plain);
}

/**
 *  Reads the file header: a file shorter than 0x100 bytes or whose record is not AutoCAD's fails with `paged header
 *  unreadable`, one with security flags with `encrypted drawing not read`; a record whose CRC does not match, or whose
 *  copy at the end of the file differs or is missing, is noted.
 */
[[nodiscard]] auto ReadHead(std::span<const std::byte> file, std::vector<Issue> &issues)
    -> std::expected<Head, Failure>;

/**
 *  Writes the 0x100 bytes of the file header, the record's CRC computed.
 */
[[nodiscard]] auto WriteHead(const Head &head) -> std::vector<std::byte>;

/**
 *  The 0x6c bytes of the record as the file's end repeats them.
 */
[[nodiscard]] auto WriteCopy(const Head &head) -> std::vector<std::byte>;
} // namespace DwgIO::File::Paged

#endif /* DWGIO_FILE_PAGED_HEAD_HPP */
