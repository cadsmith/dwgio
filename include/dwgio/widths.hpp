/***********************************************************************************************************************
 *  @file      dwgio/widths.hpp
 *  @brief     A polyline segment's start and end widths.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_WIDTHS_HPP
#define DWGIO_WIDTHS_HPP

#include "dwgio/api.hpp"

namespace DwgIO
{
/**
 *  A polyline segment's start and end widths: groups 40 and 41 of the vertex it starts at.
 */
struct DWGIO_API Widths
{
  double Start{};
  double End{};

  auto operator==(const Widths &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_WIDTHS_HPP */
