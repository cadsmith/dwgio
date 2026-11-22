/***********************************************************************************************************************
 *  @file      source/store/write.hpp
 *  @brief     The data store written, as AutoCAD's first save lays it out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_STORE_WRITE_HPP
#define DWGIO_STORE_WRITE_HPP

#include "store/entry.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace DwgIO::Store
{
/**
 *  A data store of those bodies, each under the schema of ACIS bodies, as AutoCAD's first save of a drawing lays one
 *  out: the head, the segment index, the data index, one data segment with every record, the schema index and data, a
 *  blob segment for each page of a record too large to hold itself, and the search segment.
 */
[[nodiscard]] auto Write(std::span<const Entry> bodies) -> std::vector<std::byte>;
} // namespace DwgIO::Store

#endif /* DWGIO_STORE_WRITE_HPP */
