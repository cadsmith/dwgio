/***********************************************************************************************************************
 *  @file      dwgio/base.hpp
 *  @brief     The handle, owner, reactors, extension dictionary and extended data every object carries.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BASE_HPP
#define DWGIO_BASE_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/xdata.hpp"

#include <vector>

namespace DwgIO
{
/**
 *  What every object carries: its handle, owner, reactors, extension dictionary and extended data.
 */
struct DWGIO_API Base
{
  Handle Self{};
  /**
   *  A table's control object, a dictionary's parent, an entity's block record; null for the roots and for an entity
   *  whose space the file gives instead.
   */
  Handle Owner{};
  Handle Dictionary{}; /**< The extension dictionary; null for none. */
  std::vector<Handle> Reactors{};
  std::vector<Xdata> Extended{}; /**< One record per application. */
  /**
   *  From R2013, whether the object has data in the data store section, which the drawing keeps as bytes.
   */
  bool Stored{};

  auto operator==(const Base &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_BASE_HPP */
