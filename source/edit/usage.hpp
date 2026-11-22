/***********************************************************************************************************************
 *  @file      source/edit/usage.hpp
 *  @brief     What still needs an object a removal would take.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_USAGE_HPP
#define DWGIO_EDIT_USAGE_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"

#include <cstdint>
#include <unordered_set>

namespace DwgIO::Edit
{
/**
 *  One of the handles that the header or an object outside them still names as a removal cannot follow: a record an
 *  entity, a record or the header uses, an application of extended data, a block an insert, a dimension or an arrow
 *  draws, a layout its block record names, a dictionary's default, a viewport's clip; null when none is named.
 */
[[nodiscard]] auto FindUsed(Drawing &drawing, const std::unordered_set<std::uint64_t> &removed) -> Handle;
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_USAGE_HPP */
