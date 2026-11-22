/***********************************************************************************************************************
 *  @file      source/fields/fixed.hpp
 *  @brief     A member of raw bytes of a fixed count.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_FIXED_HPP
#define DWGIO_FIELDS_FIXED_HPP

#include "fields/context.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

namespace DwgIO::Fields
{
/**
 *  A member of raw bytes of a fixed count, written as kept, shorter ones padded with zeros and longer ones cut.
 */
template <typename KindT>
struct FixedField
{
  std::vector<std::byte> KindT::*At{nullptr};
  std::size_t Count{};

  auto Read(KindT &kind, Input &in) const -> bool
  {
    if (in.In->HasFailed())
    {
      return false;
    }
    kind.*At = in.In->Bytes(Count);
    return !in.In->HasFailed();
  }

  auto Write(const KindT &kind, Output &to) const -> bool
  {
    const std::vector<std::byte> &bytes = kind.*At;
    const std::size_t kept = std::min(bytes.size(), Count);
    to.To->Raw(std::span{bytes}.first(kept));
    for (std::size_t at = kept; at < Count; ++at)
    {
      to.To->RC(0);
    }
    return true;
  }
};

template <typename KindT>
auto FixedBytes(std::vector<std::byte> KindT::*at, std::size_t count) -> FixedField<KindT>
{
  return {.At = at, .Count = count};
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_FIXED_HPP */
