/***********************************************************************************************************************
 *  @file      source/entities/shapes/attdef.hpp
 *  @brief     How an ATTDEF lies in the file, chapter 20.4.5.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_ATTDEF_HPP
#define DWGIO_ENTITIES_SHAPES_ATTDEF_HPP

#include "entities/shapes/lettering.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/attdef.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::AttDef>
{
  static constexpr std::array<std::uint16_t, 1> Types{3};
  static constexpr std::string_view Name = "ATTDEF";

  /**
   *  A multiline attribute, whose embedded text the layout does not read, is kept as bytes.
   */
  static auto IsReadable(const Entities::AttDef &attribute) -> bool { return attribute.Type == 1; }

  static auto Layout()
  {
    using Entities::AttDef;
    using namespace Entities::Shapes;
    return Fields::Layout(Fields::Part(&AttDef::Lettering, LetteringLayout()),
                          Fields::Since(Version::R2010, Fields::Byte(&AttDef::Revision)),
                          Fields::Since(Version::R2018, Fields::Custom(ReadType<AttDef>, WriteType<AttDef>)),
                          Fields::Text(&AttDef::Tag), Fields::Short(&AttDef::Length), Fields::Byte(&AttDef::Flags),
                          Fields::Since(Version::R2007, Fields::Bit(&AttDef::Locked)),
                          Fields::Since(Version::R2010, Fields::Byte(&AttDef::DefinitionVersion)),
                          Fields::Text(&AttDef::Prompt));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_ATTDEF_HPP */
