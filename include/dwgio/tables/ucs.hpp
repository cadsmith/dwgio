/***********************************************************************************************************************
 *  @file      dwgio/tables/ucs.hpp
 *  @brief     The UCS record: a named user coordinate system.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_UCS_HPP
#define DWGIO_TABLES_UCS_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/record.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Tables
{
struct DWGIO_API Ucs
{
  Tables::Record Record{};
  Vector Origin{}; /**< In world coordinates. */
  Vector X{.X = 1, .Y = 0, .Z = 0};
  Vector Y{.X = 0, .Y = 1, .Z = 0};
  double Elevation{}; /**< From R2000.                                                                            */
  int Ortho{};        /**< AutoCAD writes 0. Group 79, from R2000.                                                */
  int Orthographic{}; /**< 0 as AutoCAD writes it. Group 71, from R2000.                                          */
  Handle Basis{};     /**< The UCS an orthographic system is set from; none for the world. Group 346, from R2000. */
  Handle Named{};     /**< From R2000; DXF leaves it out. Null as AutoCAD writes it.                              */

  auto operator==(const Ucs &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_UCS_HPP */
