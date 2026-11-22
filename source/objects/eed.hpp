/***********************************************************************************************************************
 *  @file      source/objects/eed.hpp
 *  @brief     An object's extended data read and written, chapter 28.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_EED_HPP
#define DWGIO_OBJECTS_EED_HPP

#include "bits/reader.hpp"
#include "fields/context.hpp"

#include "dwgio/xdata.hpp"

#include <vector>

namespace DwgIO::Objects
{
/**
 *  Chapter 28: one record per application, each a size, the application's handle and its items, until a size of zero.
 *
 *  A record running past the end fails the reader; an item code the chapter lacks ends its record with an issue.
 */
[[nodiscard]] auto ReadEed(Bits::Reader &in, const Fields::Context &with) -> std::vector<Xdata>;

/**
 *  A group the chapter cannot store, or whose value is not its code's, is left out and noted; a text or chunk over 255
 *  bytes is cut and noted; a point missing its second or third group is completed with zeros and noted; a record with
 *  nothing to write is left out and noted, since a size of zero ends the extended data.
 */
void WriteEed(const std::vector<Xdata> &records, const Fields::Output &to);
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_EED_HPP */
