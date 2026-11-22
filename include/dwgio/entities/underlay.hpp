/***********************************************************************************************************************
 *  @file      dwgio/entities/underlay.hpp
 *  @brief     The PDFUNDERLAY, DWFUNDERLAY and DGNUNDERLAY entities.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_UNDERLAY_HPP
#define DWGIO_ENTITIES_UNDERLAY_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/entities/overlay.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <vector>

namespace DwgIO::Entities
{
struct DWGIO_API Bound
{
  Vector Position{}; /**< In the entity's own coordinate system. */

  auto operator==(const Bound &) const -> bool = default;
};

/**
 *  Where an underlay lies, how it shows and how it is clipped: the AcDbUnderlayReference subclass.
 */
struct DWGIO_API UnderlayData
{
  Handle Definition{}; /**< The definition object that names the file. */
  Vector Position{};   /**< In world coordinates.                      */
  Vector Scale{.X = 1, .Y = 1, .Z = 1};
  double Rotation{}; /**< In radians; group 50 gives degrees. */
  Vector Extrusion{Up};
  int Flags{2};              /**< 1 clipped, 2 on, 4 monochrome, 8 adjusted for the background. Group 280. */
  int Contrast{100};         /**< 20 to 100. Group 281.                                                    */
  int Fade{};                /**< 0 to 80. Group 282.                                                      */
  std::vector<Bound> Clip{}; /**< Two opposite corners for a rectangle.                                    */

  auto operator==(const UnderlayData &) const -> bool = default;
};

/**
 *  An underlay of the given kind: a PDF, DWF or DGN file shown under the drawing.
 */
template <UnderlayKind KindV>
struct UnderlayOf
{
  static constexpr UnderlayKind Shows = KindV;

  DwgIO::Base Base{};
  DwgIO::Common Common{};
  Entities::UnderlayData Data{};

  auto operator==(const UnderlayOf &) const -> bool = default;
};

using PdfUnderlay = UnderlayOf<UnderlayKind::Pdf>;
using DwfUnderlay = UnderlayOf<UnderlayKind::Dwf>;
using DgnUnderlay = UnderlayOf<UnderlayKind::Dgn>;
} // namespace DwgIO::Entities

#endif /* DWGIO_ENTITIES_UNDERLAY_HPP */
