/***********************************************************************************************************************
 *  @file      source/file/coded/layout.hpp
 *  @brief     R2007's coded container, chapter 5.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_CODED_LAYOUT_HPP
#define DWGIO_FILE_CODED_LAYOUT_HPP

#include "file/paged/layout.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

/**
 *  R2007's coded container, chapter 5: pages coded against damage with Reed-Solomon and compressed with its own variant
 *  of LZ77, under a page map and a section map that are system pages of their own.
 */
namespace DwgIO::File::Coded
{
inline constexpr std::size_t MetaSize = 0x80;
inline constexpr std::size_t HeaderSize = 0x400;
/**
 *  Where the pages start, which the maps' offsets count from.
 */
inline constexpr std::size_t Pages = MetaSize + HeaderSize;

/**
 *  The plain bytes of the file's first 0x80, which R2007 lays out as R2004 does.
 */
struct Meta
{
  Version Release{};                   /**< The release the first six bytes name.           */
  std::uint8_t Maintenance{};          /**< The maintenance byte at 0x0B.                   */
  std::uint8_t Flag{3};                /**< The byte at 0x0C.                               */
  std::uint32_t Image{};               /**< The file address of the preview's data at 0x0D. */
  std::uint8_t DwgVersion{0x21};       /**< The application version at 0x11.                */
  std::uint8_t MaintRelease{0xFF};     /**< The application maintenance at 0x12.            */
  std::uint16_t Codepage{30};          /**< The code page number at 0x13.                   */
  std::array<std::byte, 0x80> Bytes{}; /**< The 0x80 bytes as the file has them.            */
};

/**
 *  One section of the section map: its data size, page size, encryption, hash, encoding and name; a section's pages are
 *  read whole into Paged::Section's bytes.
 */
struct Described
{
  std::uint64_t Size{};
  std::uint64_t PageSize{};
  std::uint64_t Encryption{};
  std::uint64_t Hash{};
  std::uint64_t Unknown1{}; /**< Unnamed in the specification; 0 as AutoCAD writes it. */
  std::uint64_t Encoding{};
  std::string Name{};

  auto operator==(const Described &) const -> bool = default;
};

/**
 *  The whole file: the meta bytes, the sections in the section map's order with their descriptions.
 */
struct Layout
{
  Coded::Meta Meta{};
  std::vector<Paged::Section> Sections{};
  std::vector<Described> Descriptions{};
};

/**
 *  The file header's first 0x80 bytes: a file shorter or not of R2007 fails with `coded header unreadable`, one with
 *  security flags with `encrypted drawing not read`.
 */
[[nodiscard]] auto ReadMeta(std::span<const std::byte> file) -> std::expected<Meta, Failure>;

/**
 *  Cuts a file into its sections: the file header at 0x80 decoded and decompressed, the page map and the section map,
 *  then each section's pages in the order the map gives them, zero pages put in where a page's start or the section's
 *  size says so.
 *
 *  A system page or data page that does not decode fails naming it; a page checksum that does not match is noted.
 */
[[nodiscard]] auto ReadLayout(std::span<const std::byte> file, std::vector<Issue> &issues)
    -> std::expected<Layout, Failure>;

/**
 *  Writes a file of the meta's release, maintenance, code page and stamps: the page map and its copy, each section's
 *  pages in the order given, compressed when that makes them smaller and coded as the data pages of encoding 4 whatever
 *  AutoCAD codes them with, the section map and its copy, the file header at 0x80 and again at the end, each system
 *  page compressed.
 *
 *  A section R2007 gives no hash is left out and noted. The meta's image address is set to the preview's first page.
 */
[[nodiscard]] auto WriteLayout(Meta &meta, std::span<const Paged::Section> sections, std::vector<Issue> &issues)
    -> std::vector<std::byte>;
} // namespace DwgIO::File::Coded

#endif /* DWGIO_FILE_CODED_LAYOUT_HPP */
