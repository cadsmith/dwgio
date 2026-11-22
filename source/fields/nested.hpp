/***********************************************************************************************************************
 *  @file      source/fields/nested.hpp
 *  @brief     A member read and written through its own kind's layout.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_NESTED_HPP
#define DWGIO_FIELDS_NESTED_HPP

#include "fields/context.hpp"
#include "fields/layout.hpp"

#include <utility>

namespace DwgIO::Fields
{
/**
 *  A member of its own kind, read and written through that kind's layout.
 */
template <typename KindT, typename PartT, typename LayoutT>
struct NestedField
{
  PartT KindT::*At{nullptr};
  LayoutT Inner;

  auto Read(KindT &kind, Input &in) const -> bool { return DwgIO::Fields::Read(Inner, kind.*At, in); }

  auto Write(const KindT &kind, Output &to) const -> bool { return DwgIO::Fields::Write(Inner, kind.*At, to); }
};

template <typename KindT, typename PartT, typename LayoutT>
auto Part(PartT KindT::*at, LayoutT inner) -> NestedField<KindT, PartT, LayoutT>
{
  return {.At = at, .Inner = std::move(inner)};
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_NESTED_HPP */
