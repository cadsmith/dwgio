/***********************************************************************************************************************
 *  @file      dwgio/objects/material.hpp
 *  @brief     The MATERIAL object: how surfaces are rendered.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_MATERIAL_HPP
#define DWGIO_OBJECTS_MATERIAL_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/matrix.hpp"

#include <cstdint>
#include <string>

namespace DwgIO::Objects::Materials
{
/**
 *  One color of a material: its ambient, diffuse or specular color.
 */
struct DWGIO_API MaterialColor
{
  /**
   *  0 to take the color of the object, 1 to override it with Color, the one method that stores the color. Group 70, 71
   *  or 76 by shade.
   */
  int Method{};
  double Factor{1}; /**< From 0 to 1. Group 40, 41 or 45 by shade. */
  /**
   *  Packed into one integer as AutoCAD does; black by default. Group 90, 91 or 92 by shade.
   */
  std::int32_t Color{-1040187392};

  auto operator==(const MaterialColor &) const -> bool = default;
};

/**
 *  One map of a material: the image laid over a channel and how it is projected.
 *
 *  Its groups depend on the map; each member gives them in the order diffuse, specular, reflection, opacity, bump,
 *  refraction.
 */
struct DWGIO_API Map
{
  double Blend{1}; /**< From 0 to 1. Group 42, 46, 48, 141, 143 or 146. */
  /**
   *  Where the map comes from: 0 the current scene, 1 an image file, the one source that stores a file. Group 72, 77,
   *  171, 175, 179 or 273.
   */
  int Source{1};
  std::string File{}; /**< Empty for none. Group 3, 4, 6, 7, 8 or 9.                                  */
  int Projection{2};  /**< 1 planar, 2 box, 3 cylinder, 4 sphere. Group 73, 78, 172, 176, 270 or 274. */
  int Tiling{1};      /**< 1 tile, 2 crop, 3 clamp. Group 74, 79, 173, 177, 271 or 275.               */
  /**
   *  Bits: 1 for no automatic transform, 2 to fit the map to the object's extents, 4 to add the block's transform.
   *  Group 75, 170, 174, 178, 272 or 276.
   */
  int Automatic{1};
  Matrix Transform{Identity}; /**< Group 43, 47, 49, 142, 144 or 147. */

  auto operator==(const Map &) const -> bool = default;
};

struct DWGIO_API Maps
{
  Map Diffuse{};
  Map Specular{};
  Map Reflection{};
  Map Opacity{};
  Map Bump{};
  /**
   *  Takes the current scene unless told otherwise.
   */
  Map Refraction{
      .Blend = 1, .Source = 0, .File = {}, .Projection = 2, .Tiling = 1, .Automatic = 1, .Transform = Identity};

  auto operator==(const Maps &) const -> bool = default;
};
} // namespace DwgIO::Objects::Materials

namespace DwgIO::Objects
{
/**
 *  A MATERIAL: how the surfaces of the objects it is given to are rendered, stored under its class.
 */
struct DWGIO_API Material
{
  DwgIO::Base Base{};
  std::string Name{}; /**< Such as "Global". */
  std::string Description{};
  Materials::MaterialColor Ambient{};  /**< In ambient light. Groups 70, 40, 90.   */
  Materials::MaterialColor Diffuse{};  /**< Under direct light. Groups 71, 41, 91. */
  double Gloss{0.5};                   /**< From 0 to 1. Group 44.                 */
  Materials::MaterialColor Specular{}; /**< Of highlights. Groups 76, 45, 92.      */
  double Opacity{1};                   /**< From 0 to 1. Group 140.                */
  double Refraction{1};                /**< The index of refraction. Group 145.    */
  Materials::Maps Maps{};
  double Translucence{}; /**< From 0 to 1, from R2007. Group 148.                         */
  double Illumination{}; /**< Light the material gives off itself, from R2007. Group 149. */
  double Reflectivity{}; /**< From R2007. Group 468.                                      */
  std::int32_t Model{};  /**< From R2007. Group 93.                                       */
  /**
   *  Bits for the channels in use, one per map in the order of Maps; 63 for all six, as AutoCAD writes it. From R2007.
   *  Group 94.
   */
  std::int32_t Channels{63};
  int Mode{}; /**< From R2007. Group 282. */

  auto operator==(const Material &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_MATERIAL_HPP */
