/***********************************************************************************************************************
 *  @file      source/objects/walk.cpp
 *  @brief     The objects found one after another where the object map fails.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/walk.hpp"

#include "bits/crc.hpp"
#include "bits/reader.hpp"

#include <cstdint>
#include <optional>

namespace DwgIO::Objects
{
namespace
{
constexpr std::size_t Window = 64;

constexpr std::size_t Crc = 2;

/**
 *  Nothing unless the object's size fits and its CRC agrees.
 */
auto Ends(std::span<const std::byte> file, std::size_t at, std::size_t to) -> std::optional<std::size_t>
{
  Bits::Reader sizer{file.subspan(at, to - at)};
  const std::uint64_t size = sizer.MS();
  const std::size_t start = at + (sizer.Position() / 8);
  if (sizer.HasFailed() || size > to - start || to - start - size < Crc)
  {
    return std::nullopt;
  }
  Bits::Reader tail{file.subspan(start + size, Crc)};
  if (Bits::Crc(file.subspan(at, start - at + size), Bits::Seed) != tail.RS())
  {
    return std::nullopt;
  }
  return start + size + Crc;
}
} // namespace

auto Walk(std::span<const std::byte> file, std::size_t from, std::size_t to) -> std::vector<std::size_t>
{
  std::vector<std::size_t> offsets;
  to = std::min(to, file.size());
  for (std::size_t at = from; at + Crc < to;)
  {
    const std::optional<std::size_t> end = Ends(file, at, to);
    if (end)
    {
      offsets.push_back(at);
      at = *end;
    }
    else if (offsets.empty() && at - from < Window)
    {
      ++at;
    }
    else
    {
      break;
    }
  }
  return offsets;
}
} // namespace DwgIO::Objects
