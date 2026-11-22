/***********************************************************************************************************************
 *  @file      dwgio/appearance.hpp
 *  @brief     The layer, color, linetype and lineweight an entity is drawn with, ByLayer and ByBlock resolved.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_APPEARANCE_HPP
#define DWGIO_APPEARANCE_HPP

#include "dwgio/api.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/index.hpp"
#include "dwgio/lineweight.hpp"

namespace DwgIO
{
/**
 *  What an entity is drawn with once ByLayer and ByBlock are followed to what they stand for.
 */
struct DWGIO_API Appearance
{
  Handle Layer{}; /**< The layer it is drawn on: an insert's for an entity on layer 0 inside a block. */
  DwgIO::Color Color{};
  Handle Linetype{}; /**< A linetype record; Continuous for ByBlock outside any block. */
  DwgIO::Lineweight Lineweight{DwgIO::Lineweight::Default};

  auto operator==(const Appearance &) const -> bool = default;
};

/**
 *  How an entity is drawn, as AutoCAD resolves it: ByLayer takes the layer's, ByBlock what the insert around it is
 *  drawn with, and an entity on layer 0 inside a block takes the insert's layer.
 *
 *  Outside any block, ByBlock draws white, index 7, with Continuous at the default lineweight. For a block nested in
 *  another, pass the appearance of the inner insert, itself resolved against the outer.
 *
 *  @param  outer The appearance of the insert the entity is drawn in; null at the top.
 *  @return       The layer's own values where the layer is missing, color 7, Continuous and the default lineweight.
 */
[[nodiscard]] DWGIO_API auto AppearanceOf(const Index &index, const Common &common, const Appearance *outer)
    -> Appearance;
} // namespace DwgIO

#endif /* DWGIO_APPEARANCE_HPP */
