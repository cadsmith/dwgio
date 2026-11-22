/***********************************************************************************************************************
 *  @file      dwgio/objects/visualstyle.hpp
 *  @brief     The VISUALSTYLE object: how faces, edges and lighting show in a view.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_VISUALSTYLE_HPP
#define DWGIO_OBJECTS_VISUALSTYLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/color.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO::Objects::VisualStyles
{
/**
 *  A property's value, of the type its code calls for.
 */
using Setting = std::variant<std::int32_t, double, bool, DwgIO::Color, std::string>;

/**
 *  One property of a visual style, in the place its release's layout gives it.
 */
struct DWGIO_API Property
{
  /**
   *  The group code AutoCAD's DXF of the release gives the property: R14 to R2010 name each property by a code of its
   *  own, such as 71 or 63, R2013 and R2018 by its kind alone, 90, 40, 62, 290 or 1.
   */
  int Code{};
  Setting Value{};
  int Flags{}; /**< From R2010, how the property applies. Group 176. */

  auto operator==(const Property &) const -> bool = default;
};
} // namespace DwgIO::Objects::VisualStyles

namespace DwgIO::Objects
{
/**
 *  A VISUALSTYLE: how faces, edges and lighting show in a view, stored under its class.
 *
 *  Its properties come in three layouts: 29 up to R2004 and 30 in R2007, 28 with their flags in R2010, and 58 from
 *  R2013.
 */
struct DWGIO_API VisualStyle
{
  DwgIO::Base Base{};
  std::string Name{}; /**< Such as "2dWireframe".                */
  int Kind{};         /**< Such as 4 for 2D wireframe. Group 70. */
  /**
   *  From R2010: 2 in every file AutoCAD writes, though its DXF of R2013 and R2018 says 3. Group 177.
   */
  int Version{2};
  bool Internal{};                                  /**< For AutoCAD's internal use only. Group 291. */
  std::vector<VisualStyles::Property> Properties{}; /**< In the order of the release's layout.       */

  auto operator==(const VisualStyle &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_VISUALSTYLE_HPP */
