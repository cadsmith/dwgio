/***********************************************************************************************************************
 *  @file      source/color.cpp
 *  @brief     A color's method, true color and transparency told from the values a DWG stores.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/color.hpp"

#include <cstdint>
#include <optional>

namespace DwgIO
{
namespace
{
constexpr unsigned MethodShift = 24;
constexpr std::uint32_t LowBits = 0xFFFFFF;
constexpr std::uint32_t AlphaBits = 0xFF;
constexpr int ByBlockIndex = 0;
constexpr int ByLayerIndex = 256;

auto TopByte(int value) -> std::uint8_t
{
  return static_cast<std::uint8_t>(static_cast<std::uint32_t>(value) >> MethodShift);
}
} // namespace

auto MethodOf(const Color &color) -> ColorMethod
{
  if (color.Rgb)
  {
    return static_cast<ColorMethod>(TopByte(*color.Rgb));
  }
  if (color.Index == ByBlockIndex)
  {
    return ColorMethod::ByBlock;
  }
  return color.Index == ByLayerIndex ? ColorMethod::ByLayer : ColorMethod::Index;
}

auto RgbOf(const Color &color) -> std::optional<std::uint32_t>
{
  if (!color.Rgb || MethodOf(color) != ColorMethod::Rgb)
  {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(*color.Rgb) & LowBits;
}

auto TransparencyMethodOf(const Color &color) -> std::optional<TransparencyMethod>
{
  if (!color.Transparency)
  {
    return std::nullopt;
  }
  return static_cast<TransparencyMethod>(TopByte(*color.Transparency));
}

auto AlphaOf(const Color &color) -> std::optional<std::uint8_t>
{
  if (!color.Transparency || TransparencyMethodOf(color) != TransparencyMethod::Alpha)
  {
    return std::nullopt;
  }
  return static_cast<std::uint8_t>(static_cast<std::uint32_t>(*color.Transparency) & AlphaBits);
}
} // namespace DwgIO
