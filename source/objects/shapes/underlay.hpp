/***********************************************************************************************************************
 *  @file      source/objects/shapes/underlay.hpp
 *  @brief     How the PDF, DWF and DGN underlay definitions lie in the file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_UNDERLAY_HPP
#define DWGIO_OBJECTS_SHAPES_UNDERLAY_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "objects/shape.hpp"

#include "dwgio/objects/underlay.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects::Shapes
{
/**
 *  In the order of UnderlayKind.
 */
constexpr std::array<std::string_view, 3> DefinitionNames{"PDFDEFINITION", "DWFDEFINITION", "DGNDEFINITION"};
} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
/**
 *  The file and the part of it shown; stored under its class.
 */
template <Entities::UnderlayKind KindV>
struct Shape<UnderlayDefinition<KindV>>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = Shapes::DefinitionNames.at(static_cast<std::size_t>(KindV));

  static auto Layout()
  {
    return Fields::Layout(Fields::Text(&UnderlayDefinition<KindV>::File),
                          Fields::Text(&UnderlayDefinition<KindV>::Name));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_UNDERLAY_HPP */
