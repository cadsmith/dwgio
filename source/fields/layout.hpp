/***********************************************************************************************************************
 *  @file      source/fields/layout.hpp
 *  @brief     A kind's fields in file order, read and written in turn.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_LAYOUT_HPP
#define DWGIO_FIELDS_LAYOUT_HPP

#include "fields/context.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <tuple>

namespace DwgIO::Fields
{
/**
 *  The fields of a kind, in the order the file has them.
 */
template <typename... FieldsT>
[[nodiscard]] auto Layout(FieldsT... fields) -> std::tuple<FieldsT...>
{
  return std::tuple<FieldsT...>{fields...};
}

/**
 *  Reads each field in turn into the kind; false when one failed.
 *
 *  A field after a failed read leaves its member as it was, since each looks at the reader before it reads.
 */
template <typename LayoutT, typename KindT>
[[nodiscard]] auto Read(const LayoutT &layout, KindT &kind, Input &in) -> bool
{
  return std::apply(
      [&kind, &in](const auto &...field) -> bool
      {
        const std::array<bool, 1 + sizeof...(field)> done{true, field.Read(kind, in)...};
        return std::ranges::all_of(done, std::identity{});
      },
      layout);
}

/**
 *  Writes each field in turn from the kind; false when one failed.
 */
template <typename LayoutT, typename KindT>
[[nodiscard]] auto Write(const LayoutT &layout, const KindT &kind, Output &to) -> bool
{
  return std::apply(
      [&kind, &to](const auto &...field) -> bool
      {
        const std::array<bool, 1 + sizeof...(field)> done{true, field.Write(kind, to)...};
        return std::ranges::all_of(done, std::identity{});
      },
      layout);
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_LAYOUT_HPP */
