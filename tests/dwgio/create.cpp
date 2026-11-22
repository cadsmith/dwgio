/***********************************************************************************************************************
 *  @file      tests/dwgio/create.cpp
 *  @brief     A drawing made from nothing: what it holds, and its saves in every release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/create.hpp"

#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/placeholder.hpp"
#include "dwgio/save.hpp"
#include "dwgio/section.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tables/vport.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO
{
TEST_CASE("create: a new drawing saves and loads back clean and equal")
{
  const Drawing drawing = Create();
  CHECK(drawing.Release == Version::R2000);
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  Drawing again = back->Drawing;
  CHECK(again.Order.size() == 7);
  CHECK(again.Kept.size() == 3);
  again.Order.clear(); // The file's layout, which a drawing from nothing has none of.
  again.Kept.clear();
  CHECK(again == drawing);
  QueueAudit(saved->Bytes, "dwgio-create.dwg");
  CHECK(Create(Version::R14).Release == Version::R14);
}

TEST_CASE("create: a new drawing saves as R14 and loads back clean")
{
  const Drawing drawing = Create(Version::R14);
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  CHECK(back->Drawing.Release == Version::R14);
  Drawing again = back->Drawing;
  again.Order.clear();
  again.Kept.clear();
  CHECK(again == drawing);
  CHECK(drawing.Header.Attreq);
  CHECK(CommonOf(*Find(drawing, View::Block(drawing, "*Model_Space")->Block))->Lineweight == std::nullopt);
  QueueAudit(saved->Bytes, "dwgio-create-r14.dwg");
}

TEST_CASE("create: a drawing made as R2010 R2013 or R2018 saves and loads back equal")
{
  for (const Version release : {Version::R2010, Version::R2013, Version::R2018})
  {
    CAPTURE(NameOf(release));
    const Drawing drawing = Create(release);
    CHECK(drawing.Header.Lenslength == 50);
    CHECK(drawing.Header.Latitude == doctest::Approx(37.795));
    CHECK(drawing.Header.Menuname == ".");
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(back->Drawing.Release == release);
    Drawing again = back->Drawing;
    again.Order.clear();
    again.Kept.clear();
    again.Classes = drawing.Classes; // A paged save stamps the classes in use with their counts and versions.
    CHECK(again == drawing);
    QueueAudit(saved->Bytes, std::format("dwgio-create-{}.dwg", NameOf(release)));
  }
}

TEST_CASE("create: a new drawing holds every object the header's roots name")
{
  const Drawing drawing = Create();
  const Header &header = drawing.Header;
  for (const Tables::Table table : {Tables::Table::Block, Tables::Table::Layer, Tables::Table::Style,
                                    Tables::Table::LType, Tables::Table::View, Tables::Table::Ucs, Tables::Table::VPort,
                                    Tables::Table::AppId, Tables::Table::DimStyle, Tables::Table::ViewportEntityHeader})
  {
    CAPTURE(static_cast<int>(table));
    CHECK(View::Control(drawing, table) != nullptr);
  }
  CHECK(View::Control(drawing, Tables::Table::Block)->Base.Self == header.BlockControlObject);
  CHECK(View::Control(drawing, Tables::Table::Layer)->Base.Self == header.LayerControlObject);
  CHECK(View::Control(drawing, Tables::Table::LType)->Base.Self == header.LTypeControlObject);
  CHECK(View::Control(drawing, Tables::Table::ViewportEntityHeader)->Base.Self ==
        header.ViewportEntityHeaderControlObject);
  const Tables::Layer *zero = View::Record<Tables::Layer>(drawing, "0");
  REQUIRE(zero);
  CHECK(zero->Record.Base.Self == header.Clayer);
  CHECK(zero->Linetype == header.LTypeContinuous);
  CHECK(View::Record<Tables::LType>(drawing, "ByBlock")->Record.Base.Self == header.LTypeByBlock);
  CHECK(View::Record<Tables::LType>(drawing, "ByLayer")->Record.Base.Self == header.LTypeByLayer);
  CHECK(View::Record<Tables::LType>(drawing, "Continuous")->Record.Base.Self == header.LTypeContinuous);
  CHECK(View::Control(drawing, Tables::Table::LType)->ByBlock == header.LTypeByBlock);
  CHECK(View::Control(drawing, Tables::Table::LType)->ByLayer == header.LTypeByLayer);
  CHECK(View::Record<Tables::Style>(drawing, "Standard")->Record.Base.Self == header.Textstyle);
  CHECK(View::Record<Tables::AppId>(drawing, "ACAD") != nullptr);
  CHECK(View::Record<Tables::DimStyle>(drawing, "Standard")->Dimtxsty == header.Textstyle);
  const Tables::DimStyle *current = View::Record<Tables::DimStyle>(drawing, "ISO-25");
  REQUIRE(current);
  CHECK(current->Record.Base.Self == header.Dimstyle);
  CHECK(current->Dimasz == header.Dimasz);
  CHECK(current->Dimtxsty == header.Textstyle);
  CHECK(View::Record<Tables::VPort>(drawing, "*Active") != nullptr);
  const Tables::BlockRecord *model = View::Block(drawing, "*Model_Space");
  const Tables::BlockRecord *paper = View::Block(drawing, "*Paper_Space");
  REQUIRE(model);
  REQUIRE(paper);
  CHECK(model->Record.Base.Self == header.BlockRecordModelSpace);
  CHECK(paper->Record.Base.Self == header.BlockRecordPaperSpace);
  CHECK(View::Control(drawing, Tables::Table::Block)->ModelSpace == model->Record.Base.Self);
  CHECK(View::Control(drawing, Tables::Table::Block)->PaperSpace == paper->Record.Base.Self);
  for (const Tables::BlockRecord *record : {model, paper})
  {
    CHECK(Holds<Entities::Block>(*Find(drawing, record->Block)));
    CHECK(Holds<Entities::EndBlk>(*Find(drawing, record->EndBlk)));
    CHECK(Get<Entities::Block>(*Find(drawing, record->Block)).Name == record->Record.Name);
    CHECK(View::Entities(drawing, *record).empty());
    CHECK(Holds<Objects::Layout>(*Find(drawing, record->Layout)));
  }
  const std::vector<const Objects::Layout *> layouts = View::Layouts(drawing);
  REQUIRE(layouts.size() == 2);
  CHECK(layouts[0]->Name == "Model");
  CHECK(layouts[0]->Block == model->Record.Base.Self);
  CHECK(layouts[1]->Name == "Layout1");
  CHECK(layouts[1]->Block == paper->Record.Base.Self);
  CHECK(layouts[1]->Order == 1);
  const Objects::Dictionary *root = View::Root(drawing);
  REQUIRE(root);
  CHECK(root->Base.Self == header.DictionaryNamedObjects);
  CHECK(View::Dictionary(drawing, "ACAD_GROUP")->Base.Self == header.DictionaryAcadGroup);
  CHECK(View::Dictionary(drawing, "ACAD_MLINESTYLE")->Base.Self == header.DictionaryAcadMLineStyle);
  CHECK(View::Dictionary(drawing, "ACAD_LAYOUT")->Base.Self == header.DictionaryLayouts);
  CHECK(View::Dictionary(drawing, "ACAD_PLOTSETTINGS")->Base.Self == header.DictionaryPlotSettings);
  const Objects::Dictionary *styles = View::Dictionary(drawing, "ACAD_PLOTSTYLENAME");
  REQUIRE(styles);
  CHECK(styles->Base.Self == header.DictionaryPlotStyles);
  REQUIRE(styles->Default);
  const Object *normal = View::Entry(drawing, *styles, "Normal");
  REQUIRE(normal);
  CHECK(Holds<Objects::Placeholder>(*normal));
  CHECK(BaseOf(*normal).Self == *styles->Default);
  CHECK(header.Cpsnid == Handle{});
  const Object *standard = View::Entry(drawing, *View::Dictionary(drawing, "ACAD_MLINESTYLE"), "Standard");
  REQUIRE(standard);
  CHECK(BaseOf(*standard).Self == header.Cmlstyle);
  CHECK(Get<Objects::MLineStyle>(*standard).Elements.size() == 2);
  for (const Object &object : drawing.Objects)
  {
    CAPTURE(HexOf(BaseOf(object).Self));
    CHECK(BaseOf(object).Self.Value < header.Handseed.Value);
    CHECK(BaseOf(object).Self != Handle{});
    CHECK((BaseOf(object).Owner == Handle{} || Find(drawing, BaseOf(object).Owner) != nullptr));
  }
  CHECK(drawing.Classes.size() == 3);
}

TEST_CASE("create: an R2004 drawing holds the block lists and the R2004 header")
{
  Drawing drawing = Create(Version::R2004);
  CHECK(drawing.Release == Version::R2004);
  CHECK(drawing.Header.Dimassoc == 2);
  CHECK(View::Block(drawing, "*Model_Space")->First == Handle{});
  Entities::Line line;
  line.End = Vector{.X = 1};
  CHECK(Edit::Add(drawing, line));
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  CHECK(back->Drawing.Objects == drawing.Objects);
  CHECK(back->Drawing.Header == drawing.Header);
  QueueAudit(saved->Bytes, "dwgio-created-r2004.dwg");
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("create: a drawing made in each release carries the stamps AutoCAD gives a new drawing")
{
  for (const Version release : FixtureReleases)
  {
    CAPTURE(NameOf(release));
    const Drawing drawing = Create(release);
    CHECK(drawing.Maintenance == MaintenanceOf(release));
    std::string name = std::format("empty-{}.dwg", NameOf(release));
    std::ranges::transform(name, name.begin(), [](char letter) -> char { return letter == 'R' ? 'r' : letter; });
    const std::vector<std::byte> theirs = ReadFixture(name);
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    // The code, the five zeros, the maintenance version and the flag; past the preview's address, the application's
    // version and maintenance and the code page, then from R2004 the three bytes that repeat them.
    const std::vector<std::byte> &ours = saved->Bytes;
    CHECK(std::ranges::equal(std::span{ours}.first(0x0D), std::span{theirs}.first(0x0D)));
    const std::size_t stamped = release >= Version::R2004 ? 0x07 : 0x04;
    CHECK(std::ranges::equal(std::span{ours}.subspan(0x11, stamped), std::span{theirs}.subspan(0x11, stamped)));
    const Drawing back = Load(ours).value().Drawing;
    CHECK(back.Maintenance == Load(theirs).value().Drawing.Maintenance);
    // From R2004 the save writes an auxiliary header, whose second short is the maintenance version.
    const auto auxiliary = std::ranges::find(back.Kept, std::string{"auxiliary"}, &Section::Name);
    REQUIRE((auxiliary != back.Kept.end()) == (release >= Version::R2004));
    if (auxiliary != back.Kept.end())
    {
      const std::vector<std::byte> &bytes = auxiliary->Bytes;
      CHECK(std::to_integer<int>(bytes[5]) + (std::to_integer<int>(bytes[6]) << 8) == MaintenanceOf(release));
    }
  }
}

TEST_CASE("create: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
