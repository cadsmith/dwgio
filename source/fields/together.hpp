/***********************************************************************************************************************
 *  @file      source/fields/together.hpp
 *  @brief     Fields built apart and read as one.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_TOGETHER_HPP
#define DWGIO_FIELDS_TOGETHER_HPP

#include "fields/context.hpp"
#include "fields/layout.hpp"

#include <tuple>
#include <utility>

namespace DwgIO::Fields
{
/**
 *  Fields of the kind itself, built apart and read and written in place, as one field.
 */
template <typename... FieldsT>
struct TogetherField
{
  std::tuple<FieldsT...> Inner;

  template <typename KindT>
  auto Read(KindT &kind, Input &in) const -> bool
  {
    return DwgIO::Fields::Read(Inner, kind, in);
  }

  template <typename KindT>
  auto Write(const KindT &kind, Output &to) const -> bool
  {
    return DwgIO::Fields::Write(Inner, kind, to);
  }
};

template <typename... FieldsT>
auto Together(std::tuple<FieldsT...> fields) -> TogetherField<FieldsT...>
{
  return {.Inner = std::move(fields)};
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_TOGETHER_HPP */
