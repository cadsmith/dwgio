/***********************************************************************************************************************
 *  @file      dwgio/tables/axes.hpp
 *  @brief     The user coordinate system a view or a viewport keeps with it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_AXES_HPP
#define DWGIO_TABLES_AXES_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Tables
{
/**
 *  The user coordinate system a view or a viewport keeps with it.
 */
struct DWGIO_API Axes
{
  Vector Origin{};
  Vector X{.X = 1, .Y = 0, .Z = 0};
  Vector Y{.X = 0, .Y = 1, .Z = 0};
  /**
   *  The orthographic view it stands for: 0 none, 1 top, 2 bottom, 3 front, 4 back, 5 left, 6 right. Group 79.
   */
  int Ortho{};
  double Elevation{};
  Handle Named{}; /**< The UCS record, when the system is a named one. Group 345.                        */
  Handle Basis{}; /**< The UCS record an orthographic system is set from; none for the world. Group 346. */

  auto operator==(const Axes &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_AXES_HPP */
