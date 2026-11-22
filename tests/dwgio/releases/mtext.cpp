/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/mtext.cpp
 *  @brief     An MTEXT's later fields kept in extended data below R2018 and taken back on upgrading.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/mtext.hpp"

#include "releases/object.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"
#include "support/pass.hpp"

#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
auto Texts(const Drawing &drawing) -> std::vector<const Entities::MText *>
{
  std::vector<const Entities::MText *> texts;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *text = GetIf<Entities::MText>(&object))
    {
      texts.push_back(text);
    }
  }
  return texts;
}

auto Groups(const Base &base) -> std::vector<std::vector<Tag>>
{
  std::vector<std::vector<Tag>> groups;
  for (const Xdata &xdata : base.Extended)
  {
    groups.push_back(xdata.Tags);
  }
  return groups;
}

auto Marker(std::string_view text) -> Tag
{
  return Tag{.Code = 1000, .Value = std::string{text}};
}

/**
 *  The groups without the list of the MTEXTs AutoCAD split the text into, which dwgio, keeping the text whole, has not;
 *  empty when they had no list.
 */
auto DropColumnList(std::vector<Tag> groups) -> std::optional<std::vector<Tag>>
{
  const auto begin = std::ranges::find(groups, Marker("ACAD_MTEXT_COLUMNS_BEGIN"));
  const auto end = std::ranges::find(groups, Marker("ACAD_MTEXT_COLUMNS_END"));
  if (begin == groups.end())
  {
    return std::nullopt;
  }
  groups.erase(begin, end + 1);
  return groups;
}

/**
 *  The fields an older release keeps in extended data.
 */
void CheckSame(const Entities::MText &ours, const Entities::MText &theirs)
{
  CHECK(ours.Columns == theirs.Columns);
  CHECK(ours.Width == theirs.Width);
  CHECK(ours.LineSpacingStyle == theirs.LineSpacingStyle);
  CHECK(ours.LineSpacingFactor == theirs.LineSpacingFactor);
  CHECK(ours.Background == theirs.Background);
  CHECK(ours.Defined == theirs.Defined);
}

auto MakeMText(Entities::Columns columns) -> Entities::MText
{
  Entities::MText text;
  text.Columns = std::move(columns);
  return text;
}
} // namespace

TEST_CASE("releases/mtext: an MTEXT saved below R2018 moves what each release keeps into xdata")
{
  const Drawing newer = LoadDrawing("columns-r2018.dwg");
  for (const std::string_view below : {"r2013", "r2010", "r2007", "r2004", "r2000", "r14"})
  {
    CAPTURE(below);
    const Drawing older = LoadDrawing(std::string{"columns-"} + std::string{below} + ".dwg");
    std::size_t split = 0;
    for (const Entities::MText *text : Texts(newer))
    {
      CAPTURE(text->Base.Self.Value);
      const auto *twin = GetIf<Entities::MText>(Find(older, text->Base.Self));
      REQUIRE(twin != nullptr);
      const Translation translated = ConvertObject(Object{*text}, newer, older.Release);
      const auto &lower = Get<Entities::MText>(translated.Object);
      CheckSame(lower, *twin);
      std::vector<std::vector<Tag>> theirs = Groups(twin->Base);
      // AutoCAD splits a text that fills more than one column; dwgio keeps it whole, and says so.
      const std::optional<std::vector<Tag>> unlisted = theirs.empty() ? std::nullopt : DropColumnList(theirs.front());
      split += unlisted ? 1U : 0U;
      if (unlisted)
      {
        theirs.front() = *unlisted;
      }
      CHECK(Groups(lower.Base) == theirs);
      CHECK(translated.Issues.size() == (text->Columns.Count > 1 ? 1 : 0));
    }
    CHECK(split == 2);
  }
  const Drawing entities = LoadDrawing("entities-r2018.dwg");
  const Object &text = *Find(entities, Handle{0x2E4});
  for (const std::string_view below : {"r2013", "r2007", "r2000", "r14"})
  {
    CAPTURE(below);
    const Drawing older = LoadDrawing(std::string{"entities-"} + std::string{below} + ".dwg");
    const Translation translated = ConvertObject(text, entities, older.Release);
    const auto &lower = Get<Entities::MText>(translated.Object);
    const auto &twin = Get<Entities::MText>(*Find(older, Handle{0x2E4}));
    CHECK(Groups(lower.Base) == Groups(twin.Base));
    CheckSame(lower, twin);
  }
}

TEST_CASE("releases/mtext: an MTEXT upgraded to R2018 takes back what its xdata keeps")
{
  const Drawing newer = LoadDrawing("columns-r2018.dwg");
  const Drawing fromFourteen = LoadDrawing("columns-r14-as-r2018.dwg");
  for (const std::string_view below : {"r2013", "r2010", "r2007", "r2004", "r2000", "r14"})
  {
    CAPTURE(below);
    const Drawing older = LoadDrawing(std::string{"columns-"} + std::string{below} + ".dwg");
    for (const Entities::MText *text : Texts(newer))
    {
      CAPTURE(text->Base.Self.Value);
      const Translation translated = ConvertObject(*Find(older, text->Base.Self), older, Version::R2018);
      CHECK(translated.Issues.empty());
      const auto &upper = Get<Entities::MText>(translated.Object);
      CheckSame(upper, *text);
      // The defined height AutoCAD reads and leaves, as its save of the R14 file shows.
      const auto &theirs =
          Get<Entities::MText>(*Find(older.Release < Version::R2007 ? fromFourteen : newer, text->Base.Self));
      CHECK(Groups(upper.Base) == Groups(theirs.Base));
    }
  }
}

TEST_CASE("releases/mtext: the MTEXTs AutoCAD split columns into join again on upgrading")
{
  const Drawing newer = LoadDrawing("columns-r2018.dwg");
  for (const std::string_view below : {"r2013", "r2000", "r14"})
  {
    CAPTURE(below);
    const Drawing older = LoadDrawing(std::string{"columns-"} + std::string{below} + ".dwg");
    const std::expected<Saved, Failure> saved = Save(older, SaveOptions{.Release = Version::R2018});
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    REQUIRE(Texts(back->Drawing).size() == Texts(newer).size());
    for (const Entities::MText *text : Texts(newer))
    {
      CAPTURE(text->Base.Self.Value);
      const auto *mine = GetIf<Entities::MText>(Find(back->Drawing, text->Base.Self));
      REQUIRE(mine != nullptr);
      CHECK(mine->Value == text->Value);
      CheckSame(*mine, *text);
    }
    QueueAudit(saved->Bytes, std::string{"dwgio-columns-"} + std::string{below} + "-as-r2018.dwg");
  }
  CHECK(!Pass(JoinColumns, newer, Version::R2018));
  CHECK(!Pass(JoinColumns, LoadDrawing("columns-r2013.dwg"), Version::R2013));
  CHECK(!Pass(JoinColumns, LoadDrawing("entities-r2013.dwg"), Version::R2018));
}

TEST_CASE("releases/mtext: MTEXTs saved below R2018 and back keep what they hold")
{
  const Drawing newer = LoadDrawing("columns-r2018.dwg");
  for (const Version below : {Version::R2013, Version::R2004, Version::R2000, Version::R14})
  {
    CAPTURE(NameOf(below));
    const std::expected<Saved, Failure> down = Save(newer, SaveOptions{.Release = below});
    REQUIRE(down);
    const std::expected<Loaded, Failure> older = Load(down->Bytes);
    REQUIRE(older);
    CHECK(older->Issues.empty());
    QueueAudit(down->Bytes, std::string{"dwgio-columns-r2018-as-"} + std::string{NameOf(below)} + ".dwg");
    const std::expected<Saved, Failure> up = Save(older->Drawing, SaveOptions{.Release = Version::R2018});
    REQUIRE(up);
    const Drawing back = Load(up->Bytes).value().Drawing;
    for (const Entities::MText *text : Texts(newer))
    {
      CAPTURE(text->Base.Self.Value);
      const auto &mine = Get<Entities::MText>(*Find(back, text->Base.Self));
      CheckSame(mine, *text);
      CHECK(mine.Value == text->Value);
    }
  }
}

TEST_CASE("releases/mtext: an annotative text keeps no columns and making sections needs ACAD")
{
  Entities::MText annotative = MakeMText(Entities::Columns{.Kind = 1, .Count = 2});
  annotative.Annotative = true;
  CHECK(ConvertMText(annotative, TextContext{.From = Version::R2018, .To = Version::R2013, .Acad = {}}));
  CHECK(annotative.Base.Extended.empty());
  CHECK(annotative.Columns == Entities::Columns{});
  Entities::MText columned = MakeMText(Entities::Columns{.Kind = 1, .Count = 2});
  CHECK(!ConvertMText(columned, TextContext{.From = Version::R2018, .To = Version::R2013, .Acad = {}}));
  CHECK(columned.Columns.Count == 2);
  Drawing drawing = Create(Version::R2018);
  std::erase_if(drawing.Objects,
                [](const Object &object) -> bool
                {
                  const auto *appid = GetIf<Tables::AppId>(&object);
                  return appid != nullptr && appid->Record.Name == "ACAD";
                });
  const Translation translated = ConvertObject(Object{columned}, drawing, Version::R2013);
  REQUIRE(translated.Issues.size() == 1);
  CHECK(translated.Issues.front().What.contains("application ACAD missing for MTEXT"));
}

TEST_CASE("releases/mtext: a background with its own color names and book converts across R2004")
{
  Entities::MText text;
  text.Background =
      Entities::Background{.Fill = 1,
                           .Margin = 2,
                           .Color = Color{.Index = 0,
                                          .Rgb = static_cast<std::int32_t>(0xC2102030U),
                                          .Names = ColorNames{.Name = std::string{"Sky"}, .Book = std::string{"Mine"}}},
                           .Transparency = 5};
  Entities::MText lower = text;
  REQUIRE(ConvertMText(lower, TextContext{.From = Version::R2004, .To = Version::R2000, .Acad = Handle{0x12}}));
  CHECK(lower.Background == Entities::Background{});
  Entities::MText upper = lower;
  REQUIRE(ConvertMText(upper, TextContext{.From = Version::R2000, .To = Version::R2004, .Acad = Handle{0x12}}));
  CHECK(upper.Background == text.Background);
  CHECK(upper.Base.Extended.empty());
  // A book without a name.
  text.Background.Color.Names = ColorNames{.Name = std::nullopt, .Book = std::string{"Mine"}};
  lower = text;
  REQUIRE(ConvertMText(lower, TextContext{.From = Version::R2004, .To = Version::R2000, .Acad = Handle{0x12}}));
  upper = lower;
  REQUIRE(ConvertMText(upper, TextContext{.From = Version::R2000, .To = Version::R2004, .Acad = Handle{0x12}}));
  CHECK(upper.Background == text.Background);
}

TEST_CASE("releases/mtext: reversed columns convert across R2018 both ways")
{
  Entities::MText text =
      MakeMText(Entities::Columns{.Kind = 1, .Count = 2, .Breadth = 3, .Gutter = 1, .Reversed = true});
  text.Width = 7;
  Entities::MText lower = text;
  REQUIRE(ConvertMText(lower, TextContext{.From = Version::R2018, .To = Version::R2013, .Acad = Handle{0x12}}));
  CHECK(lower.Width == 3);
  Entities::MText upper = lower;
  REQUIRE(ConvertMText(upper, TextContext{.From = Version::R2013, .To = Version::R2018, .Acad = Handle{0x12}}));
  CHECK(upper.Columns == text.Columns);
  CHECK(upper.Width == 7);
}

TEST_CASE("releases/mtext: an MTEXT moved between two older releases keeps its sections")
{
  for (const auto &[from, to] : {std::pair{"r2000", "r14"}, std::pair{"r2004", "r2000"}, std::pair{"r14", "r2004"}})
  {
    CAPTURE(from);
    CAPTURE(to);
    const Drawing older = LoadDrawing(std::string{"columns-"} + to + ".dwg");
    const Drawing source = LoadDrawing(std::string{"columns-"} + from + ".dwg");
    std::size_t compared = 0;
    for (const Entities::MText *text : Texts(source))
    {
      CAPTURE(text->Base.Self.Value);
      const auto *twin = GetIf<Entities::MText>(Find(older, text->Base.Self));
      if (twin == nullptr)
      {
        continue; // A further column, which each save remakes.
      }
      const Translation translated = ConvertObject(Object{*text}, source, older.Release);
      CHECK(translated.Issues.empty());
      const auto &mine = Get<Entities::MText>(translated.Object);
      CheckSame(mine, *twin);
      std::vector<std::vector<Tag>> ours = Groups(mine.Base);
      std::vector<std::vector<Tag>> theirs = Groups(twin->Base);
      // The handles of the further columns differ between AutoCAD's saves.
      for (auto *groups : {&ours, &theirs})
      {
        for (std::vector<Tag> &record : *groups)
        {
          std::erase_if(record, [](const Tag &group) -> bool { return group.Code == 1005; });
        }
      }
      CHECK(ours == theirs);
      ++compared;
    }
    CHECK(compared == 8);
  }
}

TEST_CASE("releases/mtext: xdata is read however it is cut and what else it holds stays")
{
  using Entities::Columns;
  Entities::MText text;
  text.ExtentHeight = 4;
  text.Base.Extended = {
      Xdata{.Application = {}, .AppId = Handle{0x12}, .Tags = {Marker("other")}},
      Xdata{.Application = {},
            .AppId = Handle{0x12},
            .Tags = {Marker("ACAD_MTEXT_COLUMN_INFO_BEGIN"), Tag{.Code = 1070, .Value = std::int64_t{75}},
                     Tag{.Code = 1070, .Value = std::int64_t{2}}, Tag{.Code = 1070, .Value = std::int64_t{99}},
                     Tag{.Code = 1070, .Value = std::int64_t{1}}, Tag{.Code = 1070, .Value = std::int64_t{48}},
                     Tag{.Code = 1070, .Value = std::int64_t{3}}, Tag{.Code = 1070, .Value = std::int64_t{50}},
                     Tag{.Code = 1070, .Value = std::int64_t{3}}, Tag{.Code = 1040, .Value = 2.0},
                     Tag{.Code = 1040, .Value = 0.0}, Marker("ACAD_MTEXT_COLUMN_INFO_END"), Marker("ACAD_MTEXT_BBRT"),
                     Tag{.Code = 1070, .Value = std::int64_t{65}}, Tag{.Code = 1070, .Value = std::int64_t{3}},
                     Tag{.Code = 1070, .Value = std::int64_t{1}}, Tag{.Code = 1070, .Value = std::int64_t{9}},
                     Marker("ACAD_MTEXT_BERT"), Marker("kept")}},
      Xdata{.Application = {},
            .AppId = Handle{0x12},
            .Tags = {Marker("ACAD_MTEXT_DEFINED_HEIGHT_BEGIN"), Tag{.Code = 1070, .Value = std::int64_t{99}},
                     Tag{.Code = 1040, .Value = 1.0}, Tag{.Code = 1070, .Value = std::int64_t{46}},
                     Tag{.Code = 1040, .Value = 6.0}, Marker("ACAD_MTEXT_DEFINED_HEIGHT_END")}}};
  REQUIRE(ConvertMText(text, TextContext{.From = Version::R2000, .To = Version::R2018, .Acad = Handle{0x12}}));
  CHECK(text.Columns == Columns{.Kind = 2, .Breadth = 0, .Heights = {2.0, -4.0}});
  CHECK(text.Width == 0); // No count, the width as it was.
  CHECK(text.Background == Entities::Background{});
  CHECK(text.Defined == 6);
  REQUIRE(text.Base.Extended.size() == 3); // Upgrading leaves the defined height, as AutoCAD does.
  CHECK(text.Base.Extended[1].Tags == std::vector<Tag>{Marker("kept")});
  // Saved below R2018, a stale defined height is made again after what else the record holds.
  Entities::MText stale;
  stale.Defined = 5;
  stale.Base.Extended = {Xdata{.Application = {},
                               .AppId = Handle{0x12},
                               .Tags = {Marker("kept"), Marker("ACAD_MTEXT_DEFINED_HEIGHT_BEGIN"),
                                        Tag{.Code = 1070, .Value = std::int64_t{46}}, Tag{.Code = 1040, .Value = 3.0},
                                        Marker("ACAD_MTEXT_DEFINED_HEIGHT_END")}}};
  REQUIRE(ConvertMText(stale, TextContext{.From = Version::R2018, .To = Version::R2004, .Acad = Handle{0x12}}));
  CHECK(stale.Defined == 0);
  CHECK(Groups(stale.Base) ==
        std::vector<std::vector<Tag>>{{Marker("kept"), Marker("ACAD_MTEXT_DEFINED_HEIGHT_BEGIN"),
                                       Tag{.Code = 1070, .Value = std::int64_t{46}}, Tag{.Code = 1040, .Value = 5.0},
                                       Marker("ACAD_MTEXT_DEFINED_HEIGHT_END")}});
  // A negative count of heights reads none; an unclosed section is no section.
  Entities::MText odd;
  odd.Base.Extended = {
      Xdata{.Application = {},
            .AppId = Handle{0x12},
            .Tags = {Marker("ACAD_MTEXT_COLUMN_INFO_BEGIN"), Tag{.Code = 1070, .Value = std::int64_t{50}},
                     Tag{.Code = 1070, .Value = std::int64_t{-1}}, Tag{.Code = 1070, .Value = std::int64_t{75}},
                     Tag{.Code = 1040, .Value = 2.5}, Marker("ACAD_MTEXT_COLUMN_INFO_END")}},
      Xdata{.Application = {}, .AppId = Handle{0x12}, .Tags = {Marker("ACAD_MTEXT_COLUMN_INFO_BEGIN")}}};
  REQUIRE(ConvertMText(odd, TextContext{.From = Version::R2013, .To = Version::R2018, .Acad = Handle{0x12}}));
  CHECK(odd.Columns == Columns{});
  CHECK(odd.Base.Extended.size() == 1);
}

TEST_CASE("releases/mtext: a list naming non-MTEXT objects joins only the texts it names")
{
  Drawing drawing = LoadDrawing("columns-r2013.dwg");
  const std::expected<Handle, Failure> line = Edit::Add(drawing, Object{Entities::Line{}});
  REQUIRE(line);
  auto &first = Get<Entities::MText>(*Find(drawing, Handle{0x2CE}));
  Handle replaced;
  for (Tag &group : first.Base.Extended.front().Tags)
  {
    if (std::holds_alternative<Handle>(group.Value) && !replaced)
    {
      replaced = std::get<Handle>(group.Value);
      group.Value = *line;
    }
  }
  REQUIRE(replaced);
  first.Base.Extended.push_back(Xdata{.Application = {}, .AppId = Handle{0x12}, .Tags = {Marker("other")}});
  const std::optional<Drawing> joined = Pass(JoinColumns, drawing, Version::R2018);
  REQUIRE(joined);
  CHECK(Find(*joined, *line) != nullptr);
  CHECK(Find(*joined, replaced) != nullptr);
  const std::string value = Get<Entities::MText>(*Find(*joined, Handle{0x2CE})).Value;
  CHECK(value.starts_with("alpha beta gamma "));
  CHECK(!value.contains(Get<Entities::MText>(*Find(drawing, replaced)).Value));
}

TEST_CASE("releases/mtext: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
