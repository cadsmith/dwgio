/***********************************************************************************************************************
 *  @file      dwgio/entities/section.hpp
 *  @brief     The SECTIONOBJECT entity: a section plane through solids and surfaces.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SECTION_HPP
#define DWGIO_ENTITIES_SECTION_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  A SECTIONOBJECT: a section plane through 3D solids and surfaces, stored under its class.
 */
struct DWGIO_API SectionObject
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  std::int32_t State{1}; /**< What the section cuts with: 1 a plane, 2 a boundary, 4 a volume. Group 90. */
  std::int32_t Flags{};  /**< Settings bits DXF does not spell out. Group 91.                            */
  std::string Name{};
  Vector Vertical{Up}; /**< The section plane's vertical direction.         */
  double Top{};        /**< The height of the top of the section volume.    */
  double Bottom{};     /**< The height of the bottom of the section volume. */
  int Transparency{};  /**< Of the plane's indicator. Group 70.             */
  /**
   *  Of the plane's indicator; every release stores it as R2004 does, its method and index in the true color's long.
   */
  DwgIO::Color Color{};
  std::vector<Vector> Vertices{}; /**< Of the section line.                       */
  std::vector<Vector> Back{};     /**< Of the back line.                          */
  Handle Settings{};              /**< The SECTION_SETTINGS object, a hard owner. */

  auto operator==(const SectionObject &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_SECTION_HPP */
