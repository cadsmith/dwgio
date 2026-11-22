/***********************************************************************************************************************
 *  @file      dwgio/entities/light.hpp
 *  @brief     The LIGHT entity: a point, spot or distant light.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_LIGHT_HPP
#define DWGIO_ENTITIES_LIGHT_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>
#include <string>

namespace DwgIO::Entities
{
/**
 *  A LIGHT: a point, spot or distant light, stored under its class.
 */
struct DWGIO_API Light
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  std::int32_t Version{1}; /**< 1 as AutoCAD writes it. Group 90. */
  std::string Name{};
  std::int32_t Kind{2}; /**< 1 distant, 2 point, 3 spot. Group 70. */
  bool Lit{true};
  /**
   *  White for a new light; R14 and R2000 keep its index alone. Groups 63 and 421.
   */
  DwgIO::Color Color{.Index = 7};
  bool Glyph{}; /**< Whether the light's glyph is plotted. Group 291. */
  double Intensity{1};
  Vector Position{};     /**< In world coordinates.                                     */
  Vector Target{};       /**< Where a spot or distant light aims, in world coordinates. */
  std::int32_t Fading{}; /**< 0 none, 1 inverse linear, 2 inverse square. Group 72.     */
  bool Limited{};        /**< The light reaches only from Start to End. Group 292.      */
  double AttenuationStart{1};
  double AttenuationEnd{10};
  double Hotspot{Turn / 8};      /**< A spot light's bright inner cone, in radians; DXF gives degrees. Group 50. */
  double Falloff{Turn * 5 / 36}; /**< A spot light's whole cone, in radians; DXF gives degrees. Group 51.        */
  bool CastShadows{true};
  std::int32_t Shading{};   /**< 0 ray traced, 1 shadow map. Group 73. */
  int Size{256};            /**< Of the shadow map. Group 91.          */
  std::uint8_t Softness{1}; /**< Of the shadow map's edges. Group 280. */

  auto operator==(const Light &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_LIGHT_HPP */
