/***********************************************************************************************************************
 *  @file      dwgio/matrix.hpp
 *  @brief     A 4 by 4 transform and the identity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_MATRIX_HPP
#define DWGIO_MATRIX_HPP

#include <array>

namespace DwgIO
{
/**
 *  A 4 by 4 transform, its 16 cells row by row as the file gives them.
 */
using Matrix = std::array<double, 16>;

/**
 *  The transform that leaves everything where it is.
 */
inline constexpr Matrix Identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
} // namespace DwgIO

#endif /* DWGIO_MATRIX_HPP */
