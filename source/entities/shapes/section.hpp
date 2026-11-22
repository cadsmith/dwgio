/***********************************************************************************************************************
 *  @file      source/entities/shapes/section.hpp
 *  @brief     How a SECTIONOBJECT lies in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_SECTION_HPP
#define DWGIO_ENTITIES_SHAPES_SECTION_HPP

#include "fields/codes.hpp"
#include "fields/field.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "objects/shape.hpp"

#include "dwgio/color.hpp"
#include "dwgio/entities/section.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Entities::Shapes
{
/**
 *  A CMC as R2004 stores it, in every release; its names are in the code page before R2007.
 */
struct ModernColorCode
{
  static auto Read(Fields::Input &in) -> Color
  {
    return in.In->CMC(std::max(in.With.Release, Version::R2004), Fields::TextIn(in));
  }

  static void Write(const Color &value, Fields::Output &to)
  {
    to.To->CMC(value, std::max(to.With.Release, Version::R2004), Fields::TextOut(to));
  }
};
} // namespace DwgIO::Entities::Shapes

namespace DwgIO::Objects
{
/**
 *  No chapter; the same in every release and stored under its class.
 */
template <>
struct Shape<Entities::SectionObject>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "SECTIONOBJECT";

  static auto Layout()
  {
    using Entities::SectionObject;
    constexpr int HardOwner = 3;
    return Fields::Layout(
        Fields::Long(&SectionObject::State), Fields::Long(&SectionObject::Flags), Fields::Text(&SectionObject::Name),
        Fields::Point3(&SectionObject::Vertical), Fields::Double(&SectionObject::Top),
        Fields::Double(&SectionObject::Bottom), Fields::Short(&SectionObject::Transparency),
        Fields::Member<SectionObject, Color, Entities::Shapes::ModernColorCode>{&SectionObject::Color},
        Fields::List(&SectionObject::Vertices, Fields::Counted::Long, Fields::Point3()),
        Fields::List(&SectionObject::Back, Fields::Counted::Long, Fields::Point3()),
        Fields::Link(&SectionObject::Settings, HardOwner));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_SECTION_HPP */
