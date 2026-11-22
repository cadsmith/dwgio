/***********************************************************************************************************************
 *  @file      dwgio/entities/mline.hpp
 *  @brief     The MLINE entity: a multiline and how each of its lines runs.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_MLINE_HPP
#define DWGIO_ENTITIES_MLINE_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <vector>

namespace DwgIO::Entities
{
/**
 *  How one line of a multiline runs along a segment: where its pieces and its fill lie.
 */
struct DWGIO_API Strand
{
  /**
   *  The distance along the miter to where the element starts, the distance along the segment to where it starts, then
   *  the breaks, each a start and an end. Group 41.
   */
  std::vector<double> Spans{};
  std::vector<double> Fills{}; /**< Parameters along the segment. Group 42. */

  auto operator==(const Strand &) const -> bool = default;
};

/**
 *  A vertex of a multiline, with the segment that starts at it.
 */
struct DWGIO_API Joint
{
  Vector Position{};  /**< In world coordinates.                     */
  Vector Direction{}; /**< Of the segment that starts at the vertex. */
  Vector Miter{};
  std::vector<Strand> Strands{}; /**< One per line of the style, in its order: as many as Lines says. */

  auto operator==(const Joint &) const -> bool = default;
};

struct DWGIO_API MLine
{
  DwgIO::Base Base{};
  DwgIO::Common Common{};
  double Scale{1};     /**< Scales the style's offsets. Group 40. */
  int Justification{}; /**< 0 top, 1 zero, 2 bottom. Group 70.    */
  Vector Start{};      /**< In world coordinates.                 */
  Vector Normal{Up};
  int Flags{1}; /**< 1 has vertices, 2 closed, 4 start caps suppressed, 8 end caps suppressed. Group 71. */
  int Lines{};  /**< The number of lines in the style: the strands every joint carries. Group 73.        */
  std::vector<Joint> Joints{};
  Handle Style{};

  auto operator==(const MLine &) const -> bool = default;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_MLINE_HPP */
