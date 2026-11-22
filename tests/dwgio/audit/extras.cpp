/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/extras.cpp
 *  @brief     The audit of classes, extended data, lineweights and colors a file cannot keep.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/findings.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/class.hpp"
#include "dwgio/color.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/lwpolyline.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/scale.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
constexpr Handle Nowhere{.Value = 0x999};

} // namespace

TEST_CASE("audit/extras: a missing class a kind is stored under is declared with AutoCAD's names and numbers")
{
  Drawing drawing = Create();
  std::erase_if(drawing.Classes, [](const Class &one) -> bool { return one.DxfName == "LAYOUT"; });
  Objects::Scale scale;
  scale.Base.Self = Edit::MakeHandle(drawing);
  scale.Base.Owner = drawing.Header.DictionaryNamedObjects;
  scale.Name = "1:1";
  drawing.Objects.emplace_back(scale);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) == std::vector<std::string>{"class LAYOUT not declared", "class SCALE not declared"});
  CHECK(checked.Errors() == 2);
  CHECK(drawing.Classes.size() == 2);
  CHECK(Audit::Repair(drawing).IsClean());
  REQUIRE(drawing.Classes.size() == 4);
  CHECK(drawing.Classes[2].DxfName == "LAYOUT");
  CHECK(drawing.Classes[2].CppName == "AcDbLayout");
  CHECK(drawing.Classes[2].Number == 502);
  CHECK(drawing.Classes[2].Application == "ObjectDBX Classes");
  CHECK(drawing.Classes[3].DxfName == "SCALE");
  CHECK(drawing.Classes[3].CppName == "AcDbScale");
  CHECK(drawing.Classes[3].Number == 503);
  CHECK(drawing.Classes[3].ProxyFlags == 1153);
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  CHECK(View::Layouts(Load(saved->Bytes).value().Drawing).size() == 2);
  CHECK(Audit::Check(drawing).Findings.empty());

  // A class no table of AutoCAD's knows cannot be made: the finding is left unfixed.
  Drawing strange = Create();
  Unknown unknown;
  unknown.Type = 500;
  unknown.Class = Symbol{"ACME_WIDGET"};
  unknown.Base.Self = Edit::MakeHandle(strange);
  unknown.Base.Owner = strange.Header.DictionaryNamedObjects;
  strange.Objects.emplace_back(unknown);
  strange.Classes.clear();
  const Audit::Report repaired = Audit::Repair(strange);
  CHECK(FindingTexts(repaired) ==
        std::vector<std::string>{"class ACDBDICTIONARYWDFLT not declared", "class ACDBPLACEHOLDER not declared",
                                 "class LAYOUT not declared", "class ACME_WIDGET not declared"});
  REQUIRE(repaired.Findings.size() == 4);
  CHECK(!repaired.Findings[3].Fixed);
  CHECK(repaired.Findings[0].Fixed);
  CHECK(!repaired.IsClean());
}

TEST_CASE("audit/extras: an R14 drawing must declare the class of a kind stored under one before R2000")
{
  Drawing older = Create(Version::R14);
  REQUIRE(Edit::Add(older, Entities::LwPolyline{}));
  CHECK(FindingTexts(Audit::Check(older)) == std::vector<std::string>{"class LWPOLYLINE not declared"});
  Audit::Repair(older);
  REQUIRE(older.Classes.size() == 4);
  CHECK(older.Classes[3].DxfName == "LWPOLYLINE");
  CHECK(older.Classes[3].CppName == "AcDbPolyline");
  CHECK(older.Classes[3].ItemClass == 0x1F2);
  CHECK(Audit::Check(older).Findings.empty());
  Drawing newer = Create();
  REQUIRE(Edit::Add(newer, Entities::LwPolyline{}));
  CHECK(Audit::Check(newer).Findings.empty());
}

TEST_CASE("audit/extras: xdata of an application the drawing lacks is dropped")
{
  Drawing drawing = Create();
  Entities::Line line;
  // A record with a group: a save leaves out one with none, which would hide the missing application.
  line.Base.Extended.push_back(
      Xdata{.Application = "", .AppId = Nowhere, .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string{"x"}}}});
  line.Base.Extended.push_back(
      Xdata{.Application = "", .AppId = View::Record<Tables::AppId>(drawing, "ACAD")->Record.Base.Self, .Tags = {}});
  const Handle drawn = *Edit::Add(drawing, line);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) == std::vector<std::string>{"application 999 of the extended data of LINE 2B missing"});
  CHECK(checked.Findings[0].Level == Audit::Severity::Error);
  CHECK(checked.Findings[0].Subject == drawn);
  Audit::Repair(drawing);
  CHECK(BaseOf(*Find(drawing, drawn)).Extended.size() == 1);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/extras: a lineweight or color outside what a file keeps falls back")
{
  Drawing drawing = Create();
  Entities::Line line;
  line.Common.Lineweight = static_cast<Lineweight>(25);
  line.Common.Color.Index = 300;
  const Handle drawn = *Edit::Add(drawing, line);
  Tables::Layer walls;
  walls.Record.Name = "Walls";
  walls.Linetype = drawing.Header.LTypeContinuous;
  walls.Color.Index = -7;
  walls.Lineweight = static_cast<Lineweight>(99);
  const Handle layered = *Edit::Add(drawing, walls);
  const Audit::Report checked = Audit::Check(drawing);
  CHECK(FindingTexts(checked) ==
        std::vector<std::string>{"lineweight 25 of LINE 2B unknown", "color 300 of LINE 2B out of range",
                                 "lineweight 99 of LAYER 2C unknown", "color -7 of LAYER 2C out of range"});
  CHECK(checked.Warnings() == 2);
  CHECK(checked.Errors() == 2);
  CHECK(checked.Findings[0].Level == Audit::Severity::Warning);
  CHECK(checked.Findings[1].Level == Audit::Severity::Error);
  Audit::Repair(drawing);
  CHECK(CommonOf(*Find(drawing, drawn))->Lineweight == Lineweight::ByLayer);
  CHECK(CommonOf(*Find(drawing, drawn))->Color.Index == 256);
  CHECK(Get<Tables::Layer>(*Find(drawing, layered)).Lineweight == Lineweight::Default);
  CHECK(Get<Tables::Layer>(*Find(drawing, layered)).Color.Index == 7);
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("audit/extras: a true color reloads with AutoCAD's leading index 0 and is not out of range")
{
  for (const Version release : {Version::R2004, Version::R2018})
  {
    CAPTURE(NameOf(release));
    Drawing drawing = Create(release);
    Tables::Layer brick;
    brick.Record.Name = "Brick";
    brick.Linetype = drawing.Header.LTypeContinuous;
    brick.Color = Color{.Index = 1, .Rgb = static_cast<int>(0xC2B4643CU)};
    const Handle layered = *Edit::Add(drawing, brick);
    Entities::Line wall;
    wall.Common.Layer = layered;
    wall.Common.Color = Color{.Index = 0, .Rgb = static_cast<int>(0xC2102030U)};
    const Handle drawn = *Edit::Add(drawing, wall);
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    Drawing back = Load(saved->Bytes).value().Drawing;
    const auto &layer = Get<Tables::Layer>(*Find(back, layered));
    CHECK(layer.Color.Index == 0);
    CHECK(layer.Color.Rgb == static_cast<int>(0xC2B4643CU));
    CHECK(CommonOf(*Find(back, drawn))->Color.Rgb == static_cast<int>(0xC2102030U));
    CHECK(FindingTexts(Audit::Check(back)).empty());
    Audit::Repair(back);
    CHECK(Get<Tables::Layer>(*Find(back, layered)).Color.Index == 0);
  }
}

TEST_CASE("audit/extras: findings on the broken-extras drawings match AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  // A class not declared is declared again from the classes AutoCAD knows: the layouts stay and AutoCAD is quiet.
  Drawing classes = Create();
  std::erase_if(classes.Classes, [](const Class &one) -> bool { return one.DxfName == "LAYOUT"; });
  const std::expected<Saved, Failure> undeclared = Save(classes);
  REQUIRE(undeclared);
  CHECK(undeclared->Issues.empty());
  CHECK(View::Layouts(Load(undeclared->Bytes).value().Drawing).size() == 2);
  CHECK(PassesAudit(undeclared->Bytes, "dwgio-audit-classes.dwg"));
  Audit::Repair(classes);
  CHECK(PassesAudit(Save(classes).value().Bytes, "dwgio-audit-classes-repaired.dwg"));
  Drawing xdata = Create();
  Entities::Line line;
  // A record with a group: a save leaves out one with none, which would hide the missing application.
  line.Base.Extended.push_back(
      Xdata{.Application = "", .AppId = Nowhere, .Tags = {DwgIO::Tag{.Code = 1000, .Value = std::string{"x"}}}});
  REQUIRE(Edit::Add(xdata, line));
  CHECK(!PassesAudit(Save(xdata).value().Bytes, "dwgio-audit-xdata.dwg"));
  Drawing widths = Create();
  Entities::Line wide;
  wide.Common.Lineweight = static_cast<Lineweight>(25);
  REQUIRE(Edit::Add(widths, wide));
  CHECK(PassesAudit(Save(widths).value().Bytes, "dwgio-audit-widths.dwg"));
  Drawing colors = Create();
  Entities::Line bright;
  bright.Common.Color.Index = 300;
  REQUIRE(Edit::Add(colors, bright));
  CHECK(!PassesAudit(Save(colors).value().Bytes, "dwgio-audit-colors.dwg"));
  Drawing layers = Create();
  Tables::Layer thick;
  thick.Record.Name = "Thick";
  thick.Linetype = layers.Header.LTypeContinuous;
  thick.Lineweight = static_cast<Lineweight>(99);
  REQUIRE(Edit::Add(layers, thick));
  CHECK(PassesAudit(Save(layers).value().Bytes, "dwgio-audit-layer-width.dwg"));
  Drawing hues = Create();
  Tables::Layer dark;
  dark.Record.Name = "Dark";
  dark.Linetype = hues.Header.LTypeContinuous;
  dark.Color.Index = -7;
  REQUIRE(Edit::Add(hues, dark));
  CHECK(!PassesAudit(Save(hues).value().Bytes, "dwgio-audit-layer-color.dwg"));
}
} // namespace DwgIO
