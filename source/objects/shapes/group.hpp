/***********************************************************************************************************************
 *  @file      source/objects/shapes/group.hpp
 *  @brief     How a GROUP lies in the file, chapter 20.4.72.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_GROUP_HPP
#define DWGIO_OBJECTS_SHAPES_GROUP_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/group.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Objects::Group>
{
  static constexpr std::array<std::uint16_t, 1> Types{72};
  static constexpr std::string_view Name = "GROUP";
  static constexpr int HardPointer = 5;

  static auto Layout()
  {
    using Objects::Group;
    return Fields::Layout(Fields::Text(&Group::Description), Fields::Short(&Group::Unnamed),
                          Fields::Short(&Group::Selectable),
                          Fields::List(&Group::Members, Fields::Counted::Long, Fields::Link(HardPointer)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_GROUP_HPP */
