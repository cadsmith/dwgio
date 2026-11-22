/***********************************************************************************************************************
 *  @file      source/entities/shapes/leader.hpp
 *  @brief     How a LEADER lies in the file, chapter 20.4.47.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ENTITIES_SHAPES_LEADER_HPP
#define DWGIO_ENTITIES_SHAPES_LEADER_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/entities/leader.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
/**
 *  AutoCAD's files of R2010 and later hold no text box, though the chapter keeps it.
 */
template <>
struct Shape<Entities::Leader>
{
  static constexpr std::array<std::uint16_t, 1> Types{45};
  static constexpr std::string_view Name = "LEADER";

  static auto Layout()
  {
    using Entities::Leader;
    return Fields::Layout(
        Fields::Bit(&Leader::Unknown1), Fields::Short(&Leader::Annotation), Fields::Short(&Leader::Path),
        Fields::List(&Leader::Points, Fields::Counted::Long, Fields::Point3()), Fields::Point3(&Leader::Origin),
        Fields::Point3(&Leader::Normal), Fields::Point3(&Leader::Horizontal), Fields::Point3(&Leader::Block),
        Fields::Since(Version::R14, Fields::Point3(&Leader::Offset)),
        Fields::Until(Version::R14, Fields::Double(&Leader::Gap)),
        Fields::Until(Version::R2007, Fields::Double(&Leader::Height), Fields::Double(&Leader::Width)),
        Fields::Bit(&Leader::Direction), Fields::Bit(&Leader::Arrow),
        Fields::Until(Version::R14, Fields::Short(&Leader::Arrowhead), Fields::Double(&Leader::Dimasz),
                      Fields::Bit(&Leader::Unknown2), Fields::Bit(&Leader::Unknown3),
                      Fields::Short(&Leader::RepeatedColor), Fields::Short(&Leader::Color),
                      Fields::Bit(&Leader::Unknown5), Fields::Bit(&Leader::Unknown6)),
        Fields::Since(Version::R2000, Fields::Short(&Leader::ArrowheadType), Fields::Bit(&Leader::Unknown8),
                      Fields::Bit(&Leader::Unknown9)),
        Fields::Link(&Leader::Note, 5), Fields::Link(&Leader::Style, 5));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_ENTITIES_SHAPES_LEADER_HPP */
