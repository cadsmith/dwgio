/***********************************************************************************************************************
 *  @file      dwgio/objects/dictionaryvar.hpp
 *  @brief     The DICTIONARYVAR object: a drawing variable kept as text.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_DICTIONARYVAR_HPP
#define DWGIO_OBJECTS_DICTIONARYVAR_HPP

#include "dwgio/api.hpp"
#include "dwgio/base.hpp"

#include <string>

namespace DwgIO::Objects
{
/**
 *  A DICTIONARYVAR: a drawing variable kept as text in the variables dictionary, such as DIMASSOC.
 */
struct DWGIO_API DictionaryVar
{
  DwgIO::Base Base{};
  int Schema{}; /**< 0 as AutoCAD writes it. Group 280. */
  std::string Value{};

  auto operator==(const DictionaryVar &) const -> bool = default;
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_DICTIONARYVAR_HPP */
