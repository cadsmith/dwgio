/***********************************************************************************************************************
 *  @file      source/file/paged/layout.hpp
 *  @brief     A paged file cut into its sections, and written from them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FILE_PAGED_LAYOUT_HPP
#define DWGIO_FILE_PAGED_LAYOUT_HPP

#include "file/paged/head.hpp"
#include "file/paged/pages.hpp"

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
 *  The long that opens the objects section, which the map's offsets count; a locator file's pad holds it too.
 */
constexpr std::uint32_t ObjectsMark = 0x0DCA;

/**
 *  One section of the file: dwgio's name for it, its bytes cut to its size, how its pages are stored.
 */
struct Section
{
  /**
   *  header, classes, objects, map, preview, auxiliary, measurement, or the AcDb name of any other.
   */
  std::string Name{};
  std::vector<std::byte> Bytes{};
  bool Compressed{true};
  std::uint32_t PageSize{UsualPage}; /**< The size of its pages, decompressed. */
  std::int32_t Id{};                 /**< Its id in the section map.           */

  auto operator==(const Section &) const -> bool = default;
};

/**
 *  The whole file: the header and the sections in stream order, the empty section left out.
 */
struct Layout
{
  Paged::Head Head{};
  std::vector<Section> Sections{};
};

/**
 *  The failure for a page a section names that the page map lacks: `page 17 missing`.
 */
[[nodiscard]] auto MakeMissingPage(std::uint32_t number) -> Failure;

/**
 *  Cuts a file into its sections: the header, the page map, the section map, then each section's pages in the order the
 *  map gives them, zero pages put in where a page's start says so.
 *
 *  A page the map lacks fails `section AcDb:Header page 17 missing`; one that does not read fails `section
 *  AcDb:AcDbObjects page 6 unreadable`.
 */
[[nodiscard]] auto ReadLayout(std::span<const std::byte> file, std::vector<Issue> &issues)
    -> std::expected<Layout, Failure>;
/**
 *  Writes a file: the sections in the order given, each compressed one cut into pages of its page size and each stored
 *  one in a page of its page size or its own rounded to 0x20, the pages numbered from 1 and laid from 0x100, the
 *  section map and the page map after them numbered after a gap of two, the head's computed fields filled, the record's
 *  copy at the end. The empty section is written first in the section map.
 */
[[nodiscard]] auto WriteLayout(Head &head, std::span<const Section> sections) -> std::vector<std::byte>;
} // namespace DwgIO::File::Paged

#endif /* DWGIO_FILE_PAGED_LAYOUT_HPP */
