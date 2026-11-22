/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/sortents.cpp
 *  @brief     The SORTENTSTABLE against its DXF twin, written back, and cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/sortents.hpp"

#include "bits/reader.hpp"
#include "bits/reference.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/read.hpp"
#include "objects/shape.hpp"
#include "objects/shapes/sortents.hpp"
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
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/sortents: the fixtures' sort tables match the DXF twin")
{
  for (const std::string_view fixture : {"objects", "curves"})
  {
    const std::vector<Pair> twin = ReadTwin(std::string{fixture} + "-r2000.dxf");
    for (const std::string_view release : {"-r2000.dwg", "-r14.dwg"})
    {
      const std::string name = std::string{fixture} + std::string{release};
      CAPTURE(name);
      const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
      REQUIRE(loaded);
      CHECK(loaded->Issues.empty());
      const Drawing &drawing = loaded->Drawing;
      std::size_t count = 0;
      for (const Object &object : drawing.Objects)
      {
        const auto *table = GetIf<SortEntsTable>(&object);
        if (table == nullptr)
        {
          continue;
        }
        ++count;
        CAPTURE(HexOf(table->Base.Self));
        CHECK(TypeOf(object) == "SORTENTSTABLE");
        const std::optional<std::size_t> opened = FindHandled(twin, table->Base.Self);
        REQUIRE(opened);
        // The block's 330 follows the subclass marker; the reactors' brace holds a 330 of its own before it.
        Handle block;
        std::vector<SortEntsTables::Entry> entries;
        bool inside = false;
        for (std::size_t at = *opened + 1; at + 1 < twin.size() && twin[at].Code != 0; ++at)
        {
          if (twin[at].Code == 100)
          {
            inside = true;
          }
          else if (inside && twin[at].Code == 330)
          {
            block = ParseHandle(twin[at].Value).value_or(Handle{});
          }
          else if (inside && twin[at].Code == 331 && twin[at + 1].Code == 5)
          {
            entries.push_back(SortEntsTables::Entry{.Entity = ParseHandle(twin[at].Value).value_or(Handle{}),
                                                    .Key = ParseHandle(twin[at + 1].Value).value_or(Handle{})});
          }
        }
        CHECK(table->Block == block);
        CHECK(table->Block == drawing.Header.BlockRecordModelSpace);
        CHECK(table->Entries == entries);
        CHECK(table->Entries.size() == (fixture == "curves" ? 13 : 0));
      }
      CHECK(count == 1);
    }
  }
}

TEST_CASE("objects/sortents: a table of two entries round-trips under its class")
{
  std::vector<Issue> issues;
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "SORTENTSTABLE"}};
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{.Release = Version::R2000, .Text = &Latin, .Classes = classes, .Issues = &issues};
  SortEntsTable table;
  table.Base.Self = Handle{0x100};
  table.Base.Owner = Handle{0x1CE};
  table.Block = Handle{0x1F};
  table.Entries = {SortEntsTables::Entry{.Entity = Handle{0x50}, .Key = Handle{0x51}},
                   SortEntsTables::Entry{.Entity = Handle{0x51}, .Key = Handle{0x50}}};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{table}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{table});
  CHECK(TypeOf(*back) == "SORTENTSTABLE");
  CHECK(issues.empty());
}

TEST_CASE("objects/sortents: a cut or oversized count or a short handle stream fails with the table kept")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const auto read = [&](const std::vector<std::byte> &data, const std::vector<std::byte> &handles) -> bool
  {
    Bits::Reader in{data};
    Bits::Reader stream{handles};
    Fields::Input reading{.In = &in, .Handles = &stream, .With = with};
    SortEntsTable table;
    return Fields::Read(Objects::Shape<SortEntsTable>::Layout(), table, reading);
  };
  CHECK_FALSE(read({}, {}));
  Bits::Writer big;
  big.BL(1000);
  CHECK_FALSE(read({big.Bytes().begin(), big.Bytes().end()}, {}));
  Bits::Writer negative;
  negative.BL(-1);
  CHECK_FALSE(read({negative.Bytes().begin(), negative.Bytes().end()}, {}));
  REQUIRE(issues.size() == 2);
  CHECK(issues[0].What == "sort table count 1000 out of range");
  CHECK(issues[1].What == "sort table count -1 out of range");
  // A key whose counter says four bytes where one is left fails the data before the handles.
  Bits::Writer keyless;
  keyless.BL(1);
  keyless.RC(0x04);
  keyless.RC(0x50);
  CHECK_FALSE(read({keyless.Bytes().begin(), keyless.Bytes().end()}, {}));
  Bits::Writer one;
  one.BL(1);
  one.H(Bits::Reference{.Code = 0, .Value = Handle{0x50}});
  one.Fill();
  CHECK_FALSE(read({one.Bytes().begin(), one.Bytes().end()}, {}));
  Bits::Writer block;
  block.H(Bits::Reference{.Code = 4, .Value = Handle{0x1F}});
  block.Fill();
  CHECK_FALSE(read({one.Bytes().begin(), one.Bytes().end()}, {block.Bytes().begin(), block.Bytes().end()}));
  CHECK(issues.size() == 2);
}
} // namespace DwgIO::Objects
