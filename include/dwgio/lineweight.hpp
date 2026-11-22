/***********************************************************************************************************************
 *  @file      dwgio/lineweight.hpp
 *  @brief     The lineweight codes a DWG stores, and their widths.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_LINEWEIGHT_HPP
#define DWGIO_LINEWEIGHT_HPP

#include "dwgio/api.hpp"

#include <cstdint>
#include <optional>

namespace DwgIO
{
/**
 *  A lineweight as a DWG stores it: a code for one of AutoCAD's standard widths, named by the width in hundredths of a
 *  millimeter, or ByLayer, ByBlock or the default. Group 370 of a DXF gives the width itself.
 */
enum class Lineweight : std::uint8_t
{
  Mm000 = 0,
  Mm005 = 1,
  Mm009 = 2,
  Mm013 = 3,
  Mm015 = 4,
  Mm018 = 5,
  Mm020 = 6,
  Mm025 = 7,
  Mm030 = 8,
  Mm035 = 9,
  Mm040 = 10,
  Mm050 = 11,
  Mm053 = 12,
  Mm060 = 13,
  Mm070 = 14,
  Mm080 = 15,
  Mm090 = 16,
  Mm100 = 17,
  Mm106 = 18,
  Mm120 = 19,
  Mm140 = 20,
  Mm158 = 21,
  Mm200 = 22,
  Mm211 = 23,
  ByLayer = 29,
  ByBlock = 30,
  Default = 31, /**< $LWDEFAULT. */
};

/**
 *  @return The width in millimeters; empty for ByLayer, ByBlock, Default and a code AutoCAD does not define.
 */
[[nodiscard]] DWGIO_API auto MillimetersOf(Lineweight lineweight) -> std::optional<double>;
} // namespace DwgIO

#endif /* DWGIO_LINEWEIGHT_HPP */
