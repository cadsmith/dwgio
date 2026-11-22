/***********************************************************************************************************************
 *  @file      dwgio/objects/sortents.hpp
 *  @brief     The SORTENTSTABLE object: a block's draw order.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SORTENTS_HPP
#define DWGIO_OBJECTS_SORTENTS_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"

#include <vector>

namespace DwgIO::Objects::SortEntsTables
{
/**
 *  One entity moved in the draw order.
 */
struct DWGIO_API Entry
{
  Handle Entity{};
  Handle Key{}; /**< The handle the entity sorts by in place of its own. Group 5. */

  auto operator==(const Entry &) const -> bool = default;
};
} // namespace DwgIO::Objects::SortEntsTables

namespace DwgIO::Objects
{
/**
 *  A SORTENTSTABLE: the draw order of a block's entities where it departs from their handles.
 */
struct DWGIO_API SortEntsTable
{
  DwgIO::Base Base{};
  Handle Block{}; /**< The BLOCK_RECORD of the space whose entities it orders. Group 330. */
  std::vector<SortEntsTables::Entry> Entries{};

  auto operator==(const SortEntsTable &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SORTENTS_HPP */
