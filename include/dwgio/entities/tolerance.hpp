/***********************************************************************************************************************
 *  @file      dwgio/entities/tolerance.hpp
 *  @brief     The TOLERANCE entity: a feature control frame.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_TOLERANCE_HPP
#define DWGIO_ENTITIES_TOLERANCE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <string>

namespace DwgIO::Entities
{
/**
 *  A TOLERANCE: a feature control frame of geometric tolerancing.
 */
struct DWGIO_API Tolerance
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  int Unknown1{};          /**< Unnamed in the specification. Up to R14.                  */
  double Height{};         /**< The height of the frame's text; up to R14.                */
  double Gap{};            /**< DIMGAP times DIMSCALE when the frame was made; up to R14. */
  Vector Position{};       /**< In world coordinates.                                     */
  Vector Direction{Right}; /**< The frame's X axis, in world coordinates.                 */
  Vector Normal{Up};
  std::string Text{}; /**< In the codes AutoCAD writes for its symbols and boxes. Group 1. */
  Handle Style{};

  auto operator==(const Tolerance &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_TOLERANCE_HPP */
