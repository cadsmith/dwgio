/***********************************************************************************************************************
 *  @file      tests/dwgio/support/twin.hpp
 *  @brief     A fixture's DXF twin as code and value pairs, and the lookups every twin test makes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_TWIN_HPP
#define DWGIO_TESTS_SUPPORT_TWIN_HPP

#include "ascii.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO
{
/**
 *  One group of the DXF twin: its code and its value, blanks trimmed.
 */
struct Pair
{
  int Code{};
  std::string Value;
};

/**
 *  The DXF twin of a fixture as code and value pairs, by its file name under tests/fixtures/acad.
 */
inline auto ReadTwin(std::string_view name) -> std::vector<Pair>
{
  std::ifstream file{std::string{DWGIO_FIXTURES_DIR "/acad/"} + std::string{name}};
  REQUIRE_MESSAGE(file, name);
  std::vector<Pair> pairs;
  std::string code;
  std::string value;
  while (std::getline(file, code) && std::getline(file, value))
  {
    const auto trim = [](std::string &text) -> void
    {
      const std::size_t first = text.find_first_not_of(" \t\r");
      const std::size_t last = text.find_last_not_of(" \t\r");
      text = first == std::string::npos ? std::string{} : text.substr(first, last - first + 1);
    };
    trim(code);
    trim(value);
    pairs.push_back(Pair{.Code = std::stoi(code), .Value = value});
  }
  return pairs;
}

/**
 *  The first record the twin opens with a 0 group of that kind, whose 2 group is that name when one is given: the index
 *  of its 0 group, or none.
 */
inline auto FindOpening(const std::vector<Pair> &twin, std::string_view kind, std::string_view name = {})
    -> std::optional<std::size_t>
{
  for (std::size_t at = 0; at < twin.size(); ++at)
  {
    if (twin[at].Code != 0 || twin[at].Value != kind)
    {
      continue;
    }
    if (name.empty())
    {
      return at;
    }
    for (std::size_t inner = at + 1; inner < twin.size() && twin[inner].Code != 0; ++inner)
    {
      if (twin[inner].Code == 2 && twin[inner].Value == name)
      {
        return at;
      }
    }
  }
  return std::nullopt;
}

/**
 *  The text in upper case, ASCII letters only, as AutoCAD folds names.
 */
inline auto Upper(std::string_view text) -> std::string
{
  std::string out{text};
  std::ranges::transform(out, out.begin(), Ascii::Upper);
  return out;
}

/**
 *  The first record of that kind whose 2 group is that name in any case, as an R14 file keeps names in upper case.
 */
inline auto FindOpeningNoCase(const std::vector<Pair> &twin, std::string_view kind, std::string_view name)
    -> std::optional<std::size_t>
{
  for (std::size_t at = 0; at < twin.size(); ++at)
  {
    if (twin[at].Code != 0 || twin[at].Value != kind)
    {
      continue;
    }
    for (std::size_t inner = at + 1; inner < twin.size() && twin[inner].Code != 0; ++inner)
    {
      if (twin[inner].Code == 2 && Ascii::Same(twin[inner].Value, name))
      {
        return at;
      }
    }
  }
  return std::nullopt;
}

/**
 *  The value of a group inside the record opened at that index, or empty.
 */
inline auto GroupOf(const std::vector<Pair> &twin, std::size_t opened, int code) -> std::string
{
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    if (twin[at].Code == code)
    {
      return twin[at].Value;
    }
  }
  return {};
}

/**
 *  Every value of a group inside the record opened at that index, in order.
 */
inline auto Every(const std::vector<Pair> &twin, std::size_t opened, int code) -> std::vector<std::string>
{
  std::vector<std::string> values;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    if (twin[at].Code == code)
    {
      values.push_back(twin[at].Value);
    }
  }
  return values;
}

/**
 *  A group's value as a number, 0 when the record has none.
 */
inline auto ReadReal(const std::vector<Pair> &twin, std::size_t opened, int code) -> double
{
  const std::string value = GroupOf(twin, opened, code);
  return value.empty() ? 0.0 : std::stod(value);
}

/**
 *  The point a group code and the two codes 10 and 20 above it give, inside the record opened at that index.
 */
inline auto ReadPoint(const std::vector<Pair> &twin, std::size_t opened, int code) -> Vector
{
  return Vector{.X = ReadReal(twin, opened, code),
                .Y = ReadReal(twin, opened, code + 10),
                .Z = ReadReal(twin, opened, code + 20)};
}

/**
 *  A group's value as a whole number, 0 when the record has none.
 */
inline auto ReadWhole(const std::vector<Pair> &twin, std::size_t opened, int code) -> int
{
  const std::string value = GroupOf(twin, opened, code);
  return value.empty() ? 0 : std::stoi(value);
}

/**
 *  A record's own handle: its 5 group, or 105 for a DIMSTYLE.
 */
inline auto Self(const std::vector<Pair> &twin, std::size_t opened) -> Handle
{
  const std::string five = GroupOf(twin, opened, 5);
  return ParseHandle(five.empty() ? GroupOf(twin, opened, 105) : five).value_or(Handle{});
}

/**
 *  The record of that kind with that handle: the index of its 0 group, or none. For the tables whose names repeat.
 */
inline auto FindOpeningBy(const std::vector<Pair> &twin, std::string_view kind, Handle self)
    -> std::optional<std::size_t>
{
  for (std::size_t at = 0; at < twin.size(); ++at)
  {
    if (twin[at].Code == 0 && twin[at].Value == kind && Self(twin, at) == self)
    {
      return at;
    }
  }
  return std::nullopt;
}

/**
 *  The value of a header variable, the group right after its 9 group.
 */
inline auto Variable(const std::vector<Pair> &twin, std::string_view name) -> std::string
{
  for (std::size_t at = 0; at + 1 < twin.size(); ++at)
  {
    if (twin[at].Code == 9 && twin[at].Value == name)
    {
      return twin[at + 1].Value;
    }
  }
  return {};
}
} // namespace DwgIO

#endif /* DWGIO_TESTS_SUPPORT_TWIN_HPP */
