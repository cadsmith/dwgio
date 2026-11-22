/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/history.cpp
 *  @brief     A solid's history kept in its record below R2007 and taken back on upgrading.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/history.hpp"

#include "releases/roundtrip.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"
#include "support/pass.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/save.hpp"
#include "dwgio/symbol.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  Each solid's history, by the solid's handle.
 */
auto Histories(const Drawing &drawing) -> std::map<std::uint64_t, Handle>
{
  std::map<std::uint64_t, Handle> histories;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *solid = GetIf<Entities::Solid3d>(&object))
    {
      histories[solid->Base.Self.Value] = solid->History;
    }
  }
  return histories;
}

/**
 *  Each solid's round-trip record, by the solid's handle; empty where it has none.
 */
auto Records(const Drawing &drawing) -> std::map<std::uint64_t, std::vector<Tag>>
{
  std::map<std::uint64_t, std::vector<Tag>> records;
  for (const Object &object : drawing.Objects)
  {
    const auto *solid = GetIf<Entities::Solid3d>(&object);
    const auto *dictionary =
        solid != nullptr ? GetIf<Objects::Dictionary>(Find(drawing, solid->Base.Dictionary)) : nullptr;
    if (solid == nullptr)
    {
      continue;
    }
    std::vector<Tag> &record = records[solid->Base.Self.Value];
    if (dictionary != nullptr)
    {
      const auto entry = std::ranges::find(dictionary->Entries, "ACAD_XREC_ROUNDTRIP", &Objects::Entry::Name);
      REQUIRE(entry != dictionary->Entries.end());
      record = Get<Objects::XRecord>(*Find(drawing, entry->Object)).Data;
    }
  }
  return records;
}
} // namespace

TEST_CASE("releases/history: a solid's history moves into its record below R2007")
{
  const std::map<std::uint64_t, std::vector<Tag>> theirs = Records(LoadFixture("histories-r2004.dwg").Drawing);
  for (const std::string_view name : {"histories-r2007.dwg", "histories-r2018.dwg"})
  {
    CAPTURE(name);
    const std::optional<Drawing> lowered = Pass(KeepRoundTrips, LoadFixture(name).Drawing, Version::R2004);
    REQUIRE(lowered);
    CHECK(Records(*lowered) == theirs);
  }
}

TEST_CASE("releases/history: a solid's history comes back from its record on upgrading to R2007")
{
  const std::optional<Drawing> upgraded =
      Pass(KeepRoundTrips, LoadFixture("histories-r2004.dwg").Drawing, Version::R2007);
  REQUIRE(upgraded);
  CHECK(Histories(*upgraded) == Histories(LoadFixture("histories-r2004-as-r2007.dwg").Drawing));
  for (const auto &[solid, record] : Records(*upgraded))
  {
    CAPTURE(solid);
    CHECK(record.empty());
  }
}

TEST_CASE("releases/history: solids saved below R2007 and back keep their histories")
{
  const Drawing newer = LoadFixture("histories-r2018.dwg").Drawing;
  for (const Version older : {Version::R2004, Version::R2000, Version::R14})
  {
    CAPTURE(older);
    const std::expected<Saved, Failure> down = Save(newer, SaveOptions{.Release = older});
    REQUIRE(down);
    const std::expected<Loaded, Failure> lowered = Load(down->Bytes);
    REQUIRE(lowered);
    CHECK(lowered->Issues.empty());
    QueueAudit(down->Bytes, std::format("dwgio-histories-r2018-as-{}.dwg", static_cast<int>(older)));
    const std::expected<Saved, Failure> up = Save(lowered->Drawing, SaveOptions{.Release = Version::R2018});
    REQUIRE(up);
    const std::expected<Loaded, Failure> back = Load(up->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(up->Issues.empty());
    CHECK(Histories(back->Drawing) == Histories(newer));
    QueueAudit(up->Bytes, std::format("dwgio-histories-r2018-as-{}-as-r2018.dwg", static_cast<int>(older)));
  }
}

TEST_CASE("releases/history: a record's history of the wrong kind or without its value is skipped")
{
  Entities::Solid3d solid;
  std::vector<Tag> record{Tag{.Code = 7, .Value = std::int64_t{1}},
                          Tag{.Code = 102, .Value = std::string{"ACAD_SOLID_HIST"}},
                          Tag{.Code = 360, .Value = std::int64_t{3}},
                          Tag{.Code = 102, .Value = std::string{"ACAD_SOLID_HIST1"}},
                          Tag{.Code = 90, .Value = Handle{0x40}},
                          Tag{.Code = 102, .Value = std::int64_t{2}},
                          Tag{.Code = 102, .Value = std::string{"ACAD_SOLID_HIST2"}}};
  RestoreHistory(solid, record);
  CHECK(record ==
        std::vector<Tag>{Tag{.Code = 7, .Value = std::int64_t{1}}, Tag{.Code = 102, .Value = std::int64_t{2}}});
  CHECK(solid == Entities::Solid3d{});
}
} // namespace DwgIO::Releases

namespace DwgIO::Releases
{
TEST_CASE("releases/history: a history the save drops is not kept and a record naming no history gives none")
{
  Drawing drawing = LoadFixture("histories-r2018.dwg").Drawing;
  for (Object &object : drawing.Objects)
  {
    // History nodes of a class not found in AutoCAD's files: each graph and history stays out of an older save.
    if (auto *unknown = GetIf<Unknown>(&object); unknown != nullptr && unknown->Class.View().starts_with("ACSH_") &&
                                                 unknown->Class.View() != "ACSH_HISTORY_CLASS")
    {
      unknown->Class = Symbol{"ACSH_FILLET_CLASS"};
    }
  }
  const std::optional<Drawing> lowered = Pass(KeepRoundTrips, drawing, Version::R2004);
  REQUIRE(lowered);
  for (const auto &[solid, record] : Records(*lowered))
  {
    CAPTURE(solid);
    CHECK(record.empty());
  }
  Drawing older = LoadFixture("histories-r2004.dwg").Drawing;
  for (Object &object : older.Objects)
  {
    if (auto *xrecord = GetIf<Objects::XRecord>(&object))
    {
      for (Tag &group : xrecord->Data)
      {
        group.Value = group.Code == 360 ? Content{Handle{0xFFFFF}} : group.Value;
      }
    }
  }
  const std::optional<Drawing> upgraded = Pass(KeepRoundTrips, older, Version::R2007);
  REQUIRE(upgraded);
  for (const auto &[solid, history] : Histories(*upgraded))
  {
    CAPTURE(solid);
    CHECK(history == Handle{});
  }
}

TEST_CASE("releases/history: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
