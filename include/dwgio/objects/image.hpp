/***********************************************************************************************************************
 *  @file      dwgio/objects/image.hpp
 *  @brief     The IMAGEDEF, IMAGEDEF_REACTOR and RASTERVARIABLES objects behind raster images.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_IMAGE_HPP
#define DWGIO_OBJECTS_IMAGE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/vector.hpp"

#include <string>

namespace DwgIO::Objects
{
/**
 *  An IMAGEDEF: a raster image file the drawing's IMAGE entities show, owned by the ACAD_IMAGE_DICT dictionary; its
 *  reactors are the images' IMAGEDEF_REACTORs.
 */
struct DWGIO_API ImageDef
{
  DwgIO::Base Base{};
  int Version{}; /**< 0 as AutoCAD writes it. Group 90. */
  std::string File{};
  Vector Size{.X = 1, .Y = 1}; /**< In pixels.                                                          */
  Vector Pixel{};              /**< The width and height of one pixel, in drawing units. Groups 11, 21. */
  bool Loaded{};
  int Units{}; /**< The units Pixel is in: 0 none, 2 centimeters, 5 inches. Group 281. */

  auto operator==(const ImageDef &) const -> bool = default;
};

/**
 *  An IMAGEDEF_REACTOR: links an image definition back to an IMAGE that shows it. The image is its owner, which DXF
 *  writes again as the subclass's 330.
 */
struct DWGIO_API ImageDefReactor
{
  DwgIO::Base Base{};
  int Version{2}; /**< 2 as AutoCAD writes it. Group 90. */

  auto operator==(const ImageDefReactor &) const -> bool = default;
};

/**
 *  A RASTERVARIABLES: the ACAD_IMAGE_VARS entry of the root dictionary.
 */
struct DWGIO_API RasterVariables
{
  DwgIO::Base Base{};
  int Version{}; /**< 0 as AutoCAD writes it. Group 90.                    */
  int Frame{};   /**< 1 draws a frame around images, 0 does not. Group 70. */
  int Quality{}; /**< 0 draft, 1 high. Group 71.                           */
  /**
   *  The units images are inserted in: 0 none, 1 millimeters, 2 centimeters, 3 meters, 4 kilometers, 5 inches, 6 feet,
   *  7 yards, 8 miles. Group 72.
   */
  int Units{};

  auto operator==(const RasterVariables &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_IMAGE_HPP */
