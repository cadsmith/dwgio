/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/summary.cpp
 *  @brief     The summary kept as the DWGPROPS record below R2004 and taken back on upgrading.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/summary.hpp"

#include "support/acad.hpp"
#include "support/fixtures.hpp"
#include "support/pass.hpp"
#include "support/summary.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/save.hpp"
#include "dwgio/summary.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  A summary of twelve properties, past the ten an R2000 record keeps by code, and text past the code page.
 */
auto FillSummary(const SummaryInfo &from) -> SummaryInfo
{
  SummaryInfo summary = from;
  summary.Title = "Caf\xc3\xa9 \xe3\x82\xbf title";
  summary.Author = "Auteur \xc3\xbc";
  summary.Comments = "Line one";
  summary.RevisionNumber = "Rev 7";
  summary.Properties.clear();
  for (int at = 1; at <= 12; ++at)
  {
    summary.Properties.push_back(
        Property{.Name = std::format("Key{}", at), .Value = std::format("Value \xc3\xa9{}", at)});
  }
  return summary;
}

} // namespace

TEST_CASE("releases/summary: an R2000 drawing's summary is its DWGPROPS record")
{
  for (const std::string_view fixture : {"empty-r14.dwg", "curves-r2000.dwg"})
  {
    CAPTURE(fixture);
    const Drawing drawing = Load(ReadFixture(fixture)).value().Drawing;
    const Objects::XRecord *record = FindProperties(drawing);
    REQUIRE(record != nullptr);
    REQUIRE(drawing.SummaryInfo);
    CHECK(SummaryOf(*record) == drawing.SummaryInfo);
    CHECK_FALSE(drawing.SummaryInfo->LastSavedBy.empty());
    CHECK(drawing.SummaryInfo->Properties.empty());
    CHECK(drawing.SummaryInfo->CreateDateTimeDays == drawing.Header.TdcreateDays);
    // AutoCAD's groups are those a summary makes.
    CHECK(SummaryOf(Objects::XRecord{
              .Base = {}, .Cloning = 1, .Data = MakePropertyGroups(*drawing.SummaryInfo), .Objects = {}, .Rest = {}}) ==
          drawing.SummaryInfo);
    REQUIRE(MakePropertyGroups(*drawing.SummaryInfo).size() == record->Data.size());
    for (std::size_t at = 0; at < record->Data.size(); ++at)
    {
      CAPTURE(at);
      CHECK(MakePropertyGroups(*drawing.SummaryInfo)[at].Code == record->Data[at].Code);
    }
  }
}

TEST_CASE("releases/summary: a nameless property is dropped and a record without its cookie gives none")
{
  const Objects::XRecord record{.Base = {},
                                .Cloning = 1,
                                .Data = {{.Code = 1, .Value = std::string{"DWGPROPS COOKIE"}},
                                         {.Code = 2, .Value = std::string{"Title"}},
                                         {.Code = 5, .Value = std::string{"Other"}},
                                         {.Code = 300, .Value = std::string{"=Nameless"}},
                                         {.Code = 301, .Value = std::string{"Empty="}},
                                         {.Code = 302, .Value = std::string{"no sign"}},
                                         {.Code = 303, .Value = std::int64_t{4}},
                                         {.Code = 40, .Value = 1.5},
                                         {.Code = 41, .Value = 2461353.0 - 1e-9},
                                         {.Code = 43, .Value = 1.0},
                                         {.Code = 1, .Value = std::string{"http://example.com/"}},
                                         {.Code = 90, .Value = std::int64_t{2}},
                                         {.Code = 300, .Value = std::string{"Late=one"}},
                                         {.Code = 310, .Value = std::string{"Not=one"}}},
                                .Objects = {},
                                .Rest = {}};
  const std::optional<SummaryInfo> summary = SummaryOf(record);
  REQUIRE(summary);
  CHECK(summary->Title == "Title");
  CHECK(summary->HyperlinkBase == "http://example.com/");
  CHECK(summary->Properties == std::vector<Property>{{.Name = "Empty", .Value = ""}, {.Name = "Late", .Value = "one"}});
  CHECK(summary->TotalEditingTimeDays == 1);
  CHECK(summary->TotalEditingTimeMilliseconds == 43200000);
  // A time of the next day's first millisecond rounded is that day.
  CHECK(summary->CreateDateTimeDays == 2461353);
  CHECK(summary->CreateDateTimeMilliseconds == 0);
  // Written, the dates to the second as AutoCAD writes them, the editing time to the millisecond.
  SummaryInfo exact = *summary;
  exact.CreateDateTimeMilliseconds = 1999;
  exact.TotalEditingTimeMilliseconds = 1999;
  const std::optional<SummaryInfo> written = SummaryOf(
      Objects::XRecord{.Base = {}, .Cloning = 1, .Data = MakePropertyGroups(exact), .Objects = {}, .Rest = {}});
  REQUIRE(written);
  CHECK(written->CreateDateTimeMilliseconds == 1000);
  CHECK(written->TotalEditingTimeMilliseconds == 1999);
  CHECK_FALSE(SummaryOf(Objects::XRecord{}));
  Objects::XRecord other = record;
  other.Data.front().Value = std::string{"OTHER"};
  CHECK_FALSE(SummaryOf(other));
}

TEST_CASE("releases/summary: a date past what a long counts or without a number is clamped")
{
  const std::optional<SummaryInfo> summary = SummaryOf(Objects::XRecord{
      .Base = {},
      .Cloning = 1,
      .Data = {Tag{.Code = 1, .Value = std::string{"DWGPROPS COOKIE"}}, Tag{.Code = 40, .Value = -1e300},
               Tag{.Code = 41, .Value = 1e300}, Tag{.Code = 42, .Value = std::numeric_limits<double>::quiet_NaN()}},
      .Objects = {},
      .Rest = {}});
  REQUIRE(summary);
  CHECK(summary->TotalEditingTimeDays == std::numeric_limits<std::int32_t>::min());
  CHECK(summary->TotalEditingTimeMilliseconds == 0);
  CHECK(summary->CreateDateTimeDays == std::numeric_limits<std::int32_t>::max() - 1);
  CHECK(summary->ModifiedDateTimeDays == 0);
  CHECK(summary->ModifiedDateTimeMilliseconds == 0);
}

TEST_CASE("releases/summary: below R2004 the record is made or filled and upgrading removes it")
{
  // Saved below R2004 from R2018, the record is made under the named objects dictionary, sorted by lower case.
  const Drawing newer = Load(ReadFixture("empty-r2018.dwg")).value().Drawing;
  REQUIRE(newer.SummaryInfo);
  REQUIRE(FindProperties(newer) == nullptr);
  const std::optional<Drawing> made = Pass(MoveSummary, newer, Version::R2000);
  REQUIRE(made);
  const Objects::XRecord *record = FindProperties(*made);
  REQUIRE(record != nullptr);
  CHECK(SummaryOf(*record) == newer.SummaryInfo);
  CHECK(record->Base.Owner == newer.Header.DictionaryNamedObjects);
  CHECK(record->Base.Self.Value >= newer.Header.Handseed.Value);
  CHECK(View::Root(*made)->Entries.back().Name == "DWGPROPS");
  // A record already holding the summary is left as it is; one holding another is filled.
  CHECK_FALSE(Pass(MoveSummary, *made, Version::R2000));
  Drawing changed = *made;
  changed.SummaryInfo->Title = "Changed";
  const std::optional<Drawing> filled = Pass(MoveSummary, changed, Version::R2000);
  REQUIRE(filled);
  CHECK(FindProperties(*filled)->Base.Self == record->Base.Self);
  CHECK(SummaryOf(*FindProperties(*filled))->Title == "Changed");
  // No summary, no record made; nor under a name another kind holds, nor without a named objects dictionary.
  Drawing bare = newer;
  bare.SummaryInfo.reset();
  CHECK_FALSE(Pass(MoveSummary, bare, Version::R14));
  Drawing taken = newer;
  const Handle groups = View::Dictionary(taken, "ACAD_GROUP")->Base.Self;
  Get<Objects::Dictionary>(*Find(taken, taken.Header.DictionaryNamedObjects))
      .Entries.push_back(Objects::Entry{.Name = "DWGPROPS", .Object = groups});
  CHECK_FALSE(Pass(MoveSummary, taken, Version::R2000));
  Drawing rootless = newer;
  rootless.Header.DictionaryNamedObjects = {};
  CHECK_FALSE(Pass(MoveSummary, rootless, Version::R2000));
  // Upgraded from R2000, the record and its entry are taken away; from R2004 a record of that name stays.
  const Drawing older = Load(ReadFixture("empty-r2000.dwg")).value().Drawing;
  const std::optional<Drawing> upgraded = Pass(MoveSummary, older, Version::R2018);
  REQUIRE(upgraded);
  CHECK(FindProperties(*upgraded) == nullptr);
  CHECK(upgraded->Objects.size() == older.Objects.size() - 1);
  CHECK(upgraded->SummaryInfo == older.SummaryInfo);
  CHECK_FALSE(Pass(MoveSummary, *made, Version::R2018));
  CHECK_FALSE(Pass(MoveSummary, newer, Version::R2018));
  // A record of that name holding no summary stays.
  Drawing strange = older;
  Get<Objects::XRecord>(*Find(strange, FindProperties(older)->Base.Self)).Data.clear();
  CHECK_FALSE(Pass(MoveSummary, strange, Version::R2018));
}

TEST_CASE("releases/summary: the record sorts names by ASCII lower case whatever char's sign")
{
  Drawing newer = Load(ReadFixture("empty-r2018.dwg")).value().Drawing;
  const Handle groups = View::Dictionary(newer, "ACAD_GROUP")->Base.Self;
  std::vector<Objects::Entry> &held =
      Get<Objects::Dictionary>(*Find(newer, newer.Header.DictionaryNamedObjects)).Entries;
  for (const char *name : {"DWG1", "DWG_X", "\xc3\xa9t\xc3\xa9"})
  {
    held.push_back(Objects::Entry{.Name = name, .Object = groups});
  }
  const std::optional<Drawing> made = Pass(MoveSummary, newer, Version::R2000);
  REQUIRE(made);
  const std::vector<Objects::Entry> &entries = View::Root(*made)->Entries;
  REQUIRE(entries.size() >= 4);
  CHECK(entries[entries.size() - 3].Name == "DWG_X");
  CHECK(entries[entries.size() - 2].Name == "DWGPROPS");
  CHECK(entries.back().Name == "\xc3\xa9t\xc3\xa9");
}

TEST_CASE("releases/summary: the summary saved to every release reads back")
{
  Drawing drawing = Load(ReadFixture("empty-r2018.dwg")).value().Drawing;
  drawing.SummaryInfo = FillSummary(*drawing.SummaryInfo);
  for (const Version release : FixtureReleases)
  {
    CAPTURE(NameOf(release));
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = release});
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Drawing.SummaryInfo == drawing.SummaryInfo);
    if (IsVerifying())
    {
      // AutoCAD's own save of the file as R2000 holds what it read, but for what it sets at a save.
      const std::vector<std::byte> theirs = Resave(saved->Bytes, std::format("dwgio-summary-as-{}", NameOf(release)));
      const std::expected<Loaded, Failure> read = Load(theirs);
      REQUIRE(read);
      REQUIRE(read->Drawing.SummaryInfo);
      CHECK(ClearSaveStamps(*read->Drawing.SummaryInfo) == ClearSaveStamps(*drawing.SummaryInfo));
      QueueAudit(saved->Bytes, std::format("dwgio-summary-as-{}.dwg", NameOf(release)));
    }
  }
}

TEST_CASE("releases/summary: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
