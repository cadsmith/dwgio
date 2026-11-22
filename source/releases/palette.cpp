/***********************************************************************************************************************
 *  @file      source/releases/palette.cpp
 *  @brief     AutoCAD's color palette, and the index nearest a true color.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/palette.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace DwgIO::Releases
{
namespace
{
/**
 *  Red, green and blue.
 */
using Rgb = std::array<int, 3>;

/**
 *  The first nine indices: red, yellow, green, cyan, blue, magenta, white and two greys.
 */
constexpr std::array<Rgb, 9> Named{{{255, 0, 0},
                                    {255, 255, 0},
                                    {0, 255, 0},
                                    {0, 255, 255},
                                    {0, 0, 255},
                                    {255, 0, 255},
                                    {255, 255, 255},
                                    {128, 128, 128},
                                    {192, 192, 192}}};

/**
 *  The six greys from 250 to 255.
 */
constexpr std::array<int, 6> Greys{51, 91, 132, 173, 214, 255};

/**
 *  The value a channel takes in each of a hue's ten shades, from 10 to 19 and so on, at each of the five steps of the
 *  hue's ramp: a shade of even index has its full brightness, the odd one after it half its saturation.
 */
constexpr std::array<std::array<int, 5>, 10> Shades{{{0, 63, 127, 191, 255},
                                                     {127, 159, 191, 223, 255},
                                                     {0, 51, 102, 153, 204},
                                                     {102, 127, 153, 178, 204},
                                                     {0, 38, 76, 114, 153},
                                                     {76, 95, 114, 133, 153},
                                                     {0, 31, 63, 95, 127},
                                                     {63, 79, 95, 111, 127},
                                                     {0, 19, 38, 57, 76},
                                                     {38, 47, 57, 66, 76}}};

constexpr int Steps = 4;
constexpr int FirstHue = 10;
constexpr int FirstGrey = 250;
constexpr int Last = 255;

/**
 *  The steps of red, green and blue of the hue of a column from 0 to 23, 15 degrees apart from red: each six columns
 *  one channel rises or falls a step at a time.
 */
auto HueOf(int column) -> std::array<int, 3>
{
  const int step = column % Steps;
  switch (column / Steps)
  {
  case 0:
    return {Steps, step, 0};
  case 1:
    return {Steps - step, Steps, 0};
  case 2:
    return {0, Steps, step};
  case 3:
    return {0, Steps - step, Steps};
  case 4:
    return {step, 0, Steps};
  default:
    return {Steps, 0, Steps - step};
  }
}

/**
 *  The color of an index from 1 to 255.
 */
auto ColorOf(int index) -> Rgb
{
  if (index < FirstHue)
  {
    return Named.at(static_cast<std::size_t>(index - 1));
  }
  if (index >= FirstGrey)
  {
    const int grey = Greys.at(static_cast<std::size_t>(index - FirstGrey));
    return {grey, grey, grey};
  }
  const std::array<int, 3> hue = HueOf((index - FirstHue) / FirstHue);
  const auto &shade = Shades.at(static_cast<std::size_t>(index % FirstHue));
  return {shade.at(static_cast<std::size_t>(hue[0])), shade.at(static_cast<std::size_t>(hue[1])),
          shade.at(static_cast<std::size_t>(hue[2]))};
}

/**
 *  The square of the distance between two colors.
 */
auto DistanceOf(const Rgb &one, const Rgb &two) -> int
{
  int sum = 0;
  for (std::size_t at = 0; at < one.size(); ++at)
  {
    sum += (one.at(at) - two.at(at)) * (one.at(at) - two.at(at));
  }
  return sum;
}

/**
 *  The red, green and blue of R2004's long, below the method in its top byte.
 */
constexpr std::uint32_t Mask = 0xFFFFFFU;

constexpr std::uint32_t ByLayer = 0xC0000000U;
constexpr std::uint32_t ByBlock = 0xC1000000U;
constexpr std::uint32_t ByIndex = 0xC3000000U;
constexpr std::uint32_t Method = 0xFF000000U;
constexpr std::uint32_t Low = 0xFFU;
constexpr int ByLayerIndex = 256;
} // namespace

auto NearestIndexOf(std::int32_t rgb) -> int
{
  const auto value = static_cast<std::uint32_t>(rgb);
  const Rgb wanted{static_cast<int>((value >> 16U) & 0xFFU), static_cast<int>((value >> 8U) & 0xFFU),
                   static_cast<int>(value & 0xFFU)};
  int best = 1;
  int least = DistanceOf(wanted, ColorOf(1));
  for (int index = 2; index <= Last; ++index)
  {
    const int distance = DistanceOf(wanted, ColorOf(index));
    if (distance < least)
    {
      least = distance;
      best = index;
    }
  }
  return best;
}

auto LowerColor(const Color &color) -> Color
{
  if (!color.Rgb)
  {
    return color;
  }
  if (*color.Rgb == Unmethodical)
  {
    return Color{.Index = UnmethodicalIndex};
  }
  return Color{.Index = NearestIndexOf(static_cast<std::int32_t>(static_cast<std::uint32_t>(*color.Rgb) & Mask))};
}

auto RaiseColor(const Color &color) -> Color
{
  return color.Index == UnmethodicalIndex ? Color{.Index = 0, .Rgb = Unmethodical} : color;
}

auto LongOf(const Color &color) -> std::int64_t
{
  if (color.Rgb)
  {
    return *color.Rgb;
  }
  std::uint32_t value = ByIndex | (static_cast<std::uint32_t>(color.Index) & Low);
  value = color.Index == ByLayerIndex ? ByLayer : value;
  value = color.Index == 0 ? ByBlock : value;
  return static_cast<std::int32_t>(value);
}

auto FromLong(std::int64_t value) -> Color
{
  const auto bits = static_cast<std::uint32_t>(value);
  switch (bits & Method)
  {
  case ByIndex:
    return Color{.Index = static_cast<int>(bits & Low)};
  case ByLayer:
    return Color{.Index = ByLayerIndex};
  case ByBlock:
    return Color{.Index = 0};
  default:
    return Color{.Index = 0, .Rgb = static_cast<std::int32_t>(bits)};
  }
}
} // namespace DwgIO::Releases
