/***********************************************************************************************************************
 *  @file      source/classes/catalog.hpp
 *  @brief     The classes AutoCAD declares, and those a save must add.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_CLASSES_CATALOG_HPP
#define DWGIO_CLASSES_CATALOG_HPP

#include "dwgio/class.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/version.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace DwgIO::Classes
{
/**
 *  The class AutoCAD declares for a DXF name, numbered 0 for the caller to number, or none for a name no table knows.
 */
[[nodiscard]] auto FindDeclared(std::string_view dxf) -> std::optional<Class>;

/**
 *  The classes the drawing's objects are stored under in a release that the drawing does not declare, as AutoCAD
 *  declares them, numbered after the drawing's; a name no table knows is left out.
 */
[[nodiscard]] auto CollectNeeded(const Drawing &drawing, Version release) -> std::vector<Class>;
/**
 *  Fills what AutoCAD writes in a class record from R2004 where a record has none: the instances counted among the
 *  drawing's objects, whatever type they are stored under, and for a class in use the ObjectDBX version 22 and
 *  maintenance 42 its own core classes carry; a record that carries them keeps them.
 */
void FillCounts(std::vector<Class> &classes, const Drawing &drawing);
} // namespace DwgIO::Classes

#endif /* DWGIO_CLASSES_CATALOG_HPP */
