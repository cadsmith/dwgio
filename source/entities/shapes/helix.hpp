/***********************************************************************************************************************
 *  @file      source/entities/shapes/helix.hpp
 *  @brief     How a HELIX lies in the file: a spline's curve, then the coil.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_HELIX_HPP
#define DWGIO_ENTITIES_SHAPES_HELIX_HPP

#include "entities/shapes/spline.hpp"
#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/helix.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  No chapter: the curve as chapter 20.4.40 stores a spline's, then the coil; stored under its class.
 */
template <>
struct Shape<Entities::Helix>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "HELIX";

  static auto Layout()
  {
    using Entities::Curve;
    using Entities::Helix;
    using Entities::HelixData;
    return Fields::Layout(
        Fields::Part(&Helix::Curve, Fields::Layout(Fields::Custom(Entities::Shapes::ReadSpline<Curve>,
                                                                  Entities::Shapes::WriteSpline<Curve>))),
        Fields::Part(&Helix::Data,
                     Fields::Layout(Fields::Long(&HelixData::Major), Fields::Long(&HelixData::Minor),
                                    Fields::Point3(&HelixData::BasePoint), Fields::Point3(&HelixData::Start),
                                    Fields::Point3(&HelixData::Axis), Fields::Double(&HelixData::Radius),
                                    Fields::Double(&HelixData::Turns), Fields::Double(&HelixData::Rise),
                                    Fields::Bit(&HelixData::RightHanded), Fields::Byte(&HelixData::Constraint))));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_HELIX_HPP */
