/***********************************************************************************************************************
 *  @file      source/edit/essential.hpp
 *  @brief     The records a drawing cannot do without, which no edit removes or renames.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_ESSENTIAL_HPP
#define DWGIO_EDIT_ESSENTIAL_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/object.hpp"

namespace DwgIO::Edit
{
/**
 *  Whether a record is one AutoCAD will not open a drawing without, or will not let go of: layer 0, the ByBlock,
 *  ByLayer and Continuous linetypes, the Standard text and dimension styles, the ACAD application, *Active, a space's
 *  block.
 */
[[nodiscard]] auto IsRequired(const Drawing &drawing, const Object &object) -> bool;
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_ESSENTIAL_HPP */
