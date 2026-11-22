/***********************************************************************************************************************
 *  @file      source/file/locator/sentinels.cpp
 *  @brief     The sentinels that frame a locator file's blocks.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/sentinels.hpp"

#include <algorithm>

namespace DwgIO::File::Locator
{
auto Matches(std::span<const std::byte> bytes, const Mark &mark) -> bool
{
  return bytes.size() >= mark.size() &&
         std::ranges::equal(bytes.first(mark.size()), mark, {},
                            [](std::byte one) -> std::uint8_t { return static_cast<std::uint8_t>(one); });
}

void Put(std::vector<std::byte> &out, const Mark &mark)
{
  for (const std::uint8_t one : mark)
  {
    out.push_back(static_cast<std::byte>(one));
  }
}
} // namespace DwgIO::File::Locator
