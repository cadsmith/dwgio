/***********************************************************************************************************************
 *  @file      source/fields/since.hpp
 *  @brief     Fields only some releases have.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_SINCE_HPP
#define DWGIO_FIELDS_SINCE_HPP

#include "fields/context.hpp"
#include "fields/layout.hpp"

#include "dwgio/version.hpp"

#include <tuple>

namespace DwgIO::Fields
{
/**
 *  Fields a release and the ones after it have.
 */
template <typename... FieldsT>
struct SinceField
{
  Version From{};
  std::tuple<FieldsT...> Inner;

  template <typename KindT>
  auto Read(KindT &kind, Input &in) const -> bool
  {
    if (in.With.Release < From)
    {
      return true;
    }
    return DwgIO::Fields::Read(Inner, kind, in);
  }

  template <typename KindT>
  auto Write(const KindT &kind, Output &to) const -> bool
  {
    if (to.With.Release < From)
    {
      return true;
    }
    return DwgIO::Fields::Write(Inner, kind, to);
  }
};

/**
 *  Fields a release and the ones before it have.
 */
template <typename... FieldsT>
struct UntilField
{
  Version Through{}; /**< Inclusive. */
  std::tuple<FieldsT...> Inner;

  template <typename KindT>
  auto Read(KindT &kind, Input &in) const -> bool
  {
    if (in.With.Release > Through)
    {
      return true;
    }
    return DwgIO::Fields::Read(Inner, kind, in);
  }

  template <typename KindT>
  auto Write(const KindT &kind, Output &to) const -> bool
  {
    if (to.With.Release > Through)
    {
      return true;
    }
    return DwgIO::Fields::Write(Inner, kind, to);
  }
};

template <typename... FieldsT>
auto Since(Version from, FieldsT... fields) -> SinceField<FieldsT...>
{
  return {.From = from, .Inner = std::tuple<FieldsT...>{fields...}};
}

template <typename... FieldsT>
auto Until(Version through, FieldsT... fields) -> UntilField<FieldsT...>
{
  return {.Through = through, .Inner = std::tuple<FieldsT...>{fields...}};
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_SINCE_HPP */
