/***********************************************************************************************************************
 *  @file      source/encoding/units.cpp
 *  @brief     UTF-16 code units to UTF-8 and back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/units.hpp"

#include "encoding/points.hpp"

#include <cstddef>

namespace DwgIO::Encoding
{
namespace
{
constexpr char32_t High = 0xD800;

constexpr char32_t Low = 0xDC00;

constexpr char32_t Past = 0xE000;

constexpr char32_t Plane = 0x10000;

constexpr char32_t Replacement = 0xFFFD;

constexpr unsigned SurrogateShift = 10;

constexpr char32_t Mask = 0x3FF;

auto Surrogate(char32_t unit) -> bool
{
  return unit >= High && unit < Past;
}
} // namespace

auto FromUnits(std::u16string_view units) -> std::string
{
  std::string out;
  out.reserve(units.size());
  for (std::size_t at = 0; at < units.size(); ++at)
  {
    const char32_t unit = units[at];
    if (!Surrogate(unit))
    {
      Put(out, unit);
      continue;
    }
    const char32_t next = at + 1 < units.size() ? char32_t{units[at + 1]} : 0;
    if (unit < Low && next >= Low && next < Past)
    {
      Put(out, Plane + ((unit - High) << SurrogateShift) + (next - Low));
      ++at;
    }
    else
    {
      Put(out, Replacement);
    }
  }
  return out;
}

auto ToUnits(std::string_view text) -> std::u16string
{
  std::u16string out;
  out.reserve(text.size());
  while (!text.empty())
  {
    const Step one = First(text);
    if (one.Point < Plane)
    {
      out.push_back(static_cast<char16_t>(one.Point));
    }
    else
    {
      const char32_t above = one.Point - Plane;
      out.push_back(static_cast<char16_t>(High + (above >> SurrogateShift)));
      out.push_back(static_cast<char16_t>(Low + (above & Mask)));
    }
    text.remove_prefix(one.Length);
  }
  return out;
}
} // namespace DwgIO::Encoding
