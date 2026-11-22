/***********************************************************************************************************************
 *  @file      dwgio/corners.hpp
 *  @brief     The four corners of a face, a solid or a trace.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_CORNERS_HPP
#define DWGIO_CORNERS_HPP

#include "dwgio/vector.hpp"

#include <array>

namespace DwgIO
{
/**
 *  The corners of a face, a solid or a trace; a triangle repeats the third as the fourth.
 */
using Corners = std::array<Vector, 4>;
} // namespace DwgIO

#endif /* DWGIO_CORNERS_HPP */
