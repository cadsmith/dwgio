/***********************************************************************************************************************
 *  @file      dwgio/objects/dictionary.hpp
 *  @brief     The DICTIONARY and ACDBDICTIONARYWDFLT objects.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_DICTIONARY_HPP
#define DWGIO_OBJECTS_DICTIONARY_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"

#include <optional>
#include <string>
#include <vector>

namespace DwgIO::Objects
{
struct DWGIO_API Entry
{
  std::string Name{};
  Handle Object{}; /**< Owned soft or hard as the dictionary's HardOwner says. Group 350 or 360. */

  auto operator==(const Entry &) const -> bool = default;
};

/**
 *  A DICTIONARY, or an ACDBDICTIONARYWDFLT when Default holds a value.
 */
struct DWGIO_API Dictionary
{
  DwgIO::Base Base{};
  bool HardOwner{}; /**< Owns its entries hard rather than soft. Group 280. */
  /**
   *  What cloning does with a name already taken: 0 nothing, 1 keeps the entry there, 2 takes the clone, 3 to 5 rename
   *  it. Group 281, from R2000.
   */
  int Cloning{1};
  std::vector<Entry> Entries{};    /**< In the file's order.                                  */
  std::optional<Handle> Default{}; /**< The object a name not found falls back to. Group 340. */

  auto operator==(const Dictionary &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_DICTIONARY_HPP */
