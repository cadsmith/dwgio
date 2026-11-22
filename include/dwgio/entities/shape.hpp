/***********************************************************************************************************************
 *  @file      dwgio/entities/shape.hpp
 *  @brief     The SHAPE entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPE_HPP
#define DWGIO_ENTITIES_SHAPE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

namespace DwgIO::Entities
{
/**
 *  A SHAPE: one shape of a shape file, placed by its number.
 */
struct DWGIO_API Shape
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Vector Position{}; /**< In object coordinates.  */
  double Size{1};    /**< The shape's height.     */
  double Rotation{}; /**< In radians.             */
  double Width{1};   /**< A fraction of the size. */
  double Oblique{};  /**< In radians.             */
  double Thickness{};
  int Number{}; /**< The shape's number in its file; DXF names the shape instead, in group 2. */
  Vector Extrusion{Up};
  Handle Style{}; /**< The STYLE record of the shape file. */

  auto operator==(const Shape &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_SHAPE_HPP */
