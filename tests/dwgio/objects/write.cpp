/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/write.cpp
 *  @brief     Writing every object of the fixtures back to the bytes the file holds.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "objects/write.hpp"

#include "classes/catalog.hpp"
#include "classes/layout.hpp"
#include "encoding/encoding.hpp"
#include "file/locator/section.hpp"
#include "file/locator/sentinels.hpp"
#include "file/paged/layout.hpp"
#include "objects/map.hpp"
#include "objects/read.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/class.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/hatch.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/lwpolyline.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/scale.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/write: every object of the tables fixture writes back the file's bytes")
{
  Fixture fixture = Open("tables-r2000.dwg");
  std::size_t typed = 0;
  for (const Mapped &entry : fixture.Entries)
  {
    const std::optional<Object> object = ReadObject(fixture.File, entry.Offset, fixture.Reading());
    REQUIRE_MESSAGE(object, HexOf(entry.Self));
    const std::vector<std::byte> written = WriteObject(*object, fixture.Context());
    REQUIRE(written.size() <= fixture.File.size() - entry.Offset);
    const std::span<const std::byte> kept = std::span{fixture.File}.subspan(entry.Offset, written.size());
    CHECK_MESSAGE(std::ranges::equal(written, kept), HexOf(entry.Self), " ", TypeOf(*object));
    typed += Holds<Unknown>(*object) ? 0U : 1U;
  }
  CHECK(typed >= 9);
  CHECK(fixture.Issues.empty());
}

TEST_CASE("objects/write: every object of the R14 fixtures writes back the file's bytes")
{
  for (const std::string_view name :
       {"tables-r14.dwg", "entities-r14.dwg", "curves-r14.dwg", "objects-r14.dwg", "empty-r14.dwg"})
  {
    CAPTURE(name);
    Fixture fixture = Open(name);
    std::size_t typed = 0;
    for (const Mapped &entry : fixture.Entries)
    {
      const std::optional<Object> object = ReadObject(fixture.File, entry.Offset, fixture.Reading());
      REQUIRE_MESSAGE(object, HexOf(entry.Self));
      const std::vector<std::byte> written = WriteObject(*object, fixture.Context());
      REQUIRE(written.size() <= fixture.File.size() - entry.Offset);
      const std::span<const std::byte> kept = std::span{fixture.File}.subspan(entry.Offset, written.size());
      CHECK_MESSAGE(std::ranges::equal(written, kept), HexOf(entry.Self), " ", TypeOf(*object));
      typed += Holds<Unknown>(*object) ? 0U : 1U;
    }
    CHECK(typed >= 9);
    CHECK(fixture.Issues.empty());
  }
}

TEST_CASE("objects/write: every untyped object rebuilt from its header and own bits gives its bytes")
{
  std::size_t untyped = 0;
  for (const std::string_view name : {"kinds", "dimensions", "mleaders", "objects", "table", "foundation"})
  {
    for (const std::string_view release : FixtureSuffixes)
    {
      CAPTURE(std::format("{}-{}", name, release));
      const Drawing drawing = Load(ReadFixture(std::format("{}-{}.dwg", name, release))).value().Drawing;
      const Encoding::Scheme scheme = Encoding::FindScheme(drawing.Codepage).value_or(Latin);
      std::vector<Issue> issues;
      const Fields::Context with{
          .Release = drawing.Release, .Text = &scheme, .Issues = &issues, .Classes = &drawing.Classes};
      for (const Object &object : drawing.Objects)
      {
        if (const auto *unknown = GetIf<Unknown>(&object))
        {
          REQUIRE(unknown->Body);
          Unknown bare = *unknown;
          bare.Bytes.clear();
          CHECK_MESSAGE(WriteObject(bare, with) == unknown->Bytes, HexOf(unknown->Base.Self), " ", TypeOf(object));
          ++untyped;
        }
      }
      CHECK(issues.empty());
    }
  }
  CHECK(untyped > 200);
}

TEST_CASE("objects/write: an untyped object with nothing to write for the release is noted")
{
  std::vector<Issue> issues;
  Unknown bare{.Type = 500, .Class = {}, .Entity = false, .Base = {}, .Common = {}, .Bytes = {}, .Body = {}};
  bare.Base.Self = Handle{0x42};
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Classes = nullptr};
  CHECK(WriteObject(bare, with).empty());
  bare.Body = Opaque{.Release = Version::R2004, .Data = {}, .Handles = {}, .Strings = {}};
  CHECK(WriteObject(bare, with).empty());
  REQUIRE(issues.size() == 2);
  CHECK(issues.front().What == "untyped object 42 of another release not written");
  CHECK(issues.front().Subject == Handle{0x42});
  CHECK(WriteObject(bare, Fields::Context{.Release = Version::R2000}).empty()); // Nowhere to note it.
  // Bytes as read, of a release whose own bits did not cross: not written as another's.
  bare.Bytes = {std::byte{1}, std::byte{2}};
  CHECK(WriteObject(bare, with).empty());
  CHECK(issues.size() == 3);
  bare.Body->Release = Version::R2000;
  CHECK(WriteObject(bare, with) == bare.Bytes);
  bare.Body.reset();
  CHECK(WriteObject(bare, with) == bare.Bytes); // Own bits that could not be read: the bytes as they were.
}

TEST_CASE("objects/write: an entity's proxy graphics go before its data in either release")
{
  // An LWPOLYLINE: a type in R2000, a class in R14, so one kind meets both framings and the class a release lacks.
  const std::vector<Class> classes{*Classes::FindDeclared("LWPOLYLINE")};
  for (const Version release : {Version::R2000, Version::R14})
  {
    CAPTURE(static_cast<int>(release));
    Entities::LwPolyline polyline;
    polyline.Base.Self = Handle{0x50};
    polyline.Base.Owner = Handle{0x1F};
    polyline.Common.Layer = Handle{0x10};
    polyline.Common.Graphics = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}};
    polyline.Vertices = {Entities::LwPolyline::Vertex{.Position = {.X = 1, .Y = 2}},
                         Entities::LwPolyline::Vertex{.Position = {.X = 3, .Y = 4}}};
    std::vector<Issue> issues;
    const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = classes, .Issues = &issues};
    const std::vector<std::byte> written = WriteObject(Object{polyline}, with);
    const std::optional<Object> back = ReadObject(written, 0, reading);
    REQUIRE(back);
    const Common *common = CommonOf(*back);
    REQUIRE(common);
    CHECK(common->Graphics == polyline.Common.Graphics);
    CHECK(Get<Entities::LwPolyline>(*back).Vertices == polyline.Vertices);
    CHECK(issues.empty());
  }
}

TEST_CASE("objects/write: an entity of a class the release lacks is not written")
{
  Entities::LwPolyline polyline;
  polyline.Base.Self = Handle{0x50};
  polyline.Base.Owner = Handle{0x1F};
  polyline.Common.Layer = Handle{0x10};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R14, .Text = &Latin, .Issues = &issues};
  CHECK(WriteObject(Object{polyline}, with).empty());
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "class LWPOLYLINE not declared");
}

TEST_CASE("objects/write: an unknown object writes its kept bytes")
{
  const Unknown unknown{.Bytes = {std::byte{1}, std::byte{2}, std::byte{3}}};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  CHECK(WriteObject(Object{unknown}, with) == unknown.Bytes);
}

TEST_CASE("objects/write: an R2004 object flags a missing extension dictionary and names a present one")
{
  for (const Handle dictionary : {Handle{}, Handle{0x77}})
  {
    CAPTURE(dictionary.Value);
    Scale scale;
    scale.Base.Self = Handle{0x100};
    scale.Base.Owner = Handle{0x10C};
    scale.Base.Dictionary = dictionary;
    scale.Name = "1:7";
    const std::vector<Class> classes{*Classes::FindDeclared("SCALE")};
    std::vector<Issue> issues;
    const Fields::Context with{.Release = Version::R2004, .Text = &Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{
        .Release = Version::R2004, .Text = &Latin, .Classes = classes, .Issues = &issues};
    const std::vector<std::byte> written = WriteObject(Object{scale}, with);
    const std::optional<Object> back = ReadObject(written, 0, reading);
    REQUIRE(back);
    CHECK(BaseOf(*back).Dictionary == dictionary);
    CHECK(*back == Object{scale});
    CHECK(issues.empty());
    // The same object in R2000 is one byte of handle longer when the dictionary is null: the handle is written.
    const Fields::Context older{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Classes = &classes};
    CHECK(WriteObject(Object{scale}, older).size() >= written.size());
  }
}

TEST_CASE("objects/write: an R2004 entity carries no links and a color reference in its handles")
{
  Entities::Line line;
  line.Base.Self = Handle{0x50};
  line.Base.Owner = Handle{0x1F};
  line.Common.Layer = Handle{0x10};
  line.Common.Previous = Handle{0x4F};
  line.Common.Next = Handle{0x51};
  line.Common.Color.Rgb = static_cast<int>(0xC2FF0000);
  line.Common.Color.Reference = Handle{0x99};
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2004, .Text = &Latin, .Issues = &issues};
  const Objects::FileContext reading{.Release = Version::R2004, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> back = ReadObject(WriteObject(Object{line}, with), 0, reading);
  REQUIRE(back);
  CHECK(CommonOf(*back)->Previous == Handle{});
  CHECK(CommonOf(*back)->Next == Handle{});
  CHECK(CommonOf(*back)->Color.Reference == Handle{0x99});
  CHECK(CommonOf(*back)->Color.Rgb == line.Common.Color.Rgb);
  CHECK(issues.empty());
}

TEST_CASE("objects/write: every object of the R2004 fixtures writes back the section's bytes")
{
  for (const std::string_view name : {"tables", "entities", "curves", "objects", "empty", "foundation"})
  {
    CAPTURE(name);
    std::vector<Issue> issues;
    const File::Paged::Layout layout = *File::Paged::ReadLayout(ReadFixture(std::format("{}-r2004.dwg", name)), issues);
    const File::Paged::Section &objects = layout.Sections[5];
    const File::Paged::Section &map = layout.Sections[8];
    const File::Paged::Section &section = layout.Sections[9];
    REQUIRE(objects.Name == "objects");
    REQUIRE(section.Name == "classes");
    const std::span<const std::byte> framed = std::span{section.Bytes}.first(section.Bytes.size() - Classes::Trailer);
    const File::Locator::Record whole{.Number = 1, .Seeker = 0, .Size = 0};
    const std::vector<std::byte> body = File::Locator::Cut(framed, whole, File::Locator::Sentinels::Classes)->Body;
    const Fields::Context with{.Release = Version::R2004, .Text = &Latin, .Issues = &issues};
    const std::vector<Class> classes = Classes::Read(body, with);
    const Fields::Context writing{.Release = Version::R2004, .Text = &Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{
        .Release = Version::R2004, .Text = &Latin, .Classes = classes, .Issues = &issues};
    std::size_t typed = 0;
    std::string differing;
    for (const Mapped &entry : ReadMap(map.Bytes, 0, issues))
    {
      const std::optional<Object> object = ReadObject(objects.Bytes, entry.Offset, reading);
      REQUIRE_MESSAGE(object, HexOf(entry.Self));
      const std::vector<std::byte> written = WriteObject(*object, writing);
      REQUIRE(written.size() <= objects.Bytes.size() - entry.Offset);
      const std::span<const std::byte> kept = std::span{objects.Bytes}.subspan(entry.Offset, written.size());
      if (const auto [ours, theirs] = std::ranges::mismatch(written, kept); ours != written.end())
      {
        const std::size_t size = std::to_integer<std::size_t>(objects.Bytes[entry.Offset]) |
                                 (std::to_integer<std::size_t>(objects.Bytes[entry.Offset + 1]) << 8U);
        const auto [after, other] = std::ranges::mismatch(std::span{written}.subspan(8), kept.subspan(8));
        std::string colors;
        if (const auto *style = GetIf<Tables::DimStyle>(&*object))
        {
          colors = std::format("colors {} {} {} lw {} {}", style->Dimclrd.Index, style->Dimclre.Index,
                               style->Dimclrt.Index, style->Dimlwd, style->Dimlwe);
        }
        differing += std::format("{} {} at {} (ours {} theirs {} data differs at {} {}); ", HexOf(entry.Self),
                                 TypeOf(*object), ours - written.begin(), written.size() - 4, size,
                                 8 + (after - std::span{written}.subspan(8).begin()), colors);
      }
      typed += Holds<Unknown>(*object) ? 0U : 1U;
    }
    MESSAGE(differing);
    CHECK(differing.empty());
    CHECK(typed >= 150);
    std::string whats;
    for (const Issue &issue : issues)
    {
      whats += std::format("{} {} {}; ", issue.Offset.value_or(0), HexOf(issue.Subject), issue.What);
    }
    MESSAGE(whats);
    CHECK(issues.empty());
  }
}

namespace
{
/**
 *  An object written as one release and read back as it.
 */
auto Across(const Object &object, Version release) -> Object
{
  std::vector<Issue> issues;
  const std::vector<Class> classes{*Classes::FindDeclared("LAYOUT")};
  const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = classes, .Issues = &issues};
  const std::optional<Object> back = ReadObject(WriteObject(object, with), 0, reading);
  REQUIRE(back);
  CHECK(issues.empty());
  return *back;
}
} // namespace

TEST_CASE("objects/write: the members R2004 added round-trip as R2004 and are dropped as R2000")
{
  Tables::BlockRecord record;
  record.Record.Base.Self = Handle{0x50};
  record.Record.Name = "Desk";
  record.Block = Handle{0x51};
  record.EndBlk = Handle{0x54};
  record.Entities = {Handle{0x52}, Handle{0x53}};
  CHECK(Get<Tables::BlockRecord>(Across(Object{record}, Version::R2004)).Entities == record.Entities);
  CHECK(Get<Tables::BlockRecord>(Across(Object{record}, Version::R2000)).Entities.empty());

  Entities::Insert insert;
  insert.Base.Self = Handle{0x60};
  insert.Common.Layer = Handle{0x10};
  insert.Block = Handle{0x50};
  insert.Follows = true;
  insert.Owned = {Handle{0x61}, Handle{0x62}};
  insert.SeqEnd = Handle{0x63};
  CHECK(Get<Entities::Insert>(Across(Object{insert}, Version::R2004)).Owned == insert.Owned);
  CHECK(Get<Entities::Insert>(Across(Object{insert}, Version::R2000)).Owned.empty());
  insert.Columns = 2;
  insert.Rows = 2;
  CHECK(Get<Entities::Insert>(Across(Object{insert}, Version::R2004)).Owned == insert.Owned);

  Entities::Polyline polyline;
  polyline.Base.Self = Handle{0x70};
  polyline.Common.Layer = Handle{0x10};
  polyline.Vertices = {Handle{0x71}, Handle{0x72}, Handle{0x73}};
  polyline.SeqEnd = Handle{0x74};
  CHECK(Get<Entities::Polyline>(Across(Object{polyline}, Version::R2004)).Vertices == polyline.Vertices);
  CHECK(Get<Entities::Polyline>(Across(Object{polyline}, Version::R2000)).Vertices.empty());

  Entities::Viewport viewport;
  viewport.Base.Self = Handle{0x80};
  viewport.Common.Layer = Handle{0x10};
  viewport.ShadePlotMode = 3;
  viewport.Frozen = {Handle{0x11}};
  CHECK(Get<Entities::Viewport>(Across(Object{viewport}, Version::R2004)).ShadePlotMode == 3);
  CHECK(Get<Entities::Viewport>(Across(Object{viewport}, Version::R2004)).Frozen == viewport.Frozen);
  CHECK(Get<Entities::Viewport>(Across(Object{viewport}, Version::R2000)).ShadePlotMode == 0);

  Entities::MText mtext;
  mtext.Base.Self = Handle{0x90};
  mtext.Common.Layer = Handle{0x10};
  mtext.Value = "Hello";
  // A CMC of R2004 always carries its true color, so the color read back has one.
  mtext.Background =
      Entities::Background{.Fill = 1, .Margin = 2.0, .Color = Color{.Index = 0, .Rgb = 0xC20000FF}, .Transparency = 5};
  CHECK(Get<Entities::MText>(Across(Object{mtext}, Version::R2004)).Background == mtext.Background);
  CHECK(Get<Entities::MText>(Across(Object{mtext}, Version::R2000)).Background == Entities::Background{});
  mtext.Background.Fill = 0;
  CHECK(Get<Entities::MText>(Across(Object{mtext}, Version::R2004)).Background.Margin == 1.5);

  Entities::Hatch hatch;
  hatch.Base.Self = Handle{0xA0};
  hatch.Common.Layer = Handle{0x10};
  hatch.Solid = true;
  hatch.Gradient =
      Entities::Gradient{.Shown = 1,
                         .Unknown1 = 0,
                         .Angle = 0.5,
                         .Shift = 0.25,
                         .Single = 0,
                         .Tint = 1.0,
                         .Colors = {Entities::GradientColor{.Place = 0, .Unknown1 = 0, .Rgb = 0xFF, .Unknown2 = 0},
                                    Entities::GradientColor{.Place = 1, .Unknown1 = 0, .Rgb = 0xFF00, .Unknown2 = 0}},
                         .Name = "LINEAR"};
  CHECK(Get<Entities::Hatch>(Across(Object{hatch}, Version::R2004)).Gradient == hatch.Gradient);
  CHECK(Get<Entities::Hatch>(Across(Object{hatch}, Version::R2000)).Gradient == Entities::Gradient{});

  Objects::Layout layout;
  layout.Base.Self = Handle{0xB0};
  layout.Name = "Layout1";
  layout.Plot.Shading = 2;
  layout.Plot.Resolution = 3;
  layout.Plot.Dots = 150;
  layout.PlotView = Handle{0xB1};
  layout.Viewports = {Handle{0xB2}, Handle{0xB3}};
  const auto back = Get<Objects::Layout>(Across(Object{layout}, Version::R2004));
  CHECK(back.Plot.Shading == 2);
  CHECK(back.Plot.Dots == 150);
  CHECK(back.PlotView == layout.PlotView);
  CHECK(back.Viewports == layout.Viewports);
  const auto older = Get<Objects::Layout>(Across(Object{layout}, Version::R2000));
  CHECK(older.Viewports.empty());
  CHECK(older.PlotView == Handle{});
  CHECK(older.Plot.Dots == 300); // The default, which is what AutoCAD writes for a plain plot.

  Tables::DimStyle style;
  style.Record.Base.Self = Handle{0xC0};
  style.Record.Name = "Colored";
  style.Dimclrd = Color{
      .Index = 0, .Rgb = 0xC2FF0000, .Names = ColorNames{.Name = std::string{"Red"}, .Book = std::string{"Book"}}};
  CHECK(Get<Tables::DimStyle>(Across(Object{style}, Version::R2004)).Dimclrd == style.Dimclrd);
  CHECK(Get<Tables::DimStyle>(Across(Object{style}, Version::R2000)).Dimclrd == Color{.Index = 0});
}
} // namespace DwgIO::Objects
