/***********************************************************************************************************************
 *  @file      source/releases/omission.hpp
 *  @brief     The objects a save to another release leaves out, and the pointers to them it clears.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_OMISSION_HPP
#define DWGIO_RELEASES_OMISSION_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <span>
#include <string>
#include <vector>

namespace DwgIO::Releases
{
/**
 *  Clears the pointer when it names an object left out.
 */
[[nodiscard]] auto ClearLeftOut(Handle &pointer, std::span<const Handle> leftOut) -> bool;

/**
 *  Sorted: the objects BindingOf ties to the drawing's release, the untyped ones and the typed ones whose layout the
 *  release does not share, and the ACIS entities whose body cannot cross to it.
 */
[[nodiscard]] auto CollectLeftOut(const Drawing &drawing, Version to) -> std::vector<Handle>;

/**
 *  What a save to that release says of an object CollectLeftOut lists.
 */
[[nodiscard]] auto Refusal(const DwgIO::Object &object, const Drawing &drawing, Version to) -> std::string;

/**
 *  Clears the pointers R2007 and later added where they name an object left out, each noted: the visual styles,
 *  backgrounds, suns and shade plots of views, viewport records, viewports and layouts, a view's live section, the
 *  materials and visual styles of layers and entities, a section's settings; an entity is left with its layer's
 *  material. AutoCAD refuses a file whose view names a missing visual style.
 */
void ClearPointers(DwgIO::Object &object, std::span<const Handle> leftOut, std::vector<Issue> &issues);
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_OMISSION_HPP */
