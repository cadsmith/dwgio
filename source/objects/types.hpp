/***********************************************************************************************************************
 *  @file      source/objects/types.hpp
 *  @brief     The fixed type codes of chapter 20.3 and their names.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_TYPES_HPP
#define DWGIO_OBJECTS_TYPES_HPP

#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  Chapter 20.3, such as "LINE" for 0x13; empty for a code with none.
 */
[[nodiscard]] auto TypeNameOf(std::uint16_t type) -> std::string_view;

[[nodiscard]] auto IsEntity(std::uint16_t type) -> bool;

/**
 *  The item class a class of entities declares.
 */
inline constexpr int EntityClass = 0x1F2;
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_TYPES_HPP */
