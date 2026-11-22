/***********************************************************************************************************************
 *  @file      dwgio/entities/dimension.hpp
 *  @brief     The DIMENSION entities: the seven kinds, ARC_DIMENSION and LARGE_RADIAL_DIMENSION.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_DIMENSION_HPP
#define DWGIO_ENTITIES_DIMENSION_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <string>
#include <variant>

namespace DwgIO::Entities::Forms
{
/**
 *  An ordinate dimension: the X or Y distance of a feature from the origin.
 */
struct DWGIO_API Ordinate
{
  Vector Feature{}; /**< In world coordinates. Groups 13, 23, 33.                   */
  Vector Leader{};  /**< The leader's end, in world coordinates. Groups 14, 24, 34. */

  auto operator==(const Ordinate &) const -> bool = default;
};

/**
 *  A rotated linear dimension: the distance between two points along a direction.
 */
struct DWGIO_API Linear
{
  Vector Start{};   /**< In world coordinates. Groups 13, 23, 33.                                */
  Vector End{};     /**< In world coordinates. Groups 14, 24, 34.                                */
  double Oblique{}; /**< The extension lines' oblique angle, in radians; group 52 gives degrees. */
  double Angle{};   /**< The direction measured along, in radians; group 50 gives degrees.       */

  auto operator==(const Linear &) const -> bool = default;
};

/**
 *  An aligned dimension: the distance between two points, along the line through them.
 */
struct DWGIO_API Aligned
{
  Vector Start{};   /**< In world coordinates. Groups 13, 23, 33.                                */
  Vector End{};     /**< In world coordinates. Groups 14, 24, 34.                                */
  double Oblique{}; /**< The extension lines' oblique angle, in radians; group 52 gives degrees. */

  auto operator==(const Aligned &) const -> bool = default;
};

/**
 *  An angular dimension by three points: the vertex of the angle and a point on each side.
 */
struct DWGIO_API Angular3Point
{
  Vector Start{};  /**< On the first side, in world coordinates. Groups 13, 23, 33.  */
  Vector End{};    /**< On the second side, in world coordinates. Groups 14, 24, 34. */
  Vector Vertex{}; /**< In world coordinates. Groups 15, 25, 35.                     */

  auto operator==(const Angular3Point &) const -> bool = default;
};

/**
 *  An angular dimension between two lines.
 */
struct DWGIO_API Angular2Line
{
  Vector FirstStart{}; /**< Of the first line, in world coordinates. Groups 13, 23, 33. */
  Vector FirstEnd{};   /**< Of the first line, in world coordinates. Groups 14, 24, 34. */
  /**
   *  The start of the second line, which the definition point ends, in world coordinates. Groups 15, 25, 35.
   */
  Vector SecondStart{};
  /**
   *  A point on the dimension arc, in the dimension's own coordinates, its Z the elevation. Groups 16, 26, 36.
   */
  Vector ArcPoint{};

  auto operator==(const Angular2Line &) const -> bool = default;
};

/**
 *  A radius dimension of a circle or an arc, whose center is the definition point.
 */
struct DWGIO_API Radial
{
  Vector Chord{}; /**< The point on the curve, in world coordinates. Groups 15, 25, 35. */
  double Leader{};

  auto operator==(const Radial &) const -> bool = default;
};

struct DWGIO_API Diameter
{
  /**
   *  The point on the curve across from the definition point, in world coordinates. Groups 15, 25, 35.
   */
  Vector Chord{};
  double Leader{};

  auto operator==(const Diameter &) const -> bool = default;
};

/**
 *  The kind of a dimension with the points that kind adds, in the order of the file's types 20 to 26. The low three
 *  bits of group 70 number them 6, 0, 1, 5, 2, 4 and 3.
 */
using Form = std::variant<Ordinate, Linear, Aligned, Angular3Point, Angular2Line, Radial, Diameter>;
} // namespace DwgIO::Entities::Forms

namespace DwgIO::Entities
{
/**
 *  What every kind of dimension shares: its text, its plane and its block's placing.
 */
struct DWGIO_API Measure
{
  int Version{}; /**< From R2010; 0 for R2010. Group 280. */
  Vector Normal{Up};
  /**
   *  The text's middle, in the dimension's own coordinates, its Z the elevation. Groups 11, 21, 31.
   */
  Vector TextMiddle{};
  /**
   *  The 32, 64 and 128 bits of group 70: 32 a block of this dimension alone, 64 an ordinate along X, 128 text moved by
   *  the user. The kind's bits come from Form.
   */
  int Flags{};
  /**
   *  Unnamed in the specification: the first flag byte beyond its two named bits; 8 as AutoCAD writes it.
   */
  int Unknown1{8};
  /**
   *  The text the user gave, where <> stands for the measurement; empty shows the measurement. Group 1.
   */
  std::string Text{};
  double TextRotation{};        /**< Of the text away from its default direction, in radians; group 53 gives degrees. */
  double HorizontalDirection{}; /**< Of the text and lines, in radians; group 51 gives degrees.                       */
  /**
   *  The scale the dimension's block is inserted at; DXF does not write it.
   */
  Vector Scale{.X = 1, .Y = 1, .Z = 1};
  double BlockRotation{};      /**< The block's rotation, in radians; group 54 gives degrees.          */
  int Attachment{5};           /**< As an MTEXT's: 1 top left to 9 bottom right. Group 71, from R2000. */
  int LineSpacingStyle{1};     /**< 1 at least, 2 exactly. Group 72, from R2000.                       */
  double LineSpacingFactor{1}; /**< A factor of the default line spacing. Group 41, from R2000.        */
  double Measured{-1};         /**< As AutoCAD last computed it. Group 42, from R2000.                 */
  bool Unknown2{};             /**< Unnamed in the specification. Group 73, from R2007.                */
  bool FlipFirst{};            /**< From R2007. Group 74.                                              */
  bool FlipSecond{};           /**< From R2007. Group 75.                                              */
  /**
   *  Where clones of a baseline or continued dimension are inserted, in the dimension's own coordinates, its Z the
   *  elevation. Groups 12, 22, 32.
   */
  Vector CloneInsertion{};
  /**
   *  The definition point, in world coordinates; what it marks depends on the kind. Groups 10, 20, 30.
   */
  Vector DefinitionPoint{};

  auto operator==(const Measure &) const -> bool = default;
};

struct DWGIO_API Dimension
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Measure Measure{};
  Forms::Form Form{}; /**< The kind and its points, which pick the type the file stores the dimension under. */
  Handle Style{};
  Handle Block{}; /**< The anonymous BLOCK_RECORD that draws the dimension. Group 2. */

  auto operator==(const Dimension &) const -> bool = default;
};

/**
 *  An ARC_DIMENSION: the length along an arc or a polyline's arc segment.
 */
struct DWGIO_API ArcDimension
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Measure Measure{}; /**< Its definition point is a point on the dimension arc. */
  Vector Start{};              /**< In world coordinates. Groups 13, 23, 33.              */
  Vector End{};                /**< In world coordinates. Groups 14, 24, 34.              */
  Vector Center{};             /**< In world coordinates. Groups 15, 25, 35.              */
  bool Partial{};
  double StartAngle{}; /**< In radians. Group 40. */
  double EndAngle{};   /**< In radians. Group 41. */
  bool HasLeader{};
  Vector FirstLeaderPoint{};  /**< In world coordinates. Groups 16, 26, 36. */
  Vector SecondLeaderPoint{}; /**< In world coordinates. Groups 17, 27, 37. */
  Handle Style{};
  Handle Block{}; /**< The anonymous BLOCK_RECORD that draws the dimension. Group 2. */

  auto operator==(const ArcDimension &) const -> bool = default;
};

/**
 *  A LARGE_RADIAL_DIMENSION: a jogged radius dimension, for an arc whose center lies far off.
 */
struct DWGIO_API LargeRadialDimension
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::Measure Measure{}; /**< Its definition point is the curve's center. */
  /**
   *  The point on the curve, in world coordinates. Groups 15, 25, 35, where the specification says 13.
   */
  Vector Chord{};
  double JogAngle{}; /**< Group 40, which AutoCAD writes as 0; the jog angle drawn is the style's DIMJOGANG. */
  /**
   *  The center the dimension is drawn from, in place of the curve's own, in world coordinates. Groups 13, 23, 33,
   *  where the specification says 14.
   */
  Vector Center{};
  /**
   *  The jog point, in world coordinates. Groups 14, 24, 34, where the specification says 15.
   */
  Vector JogPoint{};
  Handle Style{};
  Handle Block{}; /**< The anonymous BLOCK_RECORD that draws the dimension. Group 2. */

  auto operator==(const LargeRadialDimension &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_DIMENSION_HPP */
