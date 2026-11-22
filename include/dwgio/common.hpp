/***********************************************************************************************************************
 *  @file      dwgio/common.hpp
 *  @brief     The common entity data and handles every entity carries.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_COMMON_HPP
#define DWGIO_COMMON_HPP

#include "dwgio/api.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/lineweight.hpp"
#include "dwgio/origin.hpp"
#include "dwgio/space.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace DwgIO
{
/**
 *  What every entity carries besides its base: the common entity data and its handles.
 */
struct DWGIO_API Common
{
  DwgIO::Space Space{DwgIO::Space::Owner};
  /**
   *  From R2000; empty before, which a save as R2000 writes as ByLayer.
   */
  std::optional<DwgIO::Lineweight> Lineweight{};
  Origin LinetypeOrigin{};
  Origin PlotstyleOrigin{}; /**< From R2000; never Continuous. */
  Origin MaterialOrigin{};  /**< From R2007; never Continuous. */
  /**
   *  From R2007, the shadow flags as the file stores them; 3 when a shadow handle follows.
   */
  int ShadowFlags{};
  DwgIO::Color Color{};
  double LinetypeScale{1};
  Handle Layer{};
  Handle Linetype{};  /**< When LinetypeOrigin is Record; null otherwise.                              */
  Handle Plotstyle{}; /**< When PlotstyleOrigin is Record; null otherwise.                             */
  Handle Material{};  /**< When MaterialOrigin is Record; null otherwise.                              */
  Handle Shadow{};    /**< From R2007, when ShadowFlags is 3; null as AutoCAD writes it.               */
  Handle FullStyle{}; /**< From R2010, the visual style the entity is drawn with whole; null for none. */
  Handle FaceStyle{}; /**< From R2010, the visual style of its faces; null for none.                   */
  Handle EdgeStyle{}; /**< From R2010, the visual style of its edges; null for none.                   */
  /**
   *  Up to R2000, the entity before this one in its space; null for the first.
   *
   *  The file leaves the links out when they are the handles next to the entity's own, and the reader fills them in.
   */
  Handle Previous{};
  Handle Next{}; /**< Up to R2000, the entity after this one in its space; null for the last. */
  /**
   *  The proxy graphics the entity carries, as the file has them; empty for none.
   */
  std::vector<std::byte> Graphics{};
  bool Invisible{};

  auto operator==(const Common &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_COMMON_HPP */
