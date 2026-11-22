/***********************************************************************************************************************
 *  @file      source/entities/shapes/tolerance.hpp
 *  @brief     How a TOLERANCE lies in the file, chapter 20.4.49.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_TOLERANCE_HPP
#define DWGIO_ENTITIES_SHAPES_TOLERANCE_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/tolerance.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Entities::Tolerance>
{
  static constexpr std::array<std::uint16_t, 1> Types{46};
  static constexpr std::string_view Name = "TOLERANCE";

  static auto Layout()
  {
    using Entities::Tolerance;
    return Fields::Layout(Fields::Until(Version::R14, Fields::Short(&Tolerance::Unknown1),
                                        Fields::Double(&Tolerance::Height), Fields::Double(&Tolerance::Gap)),
                          Fields::Point3(&Tolerance::Position), Fields::Point3(&Tolerance::Direction),
                          Fields::Point3(&Tolerance::Normal), Fields::Text(&Tolerance::Text),
                          Fields::Link(&Tolerance::Style, 5));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_TOLERANCE_HPP */
