/***********************************************************************************************************************
 *  @file      source/releases/object.hpp
 *  @brief     An object as another release holds it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_OBJECT_HPP
#define DWGIO_RELEASES_OBJECT_HPP

#include "releases/lookup.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <vector>

namespace DwgIO::Releases
{
/**
 *  An object as another release holds it, and what that release could not keep.
 */
struct Translation
{
  DwgIO::Object Object;
  std::vector<Issue> Issues;
};

/**
 *  An entity's linetype goes by the flags R2000 keeps or the record handles R14 keeps, its lineweight as R2000's code
 *  or R14's extended data, its plot style dropped and noted before R2000; a layer's lineweight and plot style are
 *  dropped before R2000, the lineweight noted. An object of the release asked for comes back as it is.
 */
[[nodiscard]] auto ConvertObject(const DwgIO::Object &object, const Drawing &drawing, Version to) -> Translation;

/**
 *  For a pass over many objects, which finds the drawing's objects by handle once.
 */
[[nodiscard]] auto ConvertObject(const DwgIO::Object &object, const Lookup &found, Version to) -> Translation;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_OBJECT_HPP */
