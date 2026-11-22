/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/names.cpp
 *  @brief     Names R14 cannot hold, cut when saved as R14 and restored on upgrading, as AutoCAD does.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/names.hpp"

#include "support/acad.hpp"
#include "support/fixtures.hpp"
#include "support/pass.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/vport.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
const Tag Marker{.Code = 102, .Value = std::string{"EXTNAMES"}};

/**
 *  The round trip record of an object's extension dictionary, or null.
 */
auto RecordOf(const Drawing &drawing, const Object &object) -> const Objects::XRecord *
{
  const auto *dictionary = GetIf<Objects::Dictionary>(Find(drawing, BaseOf(object).Dictionary));
  return dictionary == nullptr ? nullptr
                               : GetIf<Objects::XRecord>(View::Entry(drawing, *dictionary, "ACAD_XREC_ROUNDTRIP"));
}

/**
 *  The EXTNAMES records of every dictionary but the root, as their groups, sorted.
 */
auto Extnames(const Drawing &drawing) -> std::vector<std::vector<Tag>>
{
  std::vector<std::vector<Tag>> records;
  for (const Object &object : drawing.Objects)
  {
    const Objects::XRecord *record =
        Holds<Objects::Dictionary>(object) && BaseOf(object).Self != drawing.Header.DictionaryNamedObjects
            ? RecordOf(drawing, object)
            : nullptr;
    if (record != nullptr && !record->Data.empty() && record->Data.front() == Marker)
    {
      records.push_back(record->Data);
    }
  }
  std::ranges::sort(records, {}, [](const std::vector<Tag> &groups) { return std::get<std::string>(groups[2].Value); });
  return records;
}

/**
 *  The names of every dictionary's entries, sorted.
 */
auto Entries(const Drawing &drawing) -> std::vector<std::string>
{
  std::vector<std::string> names;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *dictionary = GetIf<Objects::Dictionary>(&object))
    {
      for (const Objects::Entry &entry : dictionary->Entries)
      {
        names.push_back(entry.Name);
      }
    }
  }
  std::ranges::sort(names);
  return names;
}
} // namespace

TEST_CASE("releases/names: an R14 name is upper case with other characters than $-_* made underscores")
{
  CHECK(Narrow("Shaded with edges") == "SHADED_WITH_EDGES");
  CHECK(Narrow("X-Ray") == "X-RAY");
  CHECK(Narrow("*Model_Space") == "*MODEL_SPACE");
  CHECK(Narrow("a$b") == "A$B");
  CHECK(Narrow("Leve.pdf") == "LEVE_PDF");
  CHECK(Narrow("Caf\xc3\xa9 \xe3\x82\xbf") == "CAF___");
  // Kept whole to 31 characters, cut to 26 beyond.
  CHECK(Narrow("Generic 24in x 36in Title Block") == "GENERIC_24IN_X_36IN_TITLE_BLOCK");
  CHECK(Narrow("ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP") == "ACAD_ROUNDTRIP_2008_TABLES");
  CHECK(Narrow("{f72375cc-4363-4d63-987e-ec82646fff03}") == "_F72375CC-4363-4D63-987E-E");
}

TEST_CASE("releases/names: saving as R14 records each dictionary's changed names")
{
  for (const std::string_view name : {"tables", "cells", "kinds", "visualstyles"})
  {
    CAPTURE(name);
    const Drawing drawing = Load(ReadFixture(std::string{name} + "-r2000.dwg")).value().Drawing;
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R14});
    REQUIRE(saved);
    const Drawing ours = Load(saved->Bytes).value().Drawing;
    const Drawing theirs = Load(ReadFixture(std::string{name} + "-r14.dwg")).value().Drawing;
    // AutoCAD's root keeps the auxiliary header's round trip data as well, which dwgio does not make.
    CHECK(Extnames(ours) == Extnames(theirs));
    CHECK_FALSE(Extnames(ours).empty());
    // Upgraded again, every name comes back whole.
    const Drawing back = Load(Save(ours, SaveOptions{.Release = Version::R2000})->Bytes).value().Drawing;
    CHECK(Entries(back) == Entries(drawing));
    QueueAudit(saved->Bytes, std::format("dwgio-renamed-{}-as-R14.dwg", name));
    if (IsVerifying())
    {
      // AutoCAD reads the names back whole.
      const std::expected<Loaded, Failure> loaded = Load(Resave(saved->Bytes, std::format("dwgio-renamed-{}", name)));
      REQUIRE(loaded);
      const Drawing &read = loaded->Drawing;
      const Objects::Dictionary *root = View::Root(read);
      REQUIRE(root != nullptr);
      CHECK(std::ranges::contains(root->Entries, std::string{"AcDbVariableDictionary"}, &Objects::Entry::Name));
      CHECK(std::ranges::contains(Entries(read), std::string{"ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP"}));
    }
  }
}

TEST_CASE("releases/names: saving as R14 keeps the names AutoCAD keeps and existing records")
{
  Drawing drawing = Load(ReadFixture("empty-r2000.dwg")).value().Drawing;
  const Handle root = drawing.Header.DictionaryNamedObjects;
  // A dictionary with a default keeps its names; so does the one a dynamic block keeps under its data's entry.
  const Handle owner = Edit::MakeHandle(drawing);
  const Handle data = Edit::MakeHandle(drawing);
  const Handle edit = Edit::MakeHandle(drawing);
  const Handle plain = Edit::MakeHandle(drawing);
  const Handle kept = Edit::MakeHandle(drawing);
  const Handle holder = Edit::MakeHandle(drawing);
  const Handle record = Edit::MakeHandle(drawing);
  const Handle spare = Edit::MakeHandle(drawing);
  // The owner's extension dictionary holds no round trip record.
  drawing.Objects.emplace_back(Objects::Dictionary{.Base = {.Self = spare, .Owner = owner},
                                                   .HardOwner = true,
                                                   .Cloning = 1,
                                                   .Entries = {{.Name = "OTHER", .Object = edit}},
                                                   .Default = {}});
  drawing.Objects.emplace_back(Objects::Dictionary{
      .Base = {.Self = owner, .Owner = root, .Dictionary = spare},
      .HardOwner = true,
      .Cloning = 1,
      .Entries = {{.Name = "AcDbBlockTableRecordData", .Object = data}, {.Name = "Plain one", .Object = plain}},
      .Default = {}});
  drawing.Objects.emplace_back(Objects::Dictionary{.Base = {.Self = data, .Owner = owner},
                                                   .HardOwner = true,
                                                   .Cloning = 1,
                                                   .Entries = {{.Name = "Edit", .Object = edit}},
                                                   .Default = {}});
  drawing.Objects.emplace_back(Objects::XRecord{.Base = {.Self = edit, .Owner = data}});
  // A dictionary whose extension dictionary has a round trip record already, and one whose has none.
  drawing.Objects.emplace_back(Objects::Dictionary{.Base = {.Self = plain, .Owner = owner, .Dictionary = holder},
                                                   .HardOwner = true,
                                                   .Cloning = 1,
                                                   .Entries = {{.Name = "Lower", .Object = edit}},
                                                   .Default = {}});
  drawing.Objects.emplace_back(Objects::Dictionary{.Base = {.Self = holder, .Owner = plain},
                                                   .HardOwner = true,
                                                   .Cloning = 1,
                                                   .Entries = {{.Name = "ACAD_XREC_ROUNDTRIP", .Object = record}},
                                                   .Default = {}});
  drawing.Objects.emplace_back(Objects::XRecord{.Base = {.Self = record, .Owner = holder},
                                                .Data = {{.Code = 102, .Value = std::string{"UCS"}}}});
  drawing.Objects.emplace_back(Objects::Dictionary{.Base = {.Self = kept, .Owner = owner},
                                                   .HardOwner = true,
                                                   .Cloning = 1,
                                                   .Entries = {{.Name = "UPPER", .Object = edit}},
                                                   .Default = {}});
  Get<Objects::Dictionary>(*Find(drawing, root)).Entries.push_back({.Name = "Mine", .Object = owner});
  const std::optional<Drawing> made = Pass(NarrowNames, drawing, Version::R14);
  REQUIRE(made);
  CHECK(Get<Objects::Dictionary>(*Find(*made, data)).Entries.front().Name == "Edit");
  CHECK(Get<Objects::Dictionary>(*Find(*made, owner)).Entries.front().Name == "ACDBBLOCKTABLERECORDDATA");
  const auto *plot = View::Dictionary(*made, "ACAD_PLOTSTYLENAME");
  REQUIRE(plot != nullptr);
  CHECK(plot->Entries.front().Name == "Normal");
  // The record already there takes the names first; its dictionary keeps its entries.
  const auto &filled = Get<Objects::XRecord>(*Find(*made, record));
  CHECK(filled.Data == std::vector<Tag>{Marker,
                                        {.Code = 1, .Value = std::string{"LOWER"}},
                                        {.Code = 2, .Value = std::string{"Lower"}},
                                        {.Code = 102, .Value = std::string{"UCS"}}});
  // The owner's record made under its extension dictionary, which keeps its entry.
  const Objects::XRecord *named = RecordOf(*made, *Find(*made, owner));
  REQUIRE(named != nullptr);
  CHECK(named->Base.Owner == spare);
  CHECK(Get<Objects::Dictionary>(*Find(*made, spare)).Entries.size() == 2);
  CHECK(named->Data.at(2) == Tag{.Code = 2, .Value = std::string{"AcDbBlockTableRecordData"}});
  CHECK(named->Base.Reactors == std::vector<Handle>{named->Base.Owner});
  // Nothing to change when saving R14 as R14, when upgrading, or in a drawing of R14.
  CHECK_FALSE(Pass(NarrowNames, drawing, Version::R2000));
  CHECK_FALSE(Pass(NarrowNames, Load(ReadFixture("empty-r14.dwg")).value().Drawing, Version::R14));
  // A drawing whose names R14 holds as they are.
  Drawing upper = Load(ReadFixture("empty-r14.dwg")).value().Drawing;
  upper.Release = Version::R2000;
  CHECK_FALSE(Pass(NarrowNames, upper, Version::R14));
}

TEST_CASE("releases/names: upgrading from R14 restores the names and drops their records")
{
  const Drawing drawing = Load(ReadFixture("tables-r14.dwg")).value().Drawing;
  const std::optional<Drawing> made = Pass(WidenNames, drawing, Version::R2000);
  REQUIRE(made);
  for (const Object &object : made->Objects)
  {
    const auto *record = GetIf<Objects::XRecord>(&object);
    CHECK((record == nullptr || std::ranges::find(record->Data, Marker) == record->Data.end()));
  }
  CHECK(
      std::ranges::contains(View::Root(*made)->Entries, std::string{"AcDbVariableDictionary"}, &Objects::Entry::Name));
  CHECK(View::Record<Tables::Layer>(*made, "Walls")->Record.Name == "Walls");
  // A record keeps what else it holds; a block's BLOCK takes its name.
  const auto *active = View::Record<Tables::VPort>(*made, "*Active");
  REQUIRE(active != nullptr);
  CHECK(active->Record.Name == "*Active");
  REQUIRE(RecordOf(*made, *Find(*made, active->Record.Base.Self)) != nullptr);
  const auto *model = View::Record<Tables::BlockRecord>(*made, "*Model_Space");
  REQUIRE(model != nullptr);
  CHECK(model->Record.Name == "*Model_Space");
  CHECK(Get<Entities::Block>(*Find(*made, model->Block)).Name == "*Model_Space");
  // A block record whose BLOCK is not there is named all the same.
  Drawing blockless = drawing;
  for (Object &object : blockless.Objects)
  {
    if (auto *record = GetIf<Tables::BlockRecord>(&object))
    {
      record->Block = {};
    }
  }
  const std::optional<Drawing> named = Pass(WidenNames, blockless, Version::R2000);
  REQUIRE(named);
  CHECK(View::Record<Tables::BlockRecord>(*named, "*Model_Space")->Record.Name == "*Model_Space");
  CHECK(made->Objects.size() < drawing.Objects.size());
  CHECK_FALSE(Pass(WidenNames, drawing, Version::R14));
  CHECK_FALSE(Pass(WidenNames, Load(ReadFixture("tables-r2000.dwg")).value().Drawing, Version::R2018));
  // A pair of other codes is no name.
  Drawing odd = drawing;
  for (Object &object : odd.Objects)
  {
    auto *record = GetIf<Objects::XRecord>(&object);
    if (record != nullptr && !record->Data.empty() && record->Data.front() == Marker)
    {
      record->Data = {Marker,
                      {.Code = 3, .Value = std::string{"A"}},
                      {.Code = 2, .Value = std::string{"B"}},
                      {.Code = 1, .Value = std::int64_t{1}},
                      {.Code = 2, .Value = std::string{"C"}},
                      {.Code = 1, .Value = std::string{"D"}},
                      {.Code = 3, .Value = std::string{"E"}},
                      {.Code = 1, .Value = std::string{"F"}},
                      {.Code = 2, .Value = std::int64_t{2}}};
    }
  }
  const std::optional<Drawing> unnamed = Pass(WidenNames, odd, Version::R2000);
  REQUIRE(unnamed);
  REQUIRE(View::Record<Tables::Layer>(*unnamed, "WALLS") != nullptr);
  CHECK(View::Record<Tables::Layer>(*unnamed, "WALLS")->Record.Name == "WALLS");
}

TEST_CASE("releases/names: an object without a handle or with a repeated handle or a shared record stays")
{
  // Upgrading: a layer of no handle, and one of its own sharing the record another layer already gave its names back.
  Drawing older = Load(ReadFixture("tables-r14.dwg")).value().Drawing;
  const Object walls = *Find(older, View::Record<Tables::Layer>(older, "WALLS")->Record.Base.Self);
  Object unhandled = walls;
  BaseOf(unhandled).Self = {};
  Object sharing = walls;
  BaseOf(sharing).Self = Edit::MakeHandle(older);
  older.Objects.push_back(unhandled);
  older.Objects.push_back(sharing);
  const std::optional<Drawing> widened = Pass(WidenNames, older, Version::R2000);
  REQUIRE(widened);
  CHECK(std::ranges::contains(widened->Objects, unhandled));
  CHECK(View::Record<Tables::Layer>(*widened, "Walls") != nullptr);
  // Saving as R14: a dictionary of no handle, and one whose handle another bears first.
  Drawing newer = Load(ReadFixture("tables-r2000.dwg")).value().Drawing;
  Object root = *Find(newer, newer.Header.DictionaryNamedObjects);
  Object again = root;
  BaseOf(root).Self = {};
  newer.Objects.push_back(root);
  newer.Objects.push_back(again);
  const std::optional<Drawing> narrowed = Pass(NarrowNames, newer, Version::R14);
  REQUIRE(narrowed);
  CHECK(std::ranges::contains(narrowed->Objects, root));
  CHECK(std::ranges::contains(narrowed->Objects, again));
}

TEST_CASE("releases/names: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
