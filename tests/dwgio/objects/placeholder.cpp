/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/placeholder.cpp
 *  @brief     The ACDBPLACEHOLDER as the plot style dictionary's Normal entry.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/placeholder.hpp"

#include "support/fixtures.hpp"
#include "support/objects.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/placeholder: the fixture's placeholder is the Normal entry of the plot style dictionary")
{
  for (const std::string_view name : {"objects-r2000.dwg", "objects-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    const Drawing &drawing = loaded->Drawing;
    const auto count =
        std::ranges::count_if(drawing.Objects, [](const Object &one) { return Holds<Placeholder>(one); });
    CHECK(count == 1);
    // R14's header has no plot style dictionary handle; the root reaches it in both.
    const auto *root = GetIf<Dictionary>(FindObject(drawing, drawing.Header.DictionaryNamedObjects));
    REQUIRE(root);
    const Handle *named = EntryNamed(*root, "ACAD_PLOTSTYLENAME", drawing);
    REQUIRE(named);
    const auto *styles = GetIf<Dictionary>(FindObject(drawing, *named));
    REQUIRE(styles);
    if (drawing.Release >= Version::R2000)
    {
      CHECK(drawing.Header.DictionaryPlotStyles == *named);
    }
    REQUIRE(styles->Default);
    if (drawing.Release >= Version::R2000)
    {
      // R14 keeps the entry under an empty name.
      const Handle *normal = EntryNamed(*styles, "Normal", drawing);
      REQUIRE(normal);
      CHECK(*styles->Default == *normal);
    }
    const Object *object = FindObject(drawing, *styles->Default);
    REQUIRE(object);
    CHECK(TypeOf(*object) == "ACDBPLACEHOLDER");
    const auto *placeholder = GetIf<Placeholder>(object);
    REQUIRE(placeholder);
    CHECK(placeholder->Base.Owner == styles->Base.Self);
    CHECK(placeholder->Base.Reactors == std::vector{styles->Base.Self});
  }
}
} // namespace DwgIO::Objects
