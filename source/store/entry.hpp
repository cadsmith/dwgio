/***********************************************************************************************************************
 *  @file      source/store/entry.hpp
 *  @brief     A record of the data store.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_STORE_ENTRY_HPP
#define DWGIO_STORE_ENTRY_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO::Store
{
/**
 *  Where a run of a record's data lies in the data store's bytes.
 */
struct Range
{
  std::size_t Offset{};
  std::size_t Size{};
};

/**
 *  A record of the data store: the object it belongs to, its schema's name, its data, and where that lies.
 */
struct Entry
{
  std::uint64_t Handle{}; /**< The handle of the object the data belongs to.                            */
  std::string Schema{};   /**< The name of the schema the record is of, such as `AcDb3DSolid_ASM_Data`. */
  std::string Data{};
  /**
   *  Where the data lies: one run in a data segment, or a run in each blob segment of its pages.
   */
  std::vector<Range> Ranges{};
};
} // namespace DwgIO::Store

#endif /* DWGIO_STORE_ENTRY_HPP */
