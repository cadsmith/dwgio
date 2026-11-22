/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/group.cpp
 *  @brief     The GROUP against its DXF twin, and written back as R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/group.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
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
TEST_CASE("objects/group: every group of the fixtures matches the DXF twin including an unnamed one")
{
  for (const std::string_view fixture : {"objects", "entities"})
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
      std::size_t unnamed = 0;
      for (const Object &object : drawing.Objects)
      {
        const auto *group = GetIf<Group>(&object);
        if (group == nullptr)
        {
          continue;
        }
        ++count;
        CAPTURE(HexOf(group->Base.Self));
        CHECK(TypeOf(object) == "GROUP");
        const std::optional<std::size_t> opened = FindHandled(twin, group->Base.Self);
        REQUIRE(opened);
        CHECK(SameName(group->Description, GroupOf(twin, *opened, 300), drawing));
        CHECK(group->Unnamed == ReadWhole(twin, *opened, 70));
        CHECK(group->Selectable == ReadWhole(twin, *opened, 71));
        std::vector<Handle> members;
        for (const std::string &member : Every(twin, *opened, 340))
        {
          members.push_back(ParseHandle(member).value_or(Handle{}));
        }
        CHECK(group->Members == members);
        CHECK(group->Base.Owner == drawing.Header.DictionaryAcadGroup);
        unnamed += group->Unnamed == 1 ? 1U : 0U;
      }
      CHECK(count == (fixture == "objects" ? 2 : 1));
      CHECK(unnamed == (fixture == "objects" ? 1 : 0));
    }
  }
}

TEST_CASE("objects/group: a group of three members round-trips as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const Objects::FileContext reading{.Release = Version::R2000, .Text = &Latin, .Classes = {}, .Issues = &issues};
  Group group;
  group.Base.Self = Handle{0x100};
  group.Base.Owner = Handle{0xD};
  group.Description = "three";
  group.Unnamed = 1;
  group.Selectable = 0;
  group.Members = {Handle{0x50}, Handle{0x51}, Handle{0x200}};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{group}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{group});
  CHECK(TypeOf(*back) == "GROUP");
  CHECK(issues.empty());
}
} // namespace DwgIO::Objects
