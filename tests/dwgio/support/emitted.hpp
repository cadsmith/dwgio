/***********************************************************************************************************************
 *  @file      tests/dwgio/support/emitted.hpp
 *  @brief     The DXF groups a typed object stands for, to compare with its twin's.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_EMITTED_HPP
#define DWGIO_TESTS_SUPPORT_EMITTED_HPP

#include "support/twin.hpp"

#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO
{
/**
 *  A color as DXF packs it into one integer: the method in the top byte, 0xC0 ByLayer, 0xC1 ByBlock, 0xC2 a true color
 *  with its red, green and blue below, 0xC3 an index in the low byte.
 */
inline auto Pack(const Color &color) -> std::int32_t
{
  constexpr std::uint32_t Layer = 0xC0000000U;
  constexpr std::uint32_t Block = 0xC1000000U;
  constexpr std::uint32_t True = 0xC2000000U;
  constexpr std::uint32_t Indexed = 0xC3000000U;
  constexpr std::uint32_t Low = 0xFFFFFFU;
  std::uint32_t packed = Indexed | static_cast<std::uint32_t>(color.Index);
  if (color.Rgb)
  {
    packed = True | (static_cast<std::uint32_t>(*color.Rgb) & Low);
  }
  else if (color.Index == 256)
  {
    packed = Layer;
  }
  else if (color.Index == 0)
  {
    packed = Block;
  }
  return static_cast<std::int32_t>(packed);
}

/**
 *  The DXF groups a typed object stands for, in the twin's order, to hold against the twin's own.
 */
struct Emitted
{
  std::vector<Pair> Groups{};

  void Add(int code, std::string value) { Groups.push_back(Pair{.Code = code, .Value = std::move(value)}); }

  void Add(int code, int value) { Add(code, std::to_string(value)); }

  void Add(int code, bool value) { Add(code, value ? 1 : 0); }

  void Add(int code, double value) { Add(code, std::format("{}", value)); }

  void Add(int code, const Vector &value)
  {
    Add(code, value.X);
    Add(code + 10, value.Y);
    Add(code + 20, value.Z);
  }

  void Add(int code, const Color &value) { Add(code, Pack(value)); }

  /**
   *  A handle, left out when null as DXF leaves it.
   */
  void Add(int code, Handle value)
  {
    if (value)
    {
      Add(code, std::format("{:X}", value.Value));
    }
  }
};

/**
 *  The groups of the twin's record opened at that index from the one after its subclass marker to its extended data or
 *  its end.
 */
inline auto Subclass(const std::vector<Pair> &twin, std::size_t opened, std::string_view marker) -> std::vector<Pair>
{
  std::vector<Pair> groups;
  bool inside = false;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0 && twin[at].Code != 1001; ++at)
  {
    if (inside)
    {
      groups.push_back(twin[at]);
    }
    inside = inside || (twin[at].Code == 100 && twin[at].Value == marker);
  }
  return groups;
}

/**
 *  Whether two values agree: as numbers when both are, else as text.
 */
inline auto Agrees(const std::string &one, const std::string &other) -> bool
{
  std::size_t used = 0;
  std::size_t usedOther = 0;
  try
  {
    const double number = std::stod(one, &used);
    const double numberOther = std::stod(other, &usedOther);
    if (used == one.size() && usedOther == other.size())
    {
      return number == doctest::Approx(numberOther);
    }
  }
  catch (const std::exception &)
  {
  }
  return one == other;
}

inline void CheckGroups(const std::vector<Pair> &ours, const std::vector<Pair> &theirs)
{
  CHECK(ours.size() == theirs.size());
  for (std::size_t at = 0; at < ours.size() && at < theirs.size(); ++at)
  {
    CAPTURE(at);
    CHECK(ours[at].Code == theirs[at].Code);
    CAPTURE(ours[at].Value);
    CAPTURE(theirs[at].Value);
    CHECK(Agrees(ours[at].Value, theirs[at].Value));
  }
}
} // namespace DwgIO

#endif /* DWGIO_TESTS_SUPPORT_EMITTED_HPP */
