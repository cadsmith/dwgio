/***********************************************************************************************************************
 *  @file      dwgio/entities/raster.hpp
 *  @brief     The IMAGE and WIPEOUT entities: a raster image or a hiding area placed in the drawing.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_RASTER_HPP
#define DWGIO_ENTITIES_RASTER_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <vector>

namespace DwgIO::Entities
{
struct DWGIO_API Pixel
{
  /**
   *  In pixels, from the center of the top-left pixel, Y counted down the rows: the world point is Origin + (X + 0.5)
   *  UVector + (Size.Y - 0.5 - Y) VVector.
   */
  Vector Position{};

  auto operator==(const Pixel &) const -> bool = default;
};

/**
 *  What an IMAGE and a WIPEOUT share: where the picture lies, how it shows and how it is clipped.
 */
struct DWGIO_API Raster
{
  int Version{};       /**< 0 as AutoCAD writes it. Group 90.                                */
  Vector Origin{};     /**< The outer corner of the bottom-left pixel, in world coordinates. */
  Vector UVector{};    /**< One pixel's step along the bottom edge, in world coordinates.    */
  Vector VVector{};    /**< One pixel's step up the left edge, in world coordinates.         */
  Vector Size{};       /**< In pixels; 1 by 1 for a wipeout.                                 */
  Handle Definition{}; /**< Null for a wipeout.                                              */
  /**
   *  1 shown, 2 shown when not aligned with the screen, 4 clipped, 8 transparent; 7 for a new one, as AutoCAD writes
   *  it. Group 70.
   */
  int Flags{7};
  bool Clipped{};     /**< Always on for a wipeout. Group 280. */
  int Brightness{50}; /**< 0 to 100. Group 281.                */
  int Contrast{50};   /**< 0 to 100. Group 282.                */
  int Fade{};         /**< 0 to 100. Group 283.                */
  /**
   *  From R2010, whether the clip is inverted, hiding what lies inside the boundary. Group 290.
   */
  bool Outside{};
  Handle Reactor{}; /**< A hard owner; null for a wipeout. */
  /**
   *  The shape of the clipping boundary: 1 a rectangle, as for a new image, 2 a polygon. Group 71.
   */
  int ClipShape{1};
  /**
   *  The vertices of the clipping boundary: two opposite corners for a rectangle, or a polygon's vertices without the
   *  first again at the end, which DXF adds and counts in its group 91.
   */
  std::vector<Pixel> Corners{};

  auto operator==(const Raster &) const -> bool = default;
};

struct DWGIO_API Image
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Raster Raster{};

  auto operator==(const Image &) const -> bool = default;
};

/**
 *  A WIPEOUT: an area that hides what is drawn under it, laid out as an image of one pixel with no definition.
 */
struct DWGIO_API Wipeout
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Raster Raster{};

  auto operator==(const Wipeout &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_RASTER_HPP */
