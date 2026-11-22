/***********************************************************************************************************************
 *  @file      tests/dwgio/boxed.cpp
 *  @brief     The box an object keeps its kind in: its size, copies and moves.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/boxed.hpp"

#include "dwgio/entities/line.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <type_traits>
#include <utility>

namespace DwgIO
{
TEST_CASE("boxed: an object is the size of a pointer and an index")
{
  CHECK(sizeof(Object) <= 2 * sizeof(void *));
  CHECK(std::is_nothrow_move_constructible_v<Object>);
  CHECK(std::is_same_v<Boxed<Entities::Line>::Type, Entities::Line>);
}

TEST_CASE("boxed: a copy is a separate value")
{
  Boxed<Entities::Line> line{Entities::Line{.Thickness = 2}};
  const Boxed<Entities::Line> copy{line};
  line->Thickness = 3;
  CHECK(copy->Thickness == 2);
  CHECK((*copy).Thickness == 2);
  CHECK_FALSE(copy == line);

  Boxed<Entities::Line> assigned{};
  assigned = copy;
  CHECK(assigned == copy);
  const auto &self = assigned;
  assigned = self;
  CHECK(assigned->Thickness == 2);

  const Entities::Line given{.Thickness = 4};
  const Boxed<Entities::Line> from{given};
  CHECK((*from).Thickness == 4);
  (*line).Thickness = 5;
  CHECK(line->Thickness == 5);
}

TEST_CASE("boxed: a move hands the value over")
{
  Boxed<Entities::Line> line{Entities::Line{.Thickness = 2}};
  const Entities::Line *held = line.operator->();
  Boxed<Entities::Line> moved{std::move(line)};
  CHECK(moved.operator->() == held);
  line = Boxed<Entities::Line>{Entities::Line{.Thickness = 3}};
  CHECK(line->Thickness == 3);
}
} // namespace DwgIO
