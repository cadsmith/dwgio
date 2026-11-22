/***********************************************************************************************************************
 *  @file      source/releases/binding.hpp
 *  @brief     What binds an object to its drawing's release, so a save to another leaves it out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_RELEASES_BINDING_HPP
#define DWGIO_RELEASES_BINDING_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <string_view>

namespace DwgIO::Releases
{
/**
 *  What binds an object to its drawing's release, so that a save to another leaves it out: "bytes" for an Unknown whose
 *  own bits do not cross to it in the drawing's code page, "overrides" for a mesh whose overrides are kept as bits;
 *  empty when nothing does, as for any object saved to its own release.
 */
[[nodiscard]] auto BindingOf(const Object &object, const Drawing &drawing, Version to) -> std::string_view;
} // namespace DwgIO::Releases

#endif /* DWGIO_RELEASES_BINDING_HPP */
