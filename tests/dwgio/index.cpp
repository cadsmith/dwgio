/***********************************************************************************************************************
 *  @file      tests/dwgio/index.cpp
 *  @brief     The index of a drawing's tables and controls.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/index.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/control.hpp"

#include <doctest/doctest.h>

namespace DwgIO
{
TEST_CASE("index: the first control of a table is kept and a control of no table is ignored")
{
  Drawing drawing;
  drawing.Objects = {
      Object{Tables::Control{.Base = {.Self = Handle{.Value = 1}}, .Table = Tables::Table::Layer}},
      Object{Tables::Control{.Base = {.Self = Handle{.Value = 2}}, .Table = Tables::Table::Layer}},
      Object{Tables::Control{.Base = {.Self = Handle{.Value = 3}}, .Table = static_cast<Tables::Table>(200)}}};
  const Index index{drawing};
  CHECK(index.Control(Tables::Table::Layer)->Base.Self == Handle{.Value = 1});
}
} // namespace DwgIO
