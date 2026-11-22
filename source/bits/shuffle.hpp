/***********************************************************************************************************************
 *  @file      source/bits/shuffle.hpp
 *  @brief     The byte order R2007's checksums and CRCs take an eight-byte group in.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_SHUFFLE_HPP
#define DWGIO_BITS_SHUFFLE_HPP

#include <array>
#include <cstddef>
#include <span>

namespace DwgIO::Bits
{
/**
 *  The bytes of an eight-byte group in the order R2007's checksums and CRCs take them: its pairs 6, 4, 2 and 0.
 */
inline constexpr std::array<std::size_t, 8> Group{6, 7, 4, 5, 2, 3, 0, 1};

/**
 *  The order of a shorter tail of one to seven bytes.
 */
inline constexpr std::array<std::array<std::size_t, 8>, 8> Tails{
    {{}, {0}, {0, 1}, {0, 1, 2}, {2, 3, 0, 1}, {2, 3, 0, 1, 4}, {2, 3, 0, 1, 4, 5}, {2, 3, 0, 1, 4, 5, 6}}};

/**
 *  Calls `take` with each byte of `bytes`, in that order.
 */
template <typename TakeT>
void Shuffle(std::span<const std::byte> bytes, const TakeT &take)
{
  std::size_t at = 0;
  for (; at + Group.size() <= bytes.size(); at += Group.size())
  {
    for (const std::size_t one : Group)
    {
      take(bytes[at + one]);
    }
  }
  const std::size_t rest = bytes.size() - at;
  for (std::size_t index = 0; index < rest; ++index)
  {
    take(bytes[at + Tails.at(rest).at(index)]);
  }
}
} // namespace DwgIO::Bits

#endif /* DWGIO_BITS_SHUFFLE_HPP */
