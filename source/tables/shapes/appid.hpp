/***********************************************************************************************************************
 *  @file      source/tables/shapes/appid.hpp
 *  @brief     How an APPID record lies in the file, chapter 20.4.66.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_APPID_HPP
#define DWGIO_TABLES_SHAPES_APPID_HPP

#include "fields/codes.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/appid.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace DwgIO::Objects
{
template <>
struct Shape<Tables::AppId>
{
  static constexpr std::array<std::uint16_t, 1> Types{67};
  static constexpr std::string_view Name = "APPID";

  static auto Layout()
  {
    return Fields::Layout(Fields::Part(&Tables::AppId::Record, Tables::Shapes::RecordLayout()),
                          Fields::Byte(&Tables::AppId::Unknown1));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_APPID_HPP */
