/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/block.cpp
 *  @brief     The BLOCK_RECORD against its DXF twin, written back, and cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/block.hpp"

#include "bits/reader.hpp"
#include "bits/reference.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"
#include "tables/shapes/block.hpp"

#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Tables
{
namespace
{
/**
 *  The entities of a block in the BLOCKS section, between its BLOCK and ENDBLK records, and those two.
 */
struct Defined
{
  Handle Block{};
  Handle End{};
  std::vector<Handle> Entities{};
  int Flags{};
  std::string Path{};
};

/**
 *  The block opened by the BLOCK entity with that handle, since a file names every paper space block alike.
 */
auto Block(const std::vector<Pair> &twin, Handle block) -> Defined
{
  Defined defined;
  std::size_t at = 0;
  for (; at < twin.size(); ++at)
  {
    if (twin[at].Code == 0 && twin[at].Value == "BLOCK" && Self(twin, at) == block)
    {
      break;
    }
  }
  REQUIRE(at < twin.size());
  defined.Block = Self(twin, at);
  defined.Flags = std::stoi(GroupOf(twin, at, 70));
  defined.Path = GroupOf(twin, at, 1);
  for (std::size_t inner = at + 1; inner < twin.size(); ++inner)
  {
    if (twin[inner].Code != 0)
    {
      continue;
    }
    if (twin[inner].Value == "ENDBLK")
    {
      defined.End = Self(twin, inner);
      break;
    }
    defined.Entities.push_back(Self(twin, inner));
  }
  return defined;
}

/**
 *  The entities of the ENTITIES section in one space: paper when the 67 group is 1, else model.
 */
auto CollectSpace(const std::vector<Pair> &twin, bool paper) -> std::vector<Handle>
{
  std::vector<Handle> handles;
  bool inside = false;
  for (std::size_t at = 0; at + 1 < twin.size(); ++at)
  {
    if (twin[at].Code == 2 && twin[at].Value == "ENTITIES" && twin[at - 1].Value == "SECTION")
    {
      inside = true;
      continue;
    }
    if (!inside || twin[at].Code != 0)
    {
      continue;
    }
    if (twin[at].Value == "ENDSEC")
    {
      break;
    }
    if ((GroupOf(twin, at, 67) == "1") == paper)
    {
      handles.push_back(Self(twin, at));
    }
  }
  return handles;
}

auto Inserts(const std::vector<Pair> &twin, std::string_view name) -> std::vector<Handle>
{
  std::vector<Handle> handles;
  for (std::size_t at = 0; at < twin.size(); ++at)
  {
    if (twin[at].Code == 0 && twin[at].Value == "INSERT" && Upper(GroupOf(twin, at, 2)) == Upper(name))
    {
      handles.push_back(Self(twin, at));
    }
  }
  return handles;
}
} // namespace

TEST_CASE("tables/block: the fixture's block records own what the DXF twin's own")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const bool modern = loaded->Drawing.Release >= Version::R2000;
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *record = GetIf<BlockRecord>(&object);
      if (record == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(record->Record.Name);
      CHECK(TypeOf(object) == "BLOCK_HEADER");
      // A file names every paper space block *Paper_Space; the twin numbers them, so the handle opens the records.
      const std::optional<std::size_t> opened = FindOpeningBy(twin, "BLOCK_RECORD", record->Record.Base.Self);
      REQUIRE(opened);
      if (!Upper(record->Record.Name).starts_with("*PAPER_SPACE"))
      {
        CHECK(Upper(GroupOf(twin, *opened, 2)) == Upper(record->Record.Name));
      }
      const Defined defined = Block(twin, record->Block);
      CHECK(record->EndBlk == defined.End);
      CHECK(record->Anonymous == ((defined.Flags & 1) != 0));
      CHECK(record->Attributes == ((defined.Flags & 2) != 0));
      CHECK(record->Xref == ((defined.Flags & 4) != 0));
      CHECK(record->Overlaid == ((defined.Flags & 8) != 0));
      CHECK(record->Path == defined.Path);
      CHECK(record->Origin == Vector{});
      std::vector<Handle> entities = defined.Entities;
      if (Upper(record->Record.Name) == "*MODEL_SPACE")
      {
        entities = CollectSpace(twin, false);
      }
      else if (Upper(GroupOf(twin, *opened, 2)) == "*PAPER_SPACE")
      {
        entities = CollectSpace(twin, true);
      }
      CHECK(record->First == (entities.empty() ? Handle{} : entities.front()));
      CHECK(record->Last == (entities.empty() ? Handle{} : entities.back()));
      if (!modern)
      {
        CHECK(record->Inserts.empty());
        CHECK(record->Layout == Handle{});
        continue;
      }
      CHECK(record->Inserts == Inserts(twin, record->Record.Name));
      CHECK(record->Layout == ParseHandle(GroupOf(twin, *opened, 340)).value_or(Handle{}));
      CHECK(record->Description.empty());
      CHECK(record->Preview.empty());
      CHECK_FALSE(record->Loaded);
    }
    CHECK(count == 5);
  }
}

TEST_CASE("tables/block: the chair block holds a line and a circle and is inserted once")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("tables-r2000.dwg"));
  REQUIRE(loaded);
  for (const Object &object : loaded->Drawing.Objects)
  {
    const auto *record = GetIf<BlockRecord>(&object);
    if (record == nullptr || record->Record.Name != "Chair")
    {
      continue;
    }
    CHECK(record->Inserts.size() == 3);
    CHECK(record->First != record->Last);
    const auto typed = [&loaded](Handle self) -> std::string_view
    {
      for (const Object &one : loaded->Drawing.Objects)
      {
        if (BaseOf(one).Self == self)
        {
          return TypeOf(one);
        }
      }
      return {};
    };
    CHECK(typed(record->First) == "LINE");
    CHECK(typed(record->Last) == "CIRCLE");
    CHECK(typed(record->Block) == "BLOCK");
    CHECK(typed(record->EndBlk) == "ENDBLK");
    CHECK(typed(record->Inserts.front()) == "INSERT");
    CHECK(record->Layout == Handle{});
    return;
  }
  FAIL("no Chair block");
}

TEST_CASE("tables/block: an xref and an overlay and a block with a preview and inserts round-trip")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  BlockRecord record;
  record.Record.Base.Self = Handle{0x300};
  record.Record.Base.Owner = Handle{0x1};
  record.Record.Name = "Plan";
  record.Xref = true;
  record.Loaded = true;
  record.Path = "plan.dwg";
  record.Description = "the site plan";
  record.Preview = {std::byte{1}, std::byte{2}, std::byte{3}};
  record.Block = Handle{0x301};
  record.EndBlk = Handle{0x302};
  record.Inserts = {Handle{0x2EF}, Handle{0x2F0}};
  record.Layout = Handle{0x22};
  std::optional<Object> read = Objects::ReadObject(Objects::WriteObject(Object{record}, with), 0, reading);
  REQUIRE(read);
  CHECK(*read == Object{record});
  record.Xref = false;
  record.Overlaid = true;
  read = Objects::ReadObject(Objects::WriteObject(Object{record}, with), 0, reading);
  REQUIRE(read);
  CHECK(*read == Object{record});
  CHECK(issues.empty());
}

TEST_CASE("tables/block: a record ending inside its insert run is kept with what was read and noted")
{
  // The data reader spans the whole object, so a run can only run out where the object ends: no handles follow.
  const auto build = [](std::uint32_t handles, std::uint32_t *start) -> std::vector<std::byte>
  {
    Bits::Writer data;
    data.BS(49);
    data.RL(handles);
    data.H(Bits::Reference{.Code = 0, .Value = Handle{0xD6}});
    data.BS(0);
    data.BL(0);
    data.TV("Cut", Version::R2000, Objects::Latin);
    data.B(false);
    data.BS(0);
    data.B(false);
    for (int bit = 0; bit < 5; ++bit)
    {
      data.B(false);
    }
    data.BD(0);
    data.BD(0);
    data.BD(0);
    data.TV("", Version::R2000, Objects::Latin);
    data.RC(1);
    data.Fill();
    *start = static_cast<std::uint32_t>(data.Position());
    return Objects::FrameObject(data);
  };
  std::uint32_t start = 0;
  static_cast<void>(build(0, &start));
  std::vector<Issue> issues;
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> read = Objects::ReadObject(build(start, &start), 0, reading);
  REQUIRE(read);
  REQUIRE(Holds<BlockRecord>(*read));
  const auto &record = Get<BlockRecord>(*read);
  CHECK(record.Record.Name == "Cut");
  CHECK(record.Inserts.size() == 1);
  REQUIRE(issues.size() == 2);
  CHECK(issues[0].What == "object D6 data cut short");
  CHECK(issues[1].What == "object D6 handles cut short");
}

namespace DwgIO
{
TEST_CASE("tables/block: an owned count the data cannot hold is refused before allocating")
{
  Bits::Writer out;
  out.BL(0x7FFFFFFF);
  Bits::Reader in{out.Bytes()};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2004, .Text = &Objects::Latin, .Issues = &issues};
  const Fields::Input reading{.In = &in, .Handles = &in, .With = with};
  Tables::BlockRecord record;
  CHECK_FALSE(Tables::Shapes::ReadOwnedCount(record, reading));
  CHECK(record.Entities.empty());
  Bits::Writer two;
  two.BL(2);
  two.RL(0);
  Bits::Reader fine{two.Bytes()};
  const Fields::Input sane{.In = &fine, .Handles = &fine, .With = with};
  CHECK(Tables::Shapes::ReadOwnedCount(record, sane));
  CHECK(record.Entities.size() == 2);
}
} // namespace DwgIO
} // namespace DwgIO::Tables
