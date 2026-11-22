/***********************************************************************************************************************
 *  @file      source/edit/unhook.hpp
 *  @brief     The references to removed objects taken out of what remains.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_UNHOOK_HPP
#define DWGIO_EDIT_UNHOOK_HPP

#include "dwgio/drawing.hpp"

#include <cstdint>
#include <unordered_set>

namespace DwgIO::Edit
{
/**
 *  Takes the handles out of what the objects left name them in without needing them: reactors, extension dictionaries,
 *  groups, draw orders, the dimension style table's second list, a block's inserts, a layout's viewports, a viewport's
 *  frozen layers, a leader's note and the chain of viewport entity headers.
 *
 *  A hatch bound to one of them loses its boundary, as AutoCAD's ERASE leaves it.
 */
void UnhookRemoved(Drawing &drawing, const std::unordered_set<std::uint64_t> &removed);
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_UNHOOK_HPP */
