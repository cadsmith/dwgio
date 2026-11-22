/***********************************************************************************************************************
 *  @file      dwgio/objects/scale.hpp
 *  @brief     The SCALE object: an entry of the scale list.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SCALE_HPP
#define DWGIO_OBJECTS_SCALE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"

#include <string>

namespace DwgIO::Objects
{
/**
 *  A SCALE: one entry of the scale list, paper units to drawing units.
 */
struct DWGIO_API Scale
{
  DwgIO::Base Base{};
  int Flags{};        /**< 0 as AutoCAD writes it. Group 70. */
  std::string Name{}; /**< Such as "1:50".                   */
  double Paper{1};    /**< 1 in 1:50.                        */
  double Drawing{1};  /**< 50 in 1:50.                       */
  bool Unit{};        /**< Marks the 1:1 scale. Group 290.   */

  auto operator==(const Scale &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SCALE_HPP */
