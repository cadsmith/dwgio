/***********************************************************************************************************************
 *  @file      source/lineweight.cpp
 *  @brief     The width of each lineweight code.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/lineweight.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <utility>

namespace DwgIO
{
namespace
{
/**
 *  The standard widths in hundredths of a millimeter, by code.
 */
constexpr std::array<int, 24> Hundredths{0,  5,  9,  13, 15, 18,  20,  25,  30,  35,  40,  50,
                                         53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, 211};

constexpr double PerMillimeter = 100.0;
} // namespace

auto MillimetersOf(Lineweight lineweight) -> std::optional<double>
{
  const auto code = static_cast<std::size_t>(std::to_underlying(lineweight));
  if (code >= Hundredths.size())
  {
    return std::nullopt;
  }
  return Hundredths.at(code) / PerMillimeter;
}
} // namespace DwgIO
