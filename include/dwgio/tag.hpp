/***********************************************************************************************************************
 *  @file      dwgio/tag.hpp
 *  @brief     A tag of extended data: a DXF group code and its value.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TAG_HPP
#define DWGIO_TAG_HPP

#include "dwgio/api.hpp"
#include "dwgio/handle.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO
{
/**
 *  The value of a binary group, code 1004.
 */
struct DWGIO_API Chunk
{
  std::vector<std::byte> Bytes{};

  auto operator==(const Chunk &) const -> bool = default;
};

/**
 *  A group's value, of the type its code calls for.
 */
using Content = std::variant<std::string, std::int64_t, double, bool, Handle, Chunk>;

/**
 *  One tag of extended data: a DXF group code and its value.
 */
struct DWGIO_API Tag
{
  int Code{}; /**< Such as 1000 for text or 1070 for a short. */
  Content Value{};

  auto operator==(const Tag &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_TAG_HPP */
