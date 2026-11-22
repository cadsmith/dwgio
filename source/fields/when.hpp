/***********************************************************************************************************************
 *  @file      source/fields/when.hpp
 *  @brief     Fields a test on what was read so far chooses.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_WHEN_HPP
#define DWGIO_FIELDS_WHEN_HPP

#include "fields/context.hpp"
#include "fields/layout.hpp"

#include <tuple>

namespace DwgIO::Fields
{
/**
 *  Fields a kind has when a test on what was read so far says so.
 */
template <typename KindT, typename... FieldsT>
struct WhenField
{
  bool (*Test)(const KindT &kind){nullptr};
  std::tuple<FieldsT...> Inner;

  auto Read(KindT &kind, Input &in) const -> bool
  {
    if (!Test(kind))
    {
      return true;
    }
    return DwgIO::Fields::Read(Inner, kind, in);
  }

  auto Write(const KindT &kind, Output &to) const -> bool
  {
    if (!Test(kind))
    {
      return true;
    }
    return DwgIO::Fields::Write(Inner, kind, to);
  }
};

template <typename KindT, typename... FieldsT>
auto When(bool (*test)(const KindT &kind), FieldsT... fields) -> WhenField<KindT, FieldsT...>
{
  return {.Test = test, .Inner = std::tuple<FieldsT...>{fields...}};
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_WHEN_HPP */
