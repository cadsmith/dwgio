/***********************************************************************************************************************
 *  @file      dwgio/flags.hpp
 *  @brief     Testing a bit of a flags field by its name.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FLAGS_HPP
#define DWGIO_FLAGS_HPP

#include <type_traits>
#include <utility>

namespace DwgIO
{
/**
 *  Whether a flags field has every bit of a named flag, such as `Has(polyline.Flags, PolylineFlags::Closed)`.
 */
template <class FlagT>
  requires std::is_enum_v<FlagT>
[[nodiscard]] constexpr auto Has(int flags, FlagT flag) -> bool
{
  const auto bits = static_cast<unsigned>(std::to_underlying(flag));
  return (static_cast<unsigned>(flags) & bits) == bits;
}
} // namespace DwgIO

#endif /* DWGIO_FLAGS_HPP */
