/***********************************************************************************************************************
 *  @file      source/drawing.cpp
 *  @brief     Finding an object of a drawing by its handle.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/drawing.hpp"

#include <algorithm>

namespace DwgIO
{
auto Find(const Drawing &drawing, Handle self) -> const Object *
{
  if (self == Handle{})
  {
    return nullptr;
  }
  const auto found =
      std::ranges::find(drawing.Objects, self, [](const Object &one) -> Handle { return BaseOf(one).Self; });
  return found == drawing.Objects.end() ? nullptr : &*found;
}

auto Find(Drawing &drawing, Handle self) -> Object *
{
  if (self == Handle{})
  {
    return nullptr;
  }
  const auto found =
      std::ranges::find(drawing.Objects, self, [](const Object &one) -> Handle { return BaseOf(one).Self; });
  return found == drawing.Objects.end() ? nullptr : &*found;
}
} // namespace DwgIO
