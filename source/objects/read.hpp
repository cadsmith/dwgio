/***********************************************************************************************************************
 *  @file      source/objects/read.hpp
 *  @brief     One object read from its offset in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_READ_HPP
#define DWGIO_OBJECTS_READ_HPP

#include "objects/context.hpp"

#include "dwgio/object.hpp"

#include <cstddef>
#include <optional>
#include <span>

namespace DwgIO::Objects
{
/**
 *  A type the library does not type comes back as an Unknown with the bytes kept whole.
 *
 *  Nothing, with an issue, for an offset beyond the file or a size running past it; a kind cut short is kept with what
 *  was read and noted.
 */
[[nodiscard]] auto ReadObject(std::span<const std::byte> file, std::size_t offset, const FileContext &with)
    -> std::optional<Object>;
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_READ_HPP */
