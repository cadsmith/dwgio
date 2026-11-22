/***********************************************************************************************************************
 *  @file      source/bits/strings.cpp
 *  @brief     The string stream of objects, the header and the classes from R2007.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/strings.hpp"

#include <cstdint>

namespace DwgIO::Bits
{
namespace
{
constexpr std::size_t Short = 16;

constexpr std::size_t High = 0x8000;

constexpr std::size_t Low = 0x7FFF;

constexpr std::size_t Shift = 15;
} // namespace

auto FindStrings(std::span<const std::byte> bits, std::size_t handles)
    -> std::expected<std::optional<Strung>, std::string>
{
  if (handles == 0)
  {
    return std::nullopt;
  }
  Reader at{bits};
  at.Seek(handles - 1);
  if (!at.B())
  {
    return std::nullopt;
  }
  std::size_t end = handles - 1;
  if (end < Short)
  {
    return std::unexpected(std::string{"string stream size before the data"});
  }
  end -= Short;
  at.Seek(end);
  std::size_t size = at.RS();
  if ((size & High) != 0)
  {
    if (end < Short)
    {
      return std::unexpected(std::string{"string stream size before the data"});
    }
    end -= Short;
    at.Seek(end);
    size = (size & Low) | (static_cast<std::size_t>(at.RS()) << Shift);
  }
  if (size > end)
  {
    return std::unexpected(std::string{"string stream before the data"});
  }
  return Strung{.Start = end - size, .End = end};
}

auto MakeStringReader(std::span<const std::byte> bits, const std::optional<Strung> &strung) -> std::optional<Reader>
{
  if (!strung)
  {
    return std::nullopt;
  }
  Reader reader{bits};
  reader.Seek(strung->Start);
  return reader;
}

auto WriteStringTail(const Writer &strings) -> Writer
{
  Writer tail;
  const std::size_t size = strings.Position();
  if (size == 0)
  {
    tail.B(false);
    return tail;
  }
  tail.Append(strings);
  if (size > Low)
  {
    tail.RS(static_cast<std::uint16_t>(size >> Shift));
    tail.RS(static_cast<std::uint16_t>((size & Low) | High));
  }
  else
  {
    tail.RS(static_cast<std::uint16_t>(size));
  }
  tail.B(true);
  return tail;
}
} // namespace DwgIO::Bits
