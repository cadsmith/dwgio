/***********************************************************************************************************************
 *  @file      source/objects/map.hpp
 *  @brief     The reader and writer of the object map of chapter 23.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_MAP_HPP
#define DWGIO_OBJECTS_MAP_HPP

#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace DwgIO::Objects
{
struct Mapped
{
  Handle Self{};
  std::size_t Offset{}; /**< The file offset of the object's size. */

  auto operator==(const Mapped &) const -> bool = default;
};

/**
 *  Chapter 23: sections of handle and offset deltas, each with its CRC, up to the terminator.
 *
 *  Each section counts from zero, as AutoCAD's files do, where the text counts across them. A section cut short ends
 *  the read with an issue; a wrong CRC is noted and the section kept. The offset is the map's place in the file, for
 *  the issues.
 */
[[nodiscard]] auto ReadMap(std::span<const std::byte> map, std::size_t offset, std::vector<Issue> &issues)
    -> std::vector<Mapped>;

/**
 *  The entries sorted by handle, in sections closed once they reach 2032 bytes as AutoCAD closes them.
 */
[[nodiscard]] auto WriteMap(std::span<const Mapped> entries) -> std::vector<std::byte>;
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_MAP_HPP */
