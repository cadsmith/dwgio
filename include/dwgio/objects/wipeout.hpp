/***********************************************************************************************************************
 *  @file      dwgio/objects/wipeout.hpp
 *  @brief     The WIPEOUTVARIABLES object.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_WIPEOUT_HPP
#define DWGIO_OBJECTS_WIPEOUT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"

namespace DwgIO::Objects
{
/**
 *  A WIPEOUTVARIABLES: the ACAD_WIPEOUT_VARS entry of the root dictionary.
 */
struct DWGIO_API WipeoutVariables
{
  DwgIO::Base Base{};
  int Frame{}; /**< 1 draws a frame around wipeouts, 0 does not. Group 70. */

  auto operator==(const WipeoutVariables &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_WIPEOUT_HPP */
