/***********************************************************************************************************************
 *  @file      tests/dwgio/support/cursor.hpp
 *  @brief     A cursor over the DXF groups of one record, for nested lists such as a hatch's.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_CURSOR_HPP
#define DWGIO_TESTS_SUPPORT_CURSOR_HPP

#include "support/twin.hpp"

#include "dwgio/vector.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <string>
#include <vector>

namespace DwgIO::Entities
{
/**
 *  Walks the twin's groups of one record in order, so a hatch's nested lists can be read as DXF lays them out.
 */
class Cursor
{
public:
  Cursor(const std::vector<Pair> &twin, std::size_t opened) : m_Twin{&twin}, m_At{opened + 1} {}

  [[nodiscard]] auto Code() const -> int { return m_At < m_Twin->size() ? (*m_Twin)[m_At].Code : 0; }

  auto Take(int code) -> std::string
  {
    REQUIRE(Code() == code);
    return (*m_Twin)[m_At++].Value;
  }

  auto Real(int code) -> double { return std::stod(Take(code)); }

  auto Whole(int code) -> int { return std::stoi(Take(code)); }

  auto Point(int code) -> Vector
  {
    Vector point;
    point.X = Real(code);
    point.Y = Real(code + 10);
    return point;
  }

  /**
   *  Moves past groups until that code shows, for the groups a hatch's head carries in no fixed order.
   */
  void Until(int code)
  {
    while (Code() != code && Code() != 0)
    {
      ++m_At;
    }
  }

private:
  const std::vector<Pair> *m_Twin;
  std::size_t m_At;
};
} // namespace DwgIO::Entities

#endif /* DWGIO_TESTS_SUPPORT_CURSOR_HPP */
