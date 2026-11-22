/***********************************************************************************************************************
 *  @file      source/objects/storage.hpp
 *  @brief     The class a release stores an object under.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_STORAGE_HPP
#define DWGIO_OBJECTS_STORAGE_HPP

#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <string_view>

namespace DwgIO::Objects
{
/**
 *  Empty for an object of a fixed type in that release.
 */
[[nodiscard]] auto ClassOf(const Object &object, Version release) -> std::string_view;
/**
 *  Whatever type a release stores the object under; empty for a kind of no class.
 */
[[nodiscard]] auto ClassOf(const Object &object) -> std::string_view;
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_STORAGE_HPP */
