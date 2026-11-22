/***********************************************************************************************************************
 *  @file      dwgio/handle.hpp
 *  @brief     Handles, the numbers records are known and pointed to by, and their hex form.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_HANDLE_HPP
#define DWGIO_HANDLE_HPP

#include "dwgio/api.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace DwgIO
{
/**
 *  The number a record is known by, and that other records point to it with.
 */
struct DWGIO_API Handle
{
  std::uint64_t Value{}; /**< Up to eight bytes in a DWG, hex in a DXF; zero names no record. */

  constexpr explicit operator bool() const noexcept { return Value != 0; }

  constexpr auto operator<=>(const Handle &) const = default;
};

/**
 *  The handle as DXF writes it: uppercase hex with no leading zeros, such as "1F", and "0" for the null handle.
 */
[[nodiscard]] DWGIO_API auto HexOf(Handle handle) -> std::string;

/**
 *  Reads a handle written in hex, in either case, with spaces or tabs around it; "0" gives the null handle.
 *
 *  @return Empty when the text holds no digits, anything but hex digits, or more than 16 of them.
 */
[[nodiscard]] DWGIO_API auto ParseHandle(std::string_view text) -> std::optional<Handle>;
} // namespace DwgIO

template <>
struct std::hash<DwgIO::Handle>
{
  auto operator()(DwgIO::Handle one) const noexcept -> std::size_t { return std::hash<std::uint64_t>{}(one.Value); }
};

#endif /* DWGIO_HANDLE_HPP */
