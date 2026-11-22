/***********************************************************************************************************************
 *  @file      source/objects/write.hpp
 *  @brief     One object written in a release, from its size through its CRC.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_WRITE_HPP
#define DWGIO_OBJECTS_WRITE_HPP

#include "fields/context.hpp"

#include "dwgio/object.hpp"

#include <cstddef>
#include <vector>

namespace DwgIO::Objects
{
/**
 *  From the size through the CRC: an Unknown's as kept, or without them its header and its own bits of the release
 *  written; a typed kind's from its fields.
 *
 *  Nothing, and an issue, for a kind stored under a class the context's classes do not declare.
 */
[[nodiscard]] auto WriteObject(const Object &object, const Fields::Context &with) -> std::vector<std::byte>;
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_WRITE_HPP */
