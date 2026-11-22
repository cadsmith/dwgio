/***********************************************************************************************************************
 *  @file      tests/dwgio/save/objects.cpp
 *  @brief     The objects a save writes: the map that lists them and the chains that link them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/coded/layout.hpp"
#include "file/locator/layout.hpp"
#include "file/paged/layout.hpp"
#include "objects/map.hpp"
#include "support/fixtures.hpp"

#include "dwgio/class.hpp"
#include "dwgio/common.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace DwgIO
{
namespace
{
auto MapEntries(std::span<const std::byte> bytes) -> std::size_t
{
  std::vector<Issue> issues;
  const auto layout = File::Locator::ReadLayout(bytes, issues);
  REQUIRE(layout);
  for (const File::Locator::Located &block : layout->Blocks)
  {
    if (block.Name == "map")
    {
      return Objects::ReadMap(block.Bytes, block.Offset, issues).size();
    }
  }
  return 0;
}
} // namespace

TEST_CASE("save/objects: a drawing with one object fewer loads with one fewer and a matching map")
{
  Drawing drawing = LoadDrawing("foundation-r2000.dwg");
  const auto line = std::ranges::find_if(drawing.Objects, [](const Object &one) { return TypeOf(one) == "LINE"; });
  REQUIRE(line != drawing.Objects.end());
  drawing.Objects.erase(line);
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  CHECK(loaded->Drawing.Objects.size() == 218);
  CHECK(MapEntries(saved->Bytes) == 218);
  CHECK(loaded->Drawing.Header == drawing.Header);
  CHECK(loaded->Drawing.Classes == drawing.Classes);
  CHECK(loaded->Drawing.Preview == drawing.Preview);
  CHECK(loaded->Drawing.Objects == drawing.Objects);
}

TEST_CASE("save/objects: a drawing without kept blocks or an order still saves and loads")
{
  Drawing drawing = LoadDrawing("foundation-r2000.dwg");
  drawing.Kept.clear();
  drawing.Order.clear();
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  CHECK(loaded->Drawing.Objects == drawing.Objects);
  CHECK(loaded->Drawing.Order ==
        std::vector<std::string>{"preview", "header", "classes", "pad", "objects", "map", "second", "measurement"});
  CHECK(loaded->Drawing.Kept.size() == 3);
}

TEST_CASE("save/objects: a chain cut before several entities leaves them all out of the list and notes how many")
{
  Drawing older = Create();
  const Handle one = *Edit::Add(older, Entities::Line{});
  REQUIRE(Edit::Add(older, Entities::Line{}));
  REQUIRE(Edit::Add(older, Entities::Line{}));
  CommonOf(*Find(older, one))->Next = Handle{.Value = 0x999};
  const std::expected<Saved, Failure> raised = Save(older, SaveOptions{.Release = Version::R2004});
  REQUIRE(raised);
  REQUIRE(raised->Issues.size() == 1);
  CHECK(raised->Issues[0].What.starts_with("2 entities of BLOCK_HEADER "));
  CHECK(raised->Issues[0].What.ends_with(" not on its chain, left out of its list"));
  CHECK(View::Block(Load(raised->Bytes).value().Drawing, "*Model_Space")->Entities == std::vector<Handle>{one});
}
} // namespace DwgIO
