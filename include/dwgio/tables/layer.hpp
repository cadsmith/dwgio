/***********************************************************************************************************************
 *  @file      dwgio/tables/layer.hpp
 *  @brief     The LAYER record.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_LAYER_HPP
#define DWGIO_TABLES_LAYER_HPP

#include "dwgio/api.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/lineweight.hpp"
#include "dwgio/tables/record.hpp"

namespace DwgIO::Tables
{
struct DWGIO_API Layer
{
  Tables::Record Record{};
  bool Frozen{};                                            /**< The 1 bit of group 70.                          */
  bool On{true};                                            /**< DXF negates the color of a layer that is off.   */
  bool FrozenNew{};                                         /**< Frozen in new viewports: the 2 bit of group 70. */
  bool Locked{};                                            /**< The 4 bit of group 70.                          */
  bool Plot{true};                                          /**< Group 290, from R2000.                          */
  DwgIO::Lineweight Lineweight{DwgIO::Lineweight::Default}; /**< From R2000.                                     */
  /**
   *  Group 62; DXF negates it when the layer is off, dwgio keeps the off state in On.
   */
  DwgIO::Color Color{.Index = 7, .Rgb = {}, .Transparency = {}};
  Handle Style{};    /**< A placeholder in the plot style name dictionary. Group 390, from R2000. */
  Handle Material{}; /**< Group 347, from R2007.                                                  */
  Handle Linetype{}; /**< DXF names it in group 6.                                                */
  Handle Unknown1{}; /**< Unnamed in the specification; null as AutoCAD writes it. From R2013.    */

  auto operator==(const Layer &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_LAYER_HPP */
