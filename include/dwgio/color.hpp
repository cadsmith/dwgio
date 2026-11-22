/***********************************************************************************************************************
 *  @file      dwgio/color.hpp
 *  @brief     A color as a DWG stores it: palette index, true color, names and transparency.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_COLOR_HPP
#define DWGIO_COLOR_HPP

#include "dwgio/api.hpp"
#include "dwgio/boxed.hpp"
#include "dwgio/handle.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace DwgIO
{
/**
 *  A color's names, from R2004, each when the file gives it.
 */
struct DWGIO_API ColorNames
{
  std::optional<std::string> Name{};
  std::optional<std::string> Book{};

  auto operator==(const ColorNames &) const -> bool = default;
};

/**
 *  A color as a DWG stores it: an index into the AutoCAD palette, and from R2004 the true color, the names and the
 *  transparency a file may add.
 */
struct DWGIO_API Color
{
  /**
   *  The AutoCAD color index: 0 ByBlock, 256 ByLayer, 1 to 255 a palette entry; the flags stripped from R2004.
   */
  int Index{256};
  /**
   *  From R2004, the four bytes of the true color as stored: red, green and blue in the low three, the kind above.
   */
  std::optional<int> Rgb{};
  /**
   *  From R2004, the four bytes of an entity's transparency as stored: the kind in the top byte, the value below.
   */
  std::optional<int> Transparency{};
  /**
   *  From R2004, the AcDbColor object an entity's color refers to, a hard pointer in the entity's handle stream,
   *  flagged 0x4000 in the color; null when the file flags one and names none.
   */
  std::optional<Handle> Reference{};
  /**
   *  From R2004, when the file gives either; boxed since few colors have them and every entity carries a color.
   */
  std::optional<Boxed<ColorNames>> Names{};

  auto operator==(const Color &) const -> bool = default;
};

/**
 *  How a color is given, as the top byte of a true color stores it.
 */
enum class ColorMethod : std::uint8_t
{
  ByLayer = 0xC0,
  ByBlock = 0xC1,
  Rgb = 0xC2,
  Index = 0xC3,      /**< An entry of the AutoCAD palette.                           */
  Foreground = 0xC5, /**< Drawn white on a dark background and black on a light one. */
  None = 0xC8,       /**< No color, as a visual style's property may say.            */
};

/**
 *  How a transparency is given, as its top byte stores it.
 */
enum class TransparencyMethod : std::uint8_t
{
  ByLayer = 0,
  ByBlock = 1,
  Alpha = 2, /**< The low byte holds the alpha: 0 clear, 255 opaque. */
};

/**
 *  @return The method of the true color when there is one; otherwise ByBlock for index 0, ByLayer for 256 and Index for
 *          the rest.
 */
[[nodiscard]] DWGIO_API auto MethodOf(const Color &color) -> ColorMethod;

/**
 *  @return Red, green and blue as 0xRRGGBB, for a color whose method is Rgb; empty for any other.
 */
[[nodiscard]] DWGIO_API auto RgbOf(const Color &color) -> std::optional<std::uint32_t>;

/**
 *  @return Empty for a color without a transparency of its own.
 */
[[nodiscard]] DWGIO_API auto TransparencyMethodOf(const Color &color) -> std::optional<TransparencyMethod>;

/**
 *  @return The alpha, 0 clear to 255 opaque, for a transparency whose method is Alpha; empty for any other.
 */
[[nodiscard]] DWGIO_API auto AlphaOf(const Color &color) -> std::optional<std::uint8_t>;
} // namespace DwgIO

namespace DwgIO::Colors
{
/**
 *  Index 256: the entity takes the color of its layer.
 */
inline constexpr Color ByLayer{.Index = 256};

/**
 *  Index 0: the entity takes the color of the block insert it is drawn in.
 */
inline constexpr Color ByBlock{.Index = 0};
} // namespace DwgIO::Colors

#endif /* DWGIO_COLOR_HPP */
