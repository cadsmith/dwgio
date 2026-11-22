/***********************************************************************************************************************
 *  @file      source/entities/shapes/count.hpp
 *  @brief     The counts a list of an entity is read with, checked against the bits left.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_COUNT_HPP
#define DWGIO_ENTITIES_SHAPES_COUNT_HPP

#include "fields/context.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace DwgIO::Entities::Shapes
{
/**
 *  Each element takes a bit at least; a count the data cannot hold fails the reader and gives nothing.
 */
inline auto CheckCount(std::int64_t count, const Fields::Input &in) -> std::optional<std::size_t>
{
  const std::size_t room = in.In->Size() - in.In->Position();
  if (in.In->HasFailed() || count < 0 || std::cmp_greater(count, room))
  {
    in.In->Seek(in.In->Size() + 1);
    return std::nullopt;
  }
  return static_cast<std::size_t>(count);
}

inline auto ReadCount(const Fields::Input &in) -> std::optional<std::size_t>
{
  return CheckCount(in.In->BL(), in);
}

inline auto ReadShortCount(const Fields::Input &in) -> std::optional<std::size_t>
{
  return CheckCount(in.In->BS(), in);
}
} // namespace DwgIO::Entities::Shapes

#endif /* DWGIO_ENTITIES_SHAPES_COUNT_HPP */
