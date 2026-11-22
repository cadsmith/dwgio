/***********************************************************************************************************************
 *  @file      source/acis/spec.hpp
 *  @brief     A field as a record's grammar names it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_ACIS_SPEC_HPP
#define DWGIO_ACIS_SPEC_HPP

#include "acis/model.hpp"

namespace DwgIO::Acis
{
/**
 *  A field as a record's grammar names it: its kind, the releases that have it, the value a release without it takes,
 *  and for a logical or an enumeration the words SAT writes it with.
 */
struct Spec
{
  Acis::Kind Kind{};
  Acis::Gate Gate{};
  Value Fallback{};
  const Acis::Words *Words{}; /**< A logical's words.      */
  const Acis::Table *Table{}; /**< An enumeration's words. */
};
} // namespace DwgIO::Acis

#endif /* DWGIO_ACIS_SPEC_HPP */
