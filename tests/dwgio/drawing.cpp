/***********************************************************************************************************************
 *  @file      tests/dwgio/drawing.cpp
 *  @brief     Finding an object of a drawing by its handle.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/drawing.hpp"

#include "support/fixtures.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"

#include <doctest/doctest.h>

#include <expected>
#include <variant>

namespace DwgIO
{
TEST_CASE("drawing: Find gives the object of a handle or null")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("foundation-r2000.dwg"));
  REQUIRE(loaded);
  const Drawing &drawing = loaded->Drawing;
  const Object *root = Find(drawing, drawing.Header.DictionaryNamedObjects);
  REQUIRE(root);
  CHECK(BaseOf(*root).Self == drawing.Header.DictionaryNamedObjects);
  CHECK(Holds<Objects::Dictionary>(*root));
  CHECK(Find(drawing, Handle{}) == nullptr);
  CHECK(Find(drawing, Handle{0xFFFFFF}) == nullptr);
  const Drawing empty;
  CHECK(Find(empty, Handle{1}) == nullptr);
}
} // namespace DwgIO
