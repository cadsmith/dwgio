/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/dictionary.cpp
 *  @brief     The DICTIONARY against its DXF twin, written back, and its counts cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/dictionary.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/read.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/dictionary.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"

#include "dwgio/class.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/dictionary: every dictionary of the fixture matches the DXF twin with dwgio's own reachable")
{
  const std::vector<Pair> twin = ReadTwin("objects-r2000.dxf");
  for (const std::string_view name : {"objects-r2000.dwg", "objects-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    std::size_t count = 0;
    std::size_t defaulted = 0;
    std::size_t regenerated = 0;
    for (const Object &object : drawing.Objects)
    {
      const auto *dictionary = GetIf<Dictionary>(&object);
      if (dictionary == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(dictionary->Base.Self));
      CHECK(TypeOf(object) == (dictionary->Default ? "ACDBDICTIONARYWDFLT" : "DICTIONARY"));
      const std::optional<std::size_t> opened = FindHandled(twin, dictionary->Base.Self);
      if (!opened)
      {
        // AutoCAD remakes some objects at each save, under new handles: the twin, saved last, has them elsewhere.
        ++regenerated;
        continue;
      }
      // R14 owns its plot style names hard where R2000 does not; the rest agree.
      if (drawing.Release >= Version::R2000 || !dictionary->Default)
      {
        CHECK(dictionary->HardOwner == (ReadWhole(twin, *opened, 280) == 1));
      }
      CHECK(dictionary->Cloning == ReadWhole(twin, *opened, 281));
      const std::vector<std::pair<std::string, Handle>> expected = Entries(twin, *opened);
      // An R14 save keeps entries of its own in the root; every entry of the twin is there in both.
      if (drawing.Release >= Version::R2000)
      {
        CHECK(dictionary->Entries.size() == expected.size());
      }
      for (const auto &[entryName, target] : expected)
      {
        CAPTURE(entryName);
        const Handle *found = EntryNamed(*dictionary, entryName, drawing);
        if (found == nullptr && drawing.Release < Version::R2000)
        {
          // R14 keeps the plot style Normal under an empty name: the handle says which entry it is.
          const auto byHandle = std::ranges::find(dictionary->Entries, target, &Entry::Object);
          found = byHandle == dictionary->Entries.end() ? nullptr : &byHandle->Object;
        }
        // An entry's object remade at each save has another handle in the twin, and an R14 save keeps fewer
        // of them; the entries of objects the drawing holds agree.
        if (FindObject(drawing, target) == nullptr)
        {
          CHECK((found != nullptr || drawing.Release < Version::R2000));
          continue;
        }
        REQUIRE(found);
        CHECK(*found == target);
      }
      if (dictionary->Default)
      {
        ++defaulted;
        CHECK(*dictionary->Default == ParseHandle(GroupOf(twin, *opened, 340)).value_or(Handle{}));
      }
    }
    CHECK(count >= 63);
    CHECK(defaulted == 1);
    CHECK(regenerated == (drawing.Release >= Version::R2000 ? 33 : 60));
    const auto *root = GetIf<Dictionary>(FindObject(drawing, drawing.Header.DictionaryNamedObjects));
    REQUIRE(root);
    const Handle *own = EntryNamed(*root, "DWGIO_DICT", drawing);
    REQUIRE(own);
    const auto *dictionary = GetIf<Dictionary>(FindObject(drawing, *own));
    REQUIRE(dictionary);
    CHECK_FALSE(dictionary->HardOwner);
    CHECK(dictionary->Base.Owner == root->Base.Self);
    CHECK(dictionary->Entries.size() == 3);
    CHECK(EntryNamed(*dictionary, "FULL", drawing));
    CHECK(EntryNamed(*dictionary, "EMPTY", drawing));
    const Handle *inner = EntryNamed(*dictionary, "INNER", drawing);
    REQUIRE(inner);
    const auto *nested = GetIf<Dictionary>(FindObject(drawing, *inner));
    REQUIRE(nested);
    CHECK(nested->Base.Owner == dictionary->Base.Self);
    CHECK(nested->Entries.size() == 1);
    const Handle *hard = EntryNamed(*root, "DWGIO_HARD", drawing);
    REQUIRE(hard);
    const auto *owning = GetIf<Dictionary>(FindObject(drawing, *hard));
    REQUIRE(owning);
    CHECK(owning->HardOwner);
  }
}

TEST_CASE("objects/dictionary: dictionaries of three entries round-trip as R2000")
{
  std::vector<Issue> issues;
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "ACDBDICTIONARYWDFLT"}};
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{.Release = Version::R2000, .Text = &Latin, .Classes = classes, .Issues = &issues};
  Dictionary dictionary;
  dictionary.Base.Self = Handle{0x100};
  dictionary.Base.Owner = Handle{0xC};
  dictionary.Base.Reactors = {Handle{0xC}};
  dictionary.HardOwner = true;
  dictionary.Cloning = 2;
  dictionary.Entries = {Entry{.Name = "ONE", .Object = Handle{0x101}}, Entry{.Name = "TWO", .Object = Handle{0x102}},
                        Entry{.Name = "THREE", .Object = Handle{0x50}}};
  std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{dictionary}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{dictionary});
  CHECK(TypeOf(*back) == "DICTIONARY");
  dictionary.Default = Handle{0x101};
  back = Objects::ReadObject(Objects::WriteObject(Object{dictionary}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{dictionary});
  CHECK(TypeOf(*back) == "ACDBDICTIONARYWDFLT");
  CHECK(issues.empty());
}

TEST_CASE("objects/dictionary: an oversized count or a default without its handle fails with an issue")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const auto read = [&](const std::vector<std::byte> &data, bool defaulted) -> bool
  {
    const std::vector<std::byte> none;
    Bits::Reader in{data};
    Bits::Reader stream{none};
    Fields::Input reading{.In = &in, .Handles = &stream, .With = with};
    Dictionary dictionary;
    if (defaulted)
    {
      Objects::Shape<Dictionary>::Open(dictionary, "ACDBDICTIONARYWDFLT");
    }
    return Fields::Read(Objects::Shape<Dictionary>::Layout(), dictionary, reading);
  };
  Bits::Writer big;
  big.BL(1000000);
  big.BS(1);
  big.RC(0);
  CHECK_FALSE(read({big.Bytes().begin(), big.Bytes().end()}, false));
  Bits::Writer negative;
  negative.BL(-1);
  negative.BS(1);
  negative.RC(0);
  CHECK_FALSE(read({negative.Bytes().begin(), negative.Bytes().end()}, false));
  REQUIRE(issues.size() == 2);
  CHECK(issues[0].What == "dictionary count 1000000 out of range");
  CHECK(issues[1].What == "dictionary count -1 out of range");
  Bits::Writer empty;
  empty.BL(0);
  empty.BS(1);
  empty.RC(0);
  CHECK(read({empty.Bytes().begin(), empty.Bytes().end()}, false));
  CHECK_FALSE(read({empty.Bytes().begin(), empty.Bytes().end()}, true));
  CHECK(issues.size() == 2);
}
} // namespace DwgIO::Objects
