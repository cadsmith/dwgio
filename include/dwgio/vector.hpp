/***********************************************************************************************************************
 *  @file      dwgio/vector.hpp
 *  @brief     Points and directions, and the constants the library builds on them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_VECTOR_HPP
#define DWGIO_VECTOR_HPP

#include "dwgio/api.hpp"

#include <numbers>

namespace DwgIO
{
struct DWGIO_API Vector
{
  double X{};
  double Y{};
  double Z{}; /**< Zero for a point given in two dimensions. */

  auto operator==(const Vector &) const -> bool = default;
};

/**
 *  The extrusion of an entity whose file gives none.
 */
inline constexpr Vector Up{.X = 0, .Y = 0, .Z = 1};

/**
 *  The way text reads when nothing turns it.
 */
inline constexpr Vector Right{.X = 1, .Y = 0, .Z = 0};

/**
 *  A full turn: every angle of the library is in radians.
 */
inline constexpr double Turn = 2.0 * std::numbers::pi;
} // namespace DwgIO

#endif /* DWGIO_VECTOR_HPP */
