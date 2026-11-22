/***********************************************************************************************************************
 *  @file      dwgio/entities/leader.hpp
 *  @brief     The LEADER entity.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_LEADER_HPP
#define DWGIO_ENTITIES_LEADER_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <vector>

namespace DwgIO::Entities
{
struct DWGIO_API Leader
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  bool Unknown1{}; /**< Unnamed in the specification; 0 as AutoCAD writes it. */
  /**
   *  What the leader was made with: 0 an MTEXT, 1 a TOLERANCE, 2 an INSERT, 3 no annotation. Group 73.
   */
  int Annotation{3};
  int Path{};                   /**< 0 straight segments, 1 a spline. Group 72.                                   */
  std::vector<Vector> Points{}; /**< In world coordinates.                                                        */
  Vector Origin{};              /**< The origin of the leader's plane, the first vertex unless moved; not in DXF. */
  Vector Normal{Up};
  Vector Horizontal{Right};
  /**
   *  The offset of the last vertex from the insertion point of the block it annotates. Groups 212, 222, 232.
   */
  Vector Block{};
  /**
   *  The offset of the last vertex from where the annotation is placed, from R14. Groups 213, 223, 233.
   */
  Vector Offset{};
  double Gap{};    /**< DIMGAP times DIMSCALE when the leader was made; up to R14. */
  double Height{}; /**< Of the text annotation, up to R2007. Group 40.             */
  double Width{};  /**< Of the text annotation, up to R2007. Group 41.             */
  /**
   *  Whether the hook line runs along the horizontal direction rather than against it. Group 74.
   */
  bool Direction{true};
  bool Arrow{true};
  int Arrowhead{};     /**< Up to R14.                                                                        */
  double Dimasz{};     /**< DIMASZ times DIMSCALE when the leader was made; up to R14.                        */
  bool Unknown2{};     /**< Unnamed in the specification. Up to R14.                                          */
  bool Unknown3{};     /**< Unnamed in the specification. Up to R14.                                          */
  int RepeatedColor{}; /**< Up to R14: Color again, as AutoCAD writes it.                                     */
  int Color{};         /**< The color of the leader when its style's DIMCLRD is ByBlock; up to R14. Group 77. */
  bool Unknown5{};     /**< Unnamed in the specification. Up to R14.                                          */
  bool Unknown6{};     /**< Unnamed in the specification. Up to R14.                                          */
  int ArrowheadType{}; /**< From R2000: the arrowhead Arrowhead holds up to R14.                              */
  bool Unknown8{};     /**< Unnamed in the specification. From R2000.                                         */
  bool Unknown9{};     /**< Unnamed in the specification. From R2000.                                         */
  Handle Note{};       /**< An MTEXT, a TOLERANCE or an INSERT; null without one. Group 340.                  */
  Handle Style{};

  auto operator==(const Leader &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_LEADER_HPP */
