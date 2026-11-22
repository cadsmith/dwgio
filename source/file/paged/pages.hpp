/***********************************************************************************************************************
 *  @file      source/file/paged/pages.hpp
 *  @brief     The pages, page map and section map of the paged container, chapter 4.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_PAGED_PAGES_HPP
#define DWGIO_FILE_PAGED_PAGES_HPP

#include "file/paged/head.hpp"
#include "file/paged/names.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace DwgIO::File::Paged
{
/**
 *  The page types of chapter 4.3 and 4.6.
 */
constexpr std::uint32_t PageMapType = 0x41630E3B;
constexpr std::uint32_t SectionMapType = 0x4163003B;
constexpr std::uint32_t DataPageType = 0x4163043B;
/**
 *  Every page starts on this boundary.
 */
constexpr std::size_t Alignment = 0x20;
/**
 *  The header of a system page and of a data page.
 */
constexpr std::size_t SystemHeader = 0x14;
constexpr std::size_t DataHeader = 0x20;

/**
 *  One entry of the page map: a page's number, where it lies and how many bytes it is allotted; a gap has a negative
 *  number.
 */
struct Page
{
  std::int32_t Number{};
  std::size_t Address{};
  std::size_t Size{};

  auto operator==(const Page &) const -> bool = default;
};

/**
 *  One page of a section as the section map lists it: its number, the bytes stored after its header, and where its
 *  bytes start in the section.
 */
struct SectionPage
{
  std::uint32_t Number{};
  std::uint32_t Size{};
  std::uint64_t Start{};

  auto operator==(const SectionPage &) const -> bool = default;
};

/**
 *  A section as the section map describes it, chapter 4.5.
 */
struct Description
{
  std::uint64_t Size{};
  std::uint32_t PageSize{UsualPage};
  std::uint32_t Unknown1{1};   /**< Unnamed in the specification; 1 as AutoCAD writes it. */
  std::uint32_t Compressed{2}; /**< 1 stored, 2 compressed.                               */
  std::int32_t Id{};
  std::uint32_t Encrypted{};
  std::string Name{};
  std::vector<SectionPage> Pages{};

  auto operator==(const Description &) const -> bool = default;
};

/**
 *  Reads the pages of a file, noting what does not check.
 */
class Pager
{
public:
  Pager(std::span<const std::byte> file, std::vector<Issue> &issues) : m_File(file), m_Issues(&issues) {}

  /**
   *  A system page: the header, the data decompressed to the size it says; a checksum that does not match is noted
   *  `page at 0x... checksum mismatch`; a page beyond the file, of another type or that does not decompress fails `page
   *  at 0x... unreadable`.
   */
  [[nodiscard]] auto SystemPage(std::size_t address, std::uint32_t type) const
      -> std::expected<std::vector<std::byte>, Failure>;

  /**
   *  The page map the head names: the pages from 0x100 on, gaps included; fails `page map unreadable`.
   */
  [[nodiscard]] auto PageMap(const Head &head) const -> std::expected<std::vector<Page>, Failure>;

  /**
   *  The section map the head names, from the page of its number; fails `section map unreadable`.
   */
  [[nodiscard]] auto SectionMap(std::span<const Page> pages, const Head &head) const
      -> std::expected<std::vector<Description>, Failure>;

  /**
   *  A data page at the address onto the end of `into`, which holds the section's pages before it: the header unmasked,
   *  both checksums checked and a mismatch noted, the data decompressed when the section is, and zeros to the section's
   *  page size.
   *
   *  False when the page lies beyond the file, is of another type or does not decompress.
   */
  [[nodiscard]] auto DataPage(std::size_t address, const Description &section, std::vector<std::byte> &into) const
      -> bool;

private:
  std::span<const std::byte> m_File;
  std::vector<Issue> *m_Issues;
};

/**
 *  Where a data page goes: its number, its address in the file, and where its bytes start in the section.
 */
struct Placement
{
  std::uint32_t Number{};
  std::size_t Address{};
  std::uint64_t Start{};
};

/**
 *  A data page as the file stores it, and how many bytes of data its header counts.
 */
struct Stored
{
  std::vector<std::byte> Bytes{};
  std::uint32_t Size{};
};

/**
 *  Writes a data page as the file stores it: the masked header, the page's bytes compressed when the section is and
 *  stored whole otherwise, and the padding from the magic sequence to the next 0x20 boundary.
 */
[[nodiscard]] auto WriteDataPage(const Placement &where, const Description &section, std::span<const std::byte> page)
    -> Stored;

/**
 *  Writes a system page: the header, the bytes compressed, the trailer and the padding.
 */
[[nodiscard]] auto WriteSystemPage(std::uint32_t type, std::span<const std::byte> bytes) -> std::vector<std::byte>;

/**
 *  The page map's bytes before compression: each page's number and size.
 */
[[nodiscard]] auto WritePageMap(std::span<const Page> pages) -> std::vector<std::byte>;

/**
 *  The section map's bytes before compression: the count, the constants, each description with its pages.
 */
[[nodiscard]] auto WriteSectionMap(std::span<const Description> sections) -> std::vector<std::byte>;

/**
 *  The page of that number, or null.
 */
[[nodiscard]] auto FindPage(std::span<const Page> pages, std::uint32_t number) -> const Page *;
} // namespace DwgIO::File::Paged

#endif /* DWGIO_FILE_PAGED_PAGES_HPP */
