/***********************************************************************************************************************
 *  @file      source/store/read.hpp
 *  @brief     The reader of a data store's records.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_STORE_READ_HPP
#define DWGIO_STORE_READ_HPP

#include "store/entry.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace DwgIO::Store
{
/**
 *  The records of a data store, in the order of its data index: through the head to the segment index, the schema
 *  index's names, the data index and each record in its data segment, a large one's data gathered from its blob
 *  segments. Why not when the store is broken.
 */
[[nodiscard]] auto Read(std::span<const std::byte> store) -> std::expected<std::vector<Entry>, std::string>;
} // namespace DwgIO::Store

#endif /* DWGIO_STORE_READ_HPP */
