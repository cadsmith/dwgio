/***********************************************************************************************************************
 *  @file      dwgio/objects/xrecord.hpp
 *  @brief     The XRECORD object: an application's DXF groups.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_XRECORD_HPP
#define DWGIO_OBJECTS_XRECORD_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tag.hpp"

#include <cstddef>
#include <vector>

namespace DwgIO::Objects
{
/**
 *  An XRECORD: an application's DXF groups kept in a dictionary.
 */
struct DWGIO_API XRecord
{
  DwgIO::Base Base{};
  int Cloning{1}; /**< What cloning does with a name already taken, as a dictionary's. Group 280, from R2000. */
  std::vector<DwgIO::Tag> Data{}; /**< A point takes three groups. */
  std::vector<Handle> Objects{};  /**< The handles after the groups in the handle stream. */
  std::vector<std::byte> Rest{};  /**< The data bytes after the last group that parsed, when one did not. */

  auto operator==(const XRecord &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_XRECORD_HPP */
