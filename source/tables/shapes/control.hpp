/***********************************************************************************************************************
 *  @file      source/tables/shapes/control.hpp
 *  @brief     How the tables' control objects lie in the file, chapters 20.4.51 to 20.4.69.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_CONTROL_HPP
#define DWGIO_TABLES_SHAPES_CONTROL_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/list.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"
#include "objects/shape.hpp"
#include "tables/codes.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace DwgIO::Tables::Shapes
{
inline auto IsBlock(const Control &control) -> bool
{
  return control.Table == Table::Block;
}

inline auto IsLtype(const Control &control) -> bool
{
  return control.Table == Table::LType;
}

inline auto IsDimstyle(const Control &control) -> bool
{
  return control.Table == Table::DimStyle;
}
} // namespace DwgIO::Tables::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Tables::Control>
{
  static constexpr std::array<std::uint16_t, Tables::Count> Types = Tables::ControlCodes;
  static constexpr std::string_view Name = "CONTROL";

  static void Open(Tables::Control &control, std::uint16_t type)
  {
    control.Table = static_cast<Tables::Table>(std::ranges::find(Types, type) - Types.begin());
  }

  static auto Code(const Tables::Control &control) -> std::uint16_t { return Tables::ControlCode(control.Table); }

  static auto NameOf(const Tables::Control &control) -> std::string_view { return Tables::NameOf(control.Table); }

  static auto Layout()
  {
    using Tables::Control;
    return Fields::Layout(
        Fields::List(&Control::Entries, Fields::Counted::Long, Fields::Link(Tables::Shapes::SoftOwner)),
        Fields::When(Tables::Shapes::IsBlock, Fields::Link(&Control::ModelSpace, Tables::Shapes::HardOwner),
                     Fields::Link(&Control::PaperSpace, Tables::Shapes::HardOwner)),
        Fields::When(Tables::Shapes::IsLtype, Fields::Link(&Control::ByBlock, Tables::Shapes::HardOwner),
                     Fields::Link(&Control::ByLayer, Tables::Shapes::HardOwner)),
        Fields::When(Tables::Shapes::IsDimstyle,
                     Fields::Since(Version::R2000, Fields::List(&Control::RepeatedEntries, Fields::Counted::Char,
                                                                Fields::Link(Tables::Shapes::HardPointer)))));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_CONTROL_HPP */
