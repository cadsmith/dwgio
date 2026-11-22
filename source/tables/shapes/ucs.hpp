/***********************************************************************************************************************
 *  @file      source/tables/shapes/ucs.hpp
 *  @brief     How a UCS record lies in the file, chapter 20.4.62.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_UCS_HPP
#define DWGIO_TABLES_SHAPES_UCS_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/ucs.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Tables::Ucs>
{
  static constexpr std::array<std::uint16_t, 1> Types{63};
  static constexpr std::string_view Name = "UCS";

  static auto Layout()
  {
    using Tables::Ucs;
    using namespace Tables::Shapes;
    return Fields::Layout(Fields::Part(&Ucs::Record, RecordLayout()), Fields::Point3(&Ucs::Origin),
                          Fields::Point3(&Ucs::X), Fields::Point3(&Ucs::Y),
                          Fields::Since(Version::R2000, Fields::Double(&Ucs::Elevation), Fields::Short(&Ucs::Ortho),
                                        Fields::Short(&Ucs::Orthographic), Fields::Link(&Ucs::Basis, HardPointer),
                                        Fields::Link(&Ucs::Named, HardPointer)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_UCS_HPP */
