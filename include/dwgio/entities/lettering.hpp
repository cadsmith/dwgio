/***********************************************************************************************************************
 *  @file      dwgio/entities/lettering.hpp
 *  @brief     The line of text and its placing that TEXT, ATTDEF and ATTRIB share.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_LETTERING_HPP
#define DWGIO_ENTITIES_LETTERING_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <cstdint>
#include <string>

namespace DwgIO::Entities
{
/**
 *  How a line of text lies along its alignment point, group 72.
 */
enum class HorizontalAlignment : std::int16_t // NOLINT(performance-enum-size): a 16-bit field, any value round-trips
{
  Left,
  Center,
  Right,
  Aligned, /**< Between the position and the alignment point, its height scaled with its width. */
  Middle,  /**< Centered both ways on the alignment point.                                      */
  Fit,     /**< Between the position and the alignment point, its height kept.                  */
};

/**
 *  How a line of text lies across its alignment point: group 73 of a TEXT, 74 of an attribute.
 */
enum class VerticalAlignment : std::int16_t // NOLINT(performance-enum-size): a 16-bit field, any value round-trips
{
  Baseline,
  Bottom,
  Middle,
  Top,
};

/**
 *  One line of text and its placing: what a TEXT, an ATTDEF and an ATTRIB share.
 */
struct DWGIO_API Lettering
{
  double Thickness{};
  Vector Position{}; /**< In object coordinates; its Z is the elevation. */
  /**
   *  The point the text is aligned on when it is not left on its baseline; otherwise the position again, whatever
   *  placeholder an R14 file kept. Its Z is the elevation.
   */
  Vector Alignment{};
  double Height{};
  std::string Value{};
  double Rotation{}; /**< In radians.                                                            */
  double Width{1};   /**< The text's own width factor, as the style's is for new text. Group 41. */
  double Oblique{};  /**< In radians.                                                            */
  Handle Style{};
  int Generation{}; /**< 2 backwards, 4 upside down. Group 71. */
  HorizontalAlignment Horizontal{};
  VerticalAlignment Vertical{};
  Vector Extrusion{Up};

  auto operator==(const Lettering &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_LETTERING_HPP */
