/***********************************************************************************************************************
 *  @file      source/objects/walk.hpp
 *  @brief     The objects found one after another where the object map fails.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_WALK_HPP
#define DWGIO_OBJECTS_WALK_HPP

#include <cstddef>
#include <span>
#include <vector>

namespace DwgIO::Objects
{
/**
 *  Finds the objects between two file offsets without the map: the first is at the first offset within sixty-four bytes
 *  of the start whose size and CRC agree, and each next one follows the one before.
 *
 *  The walk stops at the first object that does not add up, as a file cut short ends.
 */
[[nodiscard]] auto Walk(std::span<const std::byte> file, std::size_t from, std::size_t to) -> std::vector<std::size_t>;
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_WALK_HPP */
