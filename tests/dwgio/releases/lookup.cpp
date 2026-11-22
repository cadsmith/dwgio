/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/lookup.cpp
 *  @brief     The lookups a translation makes: objects, owned entities and records by name.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/lookup.hpp"

#include "releases/remake.hpp"

#include "dwgio/base.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
TEST_CASE("releases/lookup: a handle lookup returns the first object with it like Find")
{
  Drawing drawing = Create(Version::R2000);
  const Handle line = *Edit::Add(drawing, Entities::Line{});
  Object twin{Entities::Line{}};
  BaseOf(twin).Self = line;
  drawing.Objects.push_back(twin);                // A handle borne twice, as a broken file holds.
  drawing.Objects.emplace_back(Entities::Line{}); // One of no handle.
  const Lookup found{drawing};
  CHECK(found.Of(line) == Find(drawing, line));
  CHECK(found.Of(Handle{}) == nullptr);
  CHECK(found.Of(Handle{0xFFFFFF}) == nullptr);
  CHECK(found.First(*Find(drawing, line)));
  CHECK_FALSE(found.First(drawing.Objects[drawing.Objects.size() - 2]));
  CHECK_FALSE(found.First(drawing.Objects.back()));
}

TEST_CASE("releases/lookup: an owner's entities exclude the BLOCK and ENDBLK and SEQEND and non-entities")
{
  Drawing drawing = Create(Version::R2000);
  const Handle model = drawing.Header.BlockRecordModelSpace;
  const Lookup before{drawing};
  const std::size_t had = before.CountOwned(model);
  static_cast<void>(*Edit::Add(drawing, Entities::Line{}));
  static_cast<void>(*Edit::Add(drawing, Entities::Line{}));
  Object begin{Entities::Block{}};
  BaseOf(begin).Owner = model;
  drawing.Objects.push_back(begin);
  const Lookup found{drawing};
  CHECK(found.CountOwned(model) == had + 2);
  CHECK(found.CountOwned(Handle{0xFFFFFF}) == 0);
  CHECK(found.CountOwned(drawing.Header.Clayer) == 0); // A layer owns no entity.
}

TEST_CASE("releases/lookup: an entity's first place in its owner's list is found")
{
  const Drawing drawing = Create(Version::R2004);
  const Lookup found{drawing};
  const std::vector<Handle> list{Handle{0x10}, Handle{0x11}, Handle{0x10}};
  CHECK(found.Place(Handle{0x1}, list, Handle{0x10}) == std::optional<std::size_t>{0});
  CHECK(found.Place(Handle{0x1}, list, Handle{0x11}) == std::optional<std::size_t>{1});
  CHECK_FALSE(found.Place(Handle{0x1}, list, Handle{0x12}));
}

TEST_CASE("releases/lookup: a table record is found by name in any case like View::Record")
{
  Drawing drawing = Create(Version::R2000);
  Tables::AppId appid;
  appid.Record.Name = "DWGIO";
  static_cast<void>(*Edit::Add(drawing, appid));
  static_cast<void>(*Edit::AddBlock(drawing, "PART"));
  const Lookup found{drawing};
  REQUIRE(found.Record<Tables::AppId>("dwgio") != nullptr);
  CHECK(found.Record<Tables::AppId>("dwgio")->Record.Name == "DWGIO");
  CHECK(found.Record<Tables::AppId>("NONE") == nullptr);
  REQUIRE(found.Record<Tables::BlockRecord>("part") != nullptr);
  CHECK(found.Record<Tables::BlockRecord>("part")->Record.Name == "PART");
  // The records a control keeps apart from its entries.
  CHECK(found.Record<Tables::BlockRecord>("*MODEL_SPACE") != nullptr);
  CHECK(found.Record<Tables::BlockRecord>("*NONE") == nullptr);
  // An entry naming nothing, or what is no record of the table, is passed over.
  Drawing broken = drawing;
  for (Object &object : broken.Objects)
  {
    if (auto *control = GetIf<Tables::Control>(&object);
        control != nullptr && (control->Table == Tables::Table::AppId || control->Table == Tables::Table::Block))
    {
      control->Entries.insert(control->Entries.begin(), {Handle{0xFFFFFF}, broken.Header.Clayer});
    }
  }
  CHECK(Lookup{broken}.Record<Tables::AppId>("DWGIO") != nullptr);
  CHECK(Lookup{broken}.Record<Tables::BlockRecord>("PART") != nullptr);
  // A drawing without the table.
  std::erase_if(broken.Objects, [](const Object &object) -> bool { return Holds<Tables::Control>(object); });
  CHECK(Lookup{broken}.Record<Tables::AppId>("DWGIO") == nullptr);
}

TEST_CASE("releases/lookup: the drawing is copied only when a pass first changes it")
{
  const Drawing drawing = Create(Version::R2000);
  Remake untouched{drawing};
  CHECK(&untouched.Now() == &drawing);
  CHECK_FALSE(std::move(untouched).Take());
  Remake redrawn{drawing};
  Drawing &made = redrawn.Change();
  CHECK(&made != &drawing);
  CHECK(&redrawn.Change() == &made); // Made once.
  CHECK(&redrawn.Now() == &made);
  CHECK(made == drawing);
  CHECK(made.Objects.capacity() > made.Objects.size());
  const std::optional<Drawing> taken = std::move(redrawn).Take();
  REQUIRE(taken);
  CHECK(*taken == drawing);
}
} // namespace DwgIO::Releases
