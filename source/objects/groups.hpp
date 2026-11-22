/***********************************************************************************************************************
 *  @file      source/objects/groups.hpp
 *  @brief     What the writers of extended data and of an xrecord's groups share.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_GROUPS_HPP
#define DWGIO_OBJECTS_GROUPS_HPP

#include "bits/writer.hpp"
#include "fields/context.hpp"

#include "dwgio/issue.hpp"
#include "dwgio/tag.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace DwgIO::Objects
{
/**
 *  The groups of a point after its first: its Y and Z ten and twenty codes on.
 */
constexpr int Row = 10;

/**
 *  Where a writer of groups puts its bytes and its notes, which open with what it writes: "extended data" or "XRECORD".
 */
struct GroupWriter
{
  Bits::Writer *Out{};
  const Fields::Output *To{};
  std::string_view Subject{};

  /**
   *  Null, with a note, when the value is not of that type.
   */
  template <typename ValueT>
  [[nodiscard]] auto As(const DwgIO::Tag &group, std::string_view kind) const -> const ValueT *
  {
    const auto *value = std::get_if<ValueT>(&group.Value);
    if (value == nullptr)
    {
      Fields::Note(*To, IssueCode::Damaged, std::format("{} group {} holds no {}", Subject, group.Code, kind));
    }
    return value;
  }

  void Bytes(const std::string &raw) const
  {
    for (const char one : raw)
    {
      Out->RC(static_cast<std::uint8_t>(one));
    }
  }

  /**
   *  At most that many bytes of a text or chunk, the rest cut with a note.
   */
  [[nodiscard]] auto Truncate(std::string bytes, std::size_t most, std::string_view kind) const -> std::string
  {
    if (bytes.size() > most)
    {
      Fields::Note(*To, IssueCode::OutOfRange,
                   std::format("{} {} of {} bytes cut to {}", Subject, kind, bytes.size(), most));
      bytes.resize(most);
    }
    return bytes;
  }

  /**
   *  A point's three reals from its group, whose real is given, and the two after it, a missing one zero with a note;
   *  and how many groups it took.
   */
  [[nodiscard]] auto PointAt(std::span<const DwgIO::Tag> groups, std::size_t at, double first) const
      -> std::pair<std::array<double, 3>, std::size_t>
  {
    const int code = groups[at].Code;
    std::array<double, 3> values{first, 0.0, 0.0};
    std::size_t taken = 1;
    for (; taken < values.size(); ++taken)
    {
      const std::size_t next = at + taken;
      const auto *value = next < groups.size() && groups[next].Code == code + (static_cast<int>(taken) * Row)
                              ? std::get_if<double>(&groups[next].Value)
                              : nullptr;
      if (value == nullptr)
      {
        Fields::Note(*To, IssueCode::Damaged,
                     std::format("{} point {} without its {} and {}", Subject, code, code + Row, code + (2 * Row)));
        break;
      }
      values.at(taken) = *value;
    }
    return {values, taken};
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_GROUPS_HPP */
