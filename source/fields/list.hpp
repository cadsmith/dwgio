/***********************************************************************************************************************
 *  @file      source/fields/list.hpp
 *  @brief     A list member: its count, then its elements.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_FIELDS_LIST_HPP
#define DWGIO_FIELDS_LIST_HPP

#include "fields/context.hpp"
#include "fields/layout.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

namespace DwgIO::Fields
{
/**
 *  How a list's count is stored.
 */
enum class Counted : std::uint8_t
{
  Short, /**< A BS.  */
  Long,  /**< A BL.  */
  Char,  /**< An RC. */
};

/**
 *  The bits left in the data and, for an object, in its handle stream: an element takes one of them at least.
 */
inline auto Room(const Input &in) -> std::size_t
{
  std::size_t room = in.In->Size() - in.In->Position();
  if (in.Handles != nullptr)
  {
    room += in.Handles->Size() - in.Handles->Position();
  }
  return room;
}

/**
 *  Reads the BL count of a list of handles the reader holds.
 *
 *  False, with the data reader failed, when the count is negative or outruns what is left, so a hostile count is
 *  refused rather than sized.
 */
inline auto ReadHandleCount(const Input &in, std::size_t &count) -> bool
{
  const std::int32_t value = in.In->BL();
  if (in.In->HasFailed() || value < 0 || static_cast<std::size_t>(value) > Room(in))
  {
    in.In->Seek(in.In->Size() + 1);
    return false;
  }
  count = static_cast<std::size_t>(value);
  return true;
}

/**
 *  A vector member: its count, then each element through the element fields.
 */
template <typename KindT, typename ItemT, typename... FieldsT>
struct ListField
{
  std::vector<ItemT> KindT::*At{nullptr};
  Counted Count{};
  std::tuple<FieldsT...> Inner;

  [[nodiscard]] auto ReadCount(Input &in) const -> std::int64_t
  {
    if (Count == Counted::Long)
    {
      return in.In->BL();
    }
    if (Count == Counted::Short)
    {
      return in.In->BS();
    }
    return in.In->RC();
  }

  void WriteCount(std::int32_t count, Output &to) const
  {
    if (Count == Counted::Long)
    {
      to.To->BL(count);
    }
    else if (Count == Counted::Short)
    {
      to.To->BS(count);
    }
    else
    {
      to.To->RC(static_cast<std::uint8_t>(count));
    }
  }

  /**
   *  Reads the count and the elements; false when the reader failed, before or now, or the count outruns what is left.
   */
  auto Read(KindT &kind, Input &in) const -> bool
  {
    if (in.In->HasFailed())
    {
      return false;
    }
    const std::int64_t count = ReadCount(in);
    std::vector<ItemT> &items = kind.*At;
    items.clear();
    if (in.In->HasFailed() || count < 0 || static_cast<std::size_t>(count) > Room(in))
    {
      in.In->Seek(in.In->Size() + 1);
      return false;
    }
    items.reserve(static_cast<std::size_t>(count));
    for (std::int64_t index = 0; index < count; ++index)
    {
      ItemT item{};
      if (!DwgIO::Fields::Read(Inner, item, in))
      {
        return false;
      }
      items.push_back(std::move(item));
    }
    return true;
  }

  auto Write(const KindT &kind, Output &to) const -> bool
  {
    const std::vector<ItemT> &items = kind.*At;
    WriteCount(static_cast<std::int32_t>(items.size()), to);
    return std::ranges::all_of(items, [this, &to](const ItemT &item) -> bool
                               { return DwgIO::Fields::Write(Inner, item, to); });
  }
};

template <typename KindT, typename ItemT, typename... FieldsT>
auto List(std::vector<ItemT> KindT::*at, Counted count, FieldsT... fields) -> ListField<KindT, ItemT, FieldsT...>
{
  return {.At = at, .Count = count, .Inner = std::tuple<FieldsT...>{fields...}};
}
} // namespace DwgIO::Fields

#endif /* DWGIO_FIELDS_LIST_HPP */
