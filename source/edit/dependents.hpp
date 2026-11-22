/***********************************************************************************************************************
 *  @file      source/edit/dependents.hpp
 *  @brief     What a removal takes along with an object.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_DEPENDENTS_HPP
#define DWGIO_EDIT_DEPENDENTS_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"

#include <cstdint>
#include <unordered_set>

namespace DwgIO::Edit
{
/**
 *  The handles a removal takes: the object and, through each, what it owns, its extension dictionary among them, the
 *  dimension associations reacting to it and, before R2000, a viewport's entity header. Each names an object.
 */
[[nodiscard]] auto CollectRemoved(const Drawing &drawing, Handle self) -> std::unordered_set<std::uint64_t>;
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_DEPENDENTS_HPP */
