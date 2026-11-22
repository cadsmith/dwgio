/***********************************************************************************************************************
 *  @file      source/bits/reference.hpp
 *  @brief     A handle reference with its code, and the offset codes resolved.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_BITS_REFERENCE_HPP
#define DWGIO_BITS_REFERENCE_HPP

#include "dwgio/handle.hpp"

#include <cstdint>

namespace DwgIO::Bits
{
/**
 *  A handle reference as chapter 2.13 stores it: the code saying how the handle relates, and the handle itself.
 */
struct Reference
{
  /**
   *  2 soft owner, 3 hard owner, 4 soft pointer, 5 hard pointer; 6, 8, 0xA and 0xC an offset from the object's own.
   */
  int Code{};
  Handle Value{}; /**< The offset for the offset codes. */

  auto operator==(const Reference &) const -> bool = default;
};

/**
 *  Whether a reference code counts from the object's own handle: 6 one above it, 8 one below, 0xA the value above, 0xC
 *  the value below. Chapter 2.13 makes each of them a soft pointer.
 */
[[nodiscard]] constexpr auto IsOffset(int code) -> bool
{
  return code == 6 || code == 8 || code == 0xA || code == 0xC;
}

/**
 *  The soft pointer AutoCAD writes: code 4 and no value for null, 6 for one above self, 8 for one below, 0xA or 0xC
 *  with the distance for one further above or below while the distance is smaller than the handle, code 4 with the
 *  handle otherwise.
 */
[[nodiscard]] constexpr auto Relative(Handle target, Handle self) -> Reference
{
  if (target == Handle{})
  {
    return Reference{.Code = 4, .Value = Handle{}};
  }
  if (target.Value == self.Value + 1)
  {
    return Reference{.Code = 6, .Value = Handle{}};
  }
  if (target.Value + 1 == self.Value)
  {
    return Reference{.Code = 8, .Value = Handle{}};
  }
  const bool above = target.Value > self.Value;
  const std::uint64_t distance = above ? target.Value - self.Value : self.Value - target.Value;
  if (distance >= target.Value)
  {
    return Reference{.Code = 4, .Value = target};
  }
  return Reference{.Code = above ? 0xA : 0xC, .Value = Handle{distance}};
}

/**
 *  An offset code counts from self; any other code names the handle itself.
 */
[[nodiscard]] constexpr auto Resolve(Reference reference, Handle self) -> Handle
{
  switch (reference.Code)
  {
  case 6:
    return Handle{self.Value + 1};
  case 8:
    return Handle{self.Value - 1};
  case 0xA:
    return Handle{self.Value + reference.Value.Value};
  case 0xC:
    return Handle{self.Value - reference.Value.Value};
  default:
    return reference.Value;
  }
}
} // namespace DwgIO::Bits

#endif /* DWGIO_BITS_REFERENCE_HPP */
