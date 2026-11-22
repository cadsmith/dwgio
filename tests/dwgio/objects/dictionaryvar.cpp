/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/dictionaryvar.cpp
 *  @brief     The DICTIONARYVAR against its DXF twin, and refused without its class.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/dictionaryvar.hpp"

#include "fields/context.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/dictionaryvar: every variable of the fixture matches the DXF twin with DIMASSOC holding 2")
{
  const std::vector<Pair> twin = ReadTwin("objects-r2000.dxf");
  for (const std::string_view name : {"objects-r2000.dwg", "objects-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    const Drawing &drawing = loaded->Drawing;
    std::size_t count = 0;
    for (const Object &object : drawing.Objects)
    {
      const auto *variable = GetIf<DictionaryVar>(&object);
      if (variable == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(variable->Base.Self));
      CHECK(TypeOf(object) == "DICTIONARYVAR");
      // A variable is remade at each save, so the twin's is found by its name in the variables dictionary.
      const auto *owner = GetIf<Dictionary>(FindObject(drawing, variable->Base.Owner));
      REQUIRE(owner);
      const auto entry = std::ranges::find(owner->Entries, variable->Base.Self, &Entry::Object);
      REQUIRE(entry != owner->Entries.end());
      const std::optional<std::size_t> dictionary = FindHandled(twin, owner->Base.Self);
      REQUIRE(dictionary);
      const std::vector<std::pair<std::string, Handle>> entries = Entries(twin, *dictionary);
      const auto named = std::ranges::find_if(entries, [&](const std::pair<std::string, Handle> &one)
                                              { return SameName(one.first, entry->Name, drawing); });
      REQUIRE(named != entries.end());
      const std::optional<std::size_t> opened = FindHandled(twin, named->second);
      REQUIRE(opened);
      CHECK(variable->Schema == ReadWhole(twin, *opened, 280));
      CHECK(SameName(variable->Value, GroupOf(twin, *opened, 1), drawing));
    }
    CHECK(count == 18);
    const auto *root = GetIf<Dictionary>(FindObject(drawing, drawing.Header.DictionaryNamedObjects));
    REQUIRE(root);
    const Handle *variables = EntryNamed(*root, "AcDbVariableDictionary", drawing);
    REQUIRE(variables);
    const auto *dictionary = GetIf<Dictionary>(FindObject(drawing, *variables));
    REQUIRE(dictionary);
    const auto dimassoc = std::ranges::find(dictionary->Entries, "DIMASSOC", &Entry::Name);
    REQUIRE(dimassoc != dictionary->Entries.end());
    const auto *variable = GetIf<DictionaryVar>(FindObject(drawing, dimassoc->Object));
    REQUIRE(variable);
    CHECK(variable->Value == "2");
    CHECK(variable->Base.Owner == dictionary->Base.Self);
  }
}

TEST_CASE("objects/dictionaryvar: an object written without its class is dropped with an issue")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  DictionaryVar variable;
  variable.Base.Self = Handle{0x100};
  variable.Value = "1";
  CHECK(Objects::WriteObject(Object{variable}, with).empty());
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().Subject == Handle{0x100});
  CHECK(issues.front().What == "class DICTIONARYVAR not declared");
}
} // namespace DwgIO::Objects
