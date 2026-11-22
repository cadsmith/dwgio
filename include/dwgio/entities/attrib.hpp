/***********************************************************************************************************************
 *  @file      dwgio/entities/attrib.hpp
 *  @brief     The ATTRIB entity: an attribute of an insert.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_ATTRIB_HPP
#define DWGIO_ENTITIES_ATTRIB_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/entities/lettering.hpp"

#include <string>

namespace DwgIO::Entities
{
/**
 *  An ATTRIB: an insert's attribute, a tagged text that follows it.
 */
struct DWGIO_API Attrib
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Lettering Lettering{};
  int Revision{}; /**< From R2010; 0 as AutoCAD writes it. */
  /**
   *  From R2018, the attribute type: 1 a single line, 2 a multiline attribute, 4 a multiline definition. Only the
   *  single line is read; another type is kept as bytes.
   */
  int Type{1};
  std::string Tag{};
  int Length{};  /**< Unused. Group 73.                                                            */
  int Flags{};   /**< The flags: 1 invisible, 2 constant, 4 verified on input, 8 preset. Group 70. */
  bool Locked{}; /**< From R2007: the attribute keeps its position in the block. Group 280.        */

  auto operator==(const Attrib &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_ATTRIB_HPP */
