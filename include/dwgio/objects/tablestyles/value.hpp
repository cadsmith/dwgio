/***********************************************************************************************************************
 *  @file      dwgio/objects/tablestyles/value.hpp
 *  @brief     A table cell's value, from R2007.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_TABLESTYLES_VALUE_HPP
#define DWGIO_OBJECTS_TABLESTYLES_VALUE_HPP

#include "dwgio/api.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO::Objects::TableStyles
{
/**
 *  A cell's value, from R2007.
 */
struct DWGIO_API Value
{
  int Flags{}; /**< The flags: 1 when the value is of the general type and stores no data. Group 93. */
  /**
   *  The type of the data: 0 unknown or 1 a long, in Long; 2 a double, in Real; 4 a string, 8 a date and 512 a general
   *  value, in Bytes, a string as UTF-16 with its NUL. Group 90.
   */
  int Type{};
  std::int32_t Long{};            /**< For type 0 or 1.                                                             */
  double Real{};                  /**< For type 2.                                                                  */
  std::vector<std::byte> Bytes{}; /**< For type 4, 8 or 512, as stored.                                             */
  int Unit{};                     /**< The unit: 0 none, 1 a distance, 2 an angle, 4 an area, 8 a volume. Group 94. */
  std::string Format{};
  std::string Text{};

  auto operator==(const Value &) const -> bool = default;
};
} // namespace DwgIO::Objects::TableStyles

#endif /* DWGIO_OBJECTS_TABLESTYLES_VALUE_HPP */
