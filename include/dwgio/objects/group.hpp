/***********************************************************************************************************************
 *  @file      dwgio/objects/group.hpp
 *  @brief     The GROUP object: entities selected together.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_GROUP_HPP
#define DWGIO_OBJECTS_GROUP_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"

#include <string>
#include <vector>

namespace DwgIO::Objects
{
/**
 *  A GROUP: entities selected together, named in the groups dictionary.
 */
struct DWGIO_API Group
{
  DwgIO::Base Base{};
  std::string Description{};
  int Unnamed{};                 /**< 1 when the group has no name of its own. Group 70.         */
  int Selectable{1};             /**< 1 when picking one entity picks the whole group. Group 71. */
  std::vector<Handle> Members{}; /**< Hard pointers, group 340.                                  */

  auto operator==(const Group &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_GROUP_HPP */
