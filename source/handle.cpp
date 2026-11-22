/***********************************************************************************************************************
 *  @file      source/handle.cpp
 *  @brief     A handle written and parsed as hexadecimal text.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/handle.hpp"

#include <charconv>
#include <format>
#include <memory>
#include <system_error>

namespace DwgIO
{
namespace
{
constexpr std::size_t Digits = 16;

constexpr int Base = 16;

constexpr std::string_view Blanks = " \t";

auto Trim(std::string_view text) -> std::string_view
{
  const std::size_t first = text.find_first_not_of(Blanks);
  if (first == std::string_view::npos)
  {
    return {};
  }
  return text.substr(first, text.find_last_not_of(Blanks) - first + 1);
}
} // namespace

auto HexOf(Handle handle) -> std::string
{
  return std::format("{:X}", handle.Value);
}

auto ParseHandle(std::string_view text) -> std::optional<Handle>
{
  const std::string_view word = Trim(text);
  if (word.empty() || word.size() > Digits)
  {
    return std::nullopt;
  }
  std::uint64_t value = 0;
  const char *last = std::to_address(word.end());
  const std::from_chars_result read = std::from_chars(std::to_address(word.begin()), last, value, Base);
  if (read.ec != std::errc{} || read.ptr != last)
  {
    return std::nullopt;
  }
  return Handle{value};
}
} // namespace DwgIO
