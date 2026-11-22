/***********************************************************************************************************************
 *  @file      source/entities/shapes/attrib.hpp
 *  @brief     How an ATTRIB lies in the file, chapter 20.4.4.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_ATTRIB_HPP
#define DWGIO_ENTITIES_SHAPES_ATTRIB_HPP

#include "entities/shapes/lettering.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/attrib.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Attrib>
{
  static constexpr std::array<std::uint16_t, 1> Types{2};
  static constexpr std::string_view Name = "ATTRIB";

  /**
   *  A multiline attribute, whose embedded text the layout does not read, is kept as bytes.
   */
  static auto IsReadable(const Entities::Attrib &attribute) -> bool { return attribute.Type == 1; }

  static auto Layout()
  {
    using Entities::Attrib;
    using namespace Entities::Shapes;
    return Fields::Layout(Fields::Part(&Attrib::Lettering, LetteringLayout()),
                          Fields::Since(Version::R2010, Fields::Byte(&Attrib::Revision)),
                          Fields::Since(Version::R2018, Fields::Custom(ReadType<Attrib>, WriteType<Attrib>)),
                          Fields::Text(&Attrib::Tag), Fields::Short(&Attrib::Length), Fields::Byte(&Attrib::Flags),
                          Fields::Since(Version::R2007, Fields::Bit(&Attrib::Locked)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_ATTRIB_HPP */
