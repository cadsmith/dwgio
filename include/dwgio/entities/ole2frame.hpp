/***********************************************************************************************************************
 *  @file      dwgio/entities/ole2frame.hpp
 *  @brief     The OLE2FRAME entity: an embedded OLE object.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_OLE2FRAME_HPP
#define DWGIO_ENTITIES_OLE2FRAME_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  An OLE2FRAME: an OLE object, such as a spreadsheet.
 *
 *  Typed from the specification alone: no AutoCAD-written fixture checks it. DXF's description and corners lie inside
 *  the data.
 */
struct DWGIO_API Ole2Frame
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  int Version{2}; /**< The OLE version; the specification calls it the flags. Group 70. */
  int Mode{};     /**< From R2000, with no DXF group.                                   */
  std::vector<std::byte> Data{};
  std::uint8_t Unknown1{}; /**< Unnamed in the specification. From R2000. */

  auto operator==(const Ole2Frame &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_OLE2FRAME_HPP */
