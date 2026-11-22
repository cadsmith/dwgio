/***********************************************************************************************************************
 *  @file      dwgio/objects/tablestyles/format.hpp
 *  @brief     How a cell's content is formatted, from R2010.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_TABLESTYLES_FORMAT_HPP
#define DWGIO_OBJECTS_TABLESTYLES_FORMAT_HPP

#include "dwgio/api.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"

#include <string>

namespace DwgIO::Objects::TableStyles
{
/**
 *  How a cell's content is formatted, from R2010; a cell style extends it.
 */
struct DWGIO_API ContentFormat
{
  /**
   *  Which properties the format overrides: 1 the data type, 2 the data format, 4 the rotation, 8 the block scale, 16
   *  the alignment, 32 the color, 64 the text style, 128 the text height, 256 the auto scale; a cell style's own from
   *  512. Group 90.
   */
  int Overrides{};
  int Properties{}; /**< The property bits, of which only 256, the auto scale, is used. Group 91.                  */
  int Type{512};    /**< Such as 512 for general.                                                                  */
  int Unit{};       /**< The unit of the value: 0 none, 1 a distance, 2 an angle, 4 an area, 8 a volume. Group 93. */
  std::string Format{};
  double Rotation{}; /**< In radians.          */
  double Scale{1};   /**< For a block content. */
  /**
   *  Where the content stands in the cell: 1 to 3 top left to top right, 4 to 6 middle, 7 to 9 bottom. Group 94.
   */
  int Alignment{1};
  DwgIO::Color Color{.Index = 0};
  Handle Style{};
  double Height{};

  auto operator==(const ContentFormat &) const -> bool = default;
};
} // namespace DwgIO::Objects::TableStyles

#endif /* DWGIO_OBJECTS_TABLESTYLES_FORMAT_HPP */
