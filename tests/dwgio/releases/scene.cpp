/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/scene.cpp
 *  @brief     Lights, materials, visual styles and sections saved across releases.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/scene.hpp"

#include "classes/catalog.hpp"
#include "file/coded/layout.hpp"
#include "file/paged/layout.hpp"
#include "releases/binding.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/class.hpp"
#include "dwgio/color.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/helix.hpp"
#include "dwgio/entities/light.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/mesh.hpp"
#include "dwgio/entities/section.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/material.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  How many objects of each of the scene's kinds a drawing types.
 */
auto Scene(const Drawing &drawing) -> std::map<std::string, std::size_t>
{
  std::map<std::string, std::size_t> counts;
  for (const Object &object : drawing.Objects)
  {
    if (Holds<Entities::Helix>(object) || Holds<Entities::Mesh>(object) || Holds<Entities::Light>(object) ||
        Holds<Entities::SectionObject>(object) || Holds<Objects::Material>(object) ||
        Holds<Objects::VisualStyle>(object))
    {
      ++counts[std::string{TypeOf(object)}];
    }
  }
  return counts;
}

/**
 *  The sections of two paged or coded files, the preview of a coded file aside: its images' starts are addresses.
 */
template <typename LayoutT>
void CheckSame(const LayoutT &theirs, const LayoutT &ours)
{
  REQUIRE(ours.Sections.size() == theirs.Sections.size());
  for (std::size_t at = 0; at < theirs.Sections.size(); ++at)
  {
    CAPTURE(theirs.Sections[at].Name);
    if (theirs.Sections[at].Name != "preview")
    {
      CHECK((RemakesSection(theirs.Sections[at].Name) || ours.Sections[at].Bytes == theirs.Sections[at].Bytes));
    }
  }
}

auto Light(const Drawing &drawing) -> const Entities::Light &
{
  const auto found = std::ranges::find_if(drawing.Objects,
                                          [](const Object &object) -> bool { return Holds<Entities::Light>(object); });
  REQUIRE(found != drawing.Objects.end());
  return Get<Entities::Light>(*found);
}
} // namespace

TEST_CASE("releases/scene: the kinds and material fixtures save to their own release byte for byte")
{
  for (const std::string_view fixture : {"kinds", "material"})
  {
    for (const std::string_view release : FixtureSuffixes)
    {
      const std::string name = std::format("{}-{}.dwg", fixture, release);
      CAPTURE(name);
      const std::vector<std::byte> bytes = ReadFixture(name);
      const std::expected<Loaded, Failure> loaded = Load(bytes);
      REQUIRE(loaded);
      CHECK(loaded->Issues.empty());
      const std::map<std::string, std::size_t> counts = Scene(loaded->Drawing);
      CHECK(counts.at("MATERIAL") == (fixture == "kinds" ? 3 : 4));
      CHECK(counts.at("VISUALSTYLE") == (fixture == "kinds" ? 28 : 24));
      if (fixture == "kinds")
      {
        CHECK(counts.at("LIGHT") == 3);
        CHECK(counts.at("HELIX") == 1);
        CHECK(counts.at("MESH") == 1);
        CHECK(counts.at("SECTIONOBJECT") == 1);
      }
      const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
      REQUIRE(saved);
      CHECK(saved->Issues.empty());
      const std::expected<Loaded, Failure> again = Load(saved->Bytes);
      REQUIRE(again);
      CHECK(again->Drawing == loaded->Drawing);
      std::vector<Issue> issues;
      if (loaded->Drawing.Release <= Version::R2000)
      {
        CHECK(saved->Bytes == bytes);
      }
      else if (loaded->Drawing.Release == Version::R2007)
      {
        CheckSame(*File::Coded::ReadLayout(bytes, issues), *File::Coded::ReadLayout(saved->Bytes, issues));
      }
      else
      {
        CheckSame(*File::Paged::ReadLayout(bytes, issues), *File::Paged::ReadLayout(saved->Bytes, issues));
      }
      CHECK(issues.empty());
    }
  }
}

TEST_CASE("releases/scene: the kinds and material and empty fixtures saved to other releases load back clean")
{
  struct Crossing
  {
    std::string_view Name;
    Version To;
  };

  for (const Crossing crossing :
       {Crossing{.Name = "kinds-r2018", .To = Version::R2000}, Crossing{.Name = "kinds-r2000", .To = Version::R2018},
        Crossing{.Name = "kinds-r2007", .To = Version::R2013}, Crossing{.Name = "kinds-r2004", .To = Version::R14},
        Crossing{.Name = "kinds-r2013", .To = Version::R2018}, Crossing{.Name = "kinds-r2018", .To = Version::R2013},
        Crossing{.Name = "kinds-r14", .To = Version::R2007}, Crossing{.Name = "material-r2018", .To = Version::R2000},
        Crossing{.Name = "material-r2000", .To = Version::R2018},
        Crossing{.Name = "material-r2004", .To = Version::R2007},
        Crossing{.Name = "material-r2018", .To = Version::R2010}, Crossing{.Name = "empty-r2007", .To = Version::R2018},
        Crossing{.Name = "empty-r2018", .To = Version::R2007}})
  {
    CAPTURE(crossing.Name);
    CAPTURE(NameOf(crossing.To));
    const Drawing drawing = Load(ReadFixture(std::format("{}.dwg", crossing.Name))).value().Drawing;
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = crossing.To});
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(back->Drawing.Release == crossing.To);
    CHECK(Scene(back->Drawing) == Scene(drawing));
    QueueAudit(saved->Bytes, std::format("dwgio-{}-as-{}.dwg", crossing.Name, NameOf(crossing.To)));
  }
}

TEST_CASE("releases/scene: a light below R2018 carries AutoCAD's object version marker which R2018 drops")
{
  const Drawing newer = Load(ReadFixture("kinds-r2018.dwg")).value().Drawing;
  const Drawing older = Load(ReadFixture("kinds-r2000.dwg")).value().Drawing;
  REQUIRE(Light(newer).Base.Extended.empty());
  REQUIRE(Light(older).Base.Extended.size() == 1);
  const std::expected<Saved, Failure> lowered = Save(newer, SaveOptions{.Release = Version::R2000});
  REQUIRE(lowered);
  const Drawing down = Load(lowered->Bytes).value().Drawing;
  CHECK(Light(down).Base.Extended == Light(older).Base.Extended);
  CHECK(Light(down).Color.Index == 0); // R2004's true white dropped and noted.
  CHECK(std::ranges::contains(lowered->Issues,
                              std::format("true color of LIGHT {} not kept by R2000", HexOf(Light(newer).Base.Self)),
                              &Issue::What));
  const std::expected<Saved, Failure> raised = Save(older, SaveOptions{.Release = Version::R2018});
  REQUIRE(raised);
  CHECK(Light(Load(raised->Bytes).value().Drawing).Base.Extended.empty());

  // A light whose ACAD data holds more keeps it, the marker before it; and no marker for a drawing with no ACAD.
  Drawing drawing = newer;
  Entities::Light &light = Get<Entities::Light>(*Find(drawing, Light(newer).Base.Self));
  const Handle acad = View::Record<Tables::AppId>(drawing, "ACAD")->Record.Base.Self;
  light.Base.Extended.push_back(Xdata{.Application = {}, .AppId = Handle{0x12345}, .Tags = {}});
  light.Base.Extended.push_back(
      Xdata{.Application = {}, .AppId = acad, .Tags = {Tag{.Code = 1000, .Value = std::string{"Mine"}}}});
  light.Color = Color{.Index = 0, .Rgb = -939524096};
  std::vector<Issue> issues;
  const Way down2000{.Drawing = &drawing, .From = Version::R2018, .To = Version::R2000};
  Object marked{light};
  ConvertScene(marked, down2000, issues);
  const auto &groups = Get<Entities::Light>(marked).Base.Extended.back().Tags;
  REQUIRE(groups.size() == 3);
  CHECK(groups.front().Value == Content{std::string{"AcDbSavedByObjectVersion"}});
  CHECK(Get<Entities::Light>(marked).Color == Color{.Index = 257});
  CHECK(issues.empty());
  Object unmarked = marked;
  ConvertScene(unmarked, Way{.Drawing = &drawing, .From = Version::R2000, .To = Version::R2018}, issues);
  CHECK(Get<Entities::Light>(unmarked).Base.Extended.back().Tags.size() == 1);
  // ACAD's data opening with a short gets the marker before it.
  Entities::Light shorter;
  shorter.Base.Extended.push_back(
      Xdata{.Application = {}, .AppId = acad, .Tags = {Tag{.Code = 1070, .Value = std::int64_t{5}}}});
  Object shortly{shorter};
  ConvertScene(shortly, down2000, issues);
  CHECK(Get<Entities::Light>(shortly).Base.Extended.front().Tags.size() == 3);
  // A light already marked keeps one marker when saved below R2018 again.
  Object twice = marked;
  ConvertScene(twice, down2000, issues);
  CHECK(twice == marked);
  CHECK(Get<Entities::Light>(unmarked).Color.Rgb == -939524096);
  Drawing bare = Create(Version::R2018);
  std::erase_if(bare.Objects, [](const Object &one) { return Holds<Tables::AppId>(one); });
  Object plain{Entities::Light{}};
  ConvertScene(plain, Way{.Drawing = &bare, .From = Version::R2018, .To = Version::R2013}, issues);
  CHECK(Get<Entities::Light>(plain).Base.Extended.empty());
  ConvertScene(plain, Way{.Drawing = &bare, .From = Version::R2013, .To = Version::R2018}, issues);
  CHECK(Get<Entities::Light>(plain).Base.Extended.empty());
  // The same release asks for nothing.
  Object same{light};
  ConvertScene(same, Way{.Drawing = &drawing, .From = Version::R2018, .To = Version::R2018}, issues);
  CHECK(same == Object{light});
}

TEST_CASE("releases/scene: a visual style an older AutoCAD left unmarked takes the marker below R2018")
{
  // An R2007 file an older AutoCAD wrote, as the break line symbol AutoCAD ships, holds visual styles without the
  // marker; AutoCAD refuses them saved to R2004 so.
  Drawing older = Load(ReadFixture("objects-r2007.dwg")).value().Drawing;
  std::size_t styles = 0;
  for (Object &object : older.Objects)
  {
    if (auto *style = GetIf<Objects::VisualStyle>(&object))
    {
      REQUIRE(style->Base.Extended.size() == 1);
      style->Base.Extended.clear();
      ++styles;
    }
  }
  REQUIRE(styles > 0);
  for (const Version release : {Version::R14, Version::R2000, Version::R2004})
  {
    CAPTURE(NameOf(release));
    const std::expected<Saved, Failure> saved = Save(older, SaveOptions{.Release = release});
    REQUIRE(saved);
    const Drawing down = Load(saved->Bytes).value().Drawing;
    std::size_t marked = 0;
    for (const Object &object : down.Objects)
    {
      if (const auto *style = GetIf<Objects::VisualStyle>(&object))
      {
        REQUIRE(style->Base.Extended.size() == 1);
        CHECK(style->Base.Extended.front().Tags.front().Value == Content{std::string{"AcDbSavedByObjectVersion"}});
        ++marked;
      }
    }
    CHECK(marked == styles);
    QueueAudit(saved->Bytes, std::format("dwgio-objects-unmarked-as-{}.dwg", NameOf(release)));
  }
}

TEST_CASE("releases/scene: a mesh with overrides stays in its release and section settings convert")
{
  Drawing drawing = Load(ReadFixture("kinds-r2018.dwg")).value().Drawing;
  const auto mesh =
      std::ranges::find_if(drawing.Objects, [](const Object &object) -> bool { return Holds<Entities::Mesh>(object); });
  REQUIRE(mesh != drawing.Objects.end());
  CHECK(BindingOf(*mesh, drawing, Version::R2013).empty());
  Get<Entities::Mesh>(*mesh).Overridden = 1;
  CHECK(BindingOf(*mesh, drawing, Version::R2013) == "overrides");
  CHECK(BindingOf(*mesh, drawing, Version::R2018).empty());
  CHECK(BindingOf(Object{Unknown{}}, drawing, Version::R2013) == "bytes");
  const Handle self = BaseOf(*mesh).Self;
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2013});
  REQUIRE(saved);
  CHECK(std::ranges::contains(
      saved->Issues, std::format("object {} left out, its R2018 overrides cannot be written as R2013", HexOf(self)),
      &Issue::What));
  const Drawing back = Load(saved->Bytes).value().Drawing;
  CHECK(Find(back, self) == nullptr);
  for (const Object &object : back.Objects)
  {
    if (const auto *section = GetIf<Entities::SectionObject>(&object))
    {
      // AutoCAD writes the settings alike in every release: they cross, untyped, and the section keeps them.
      const Object *settings = Find(back, section->Settings);
      REQUIRE(settings != nullptr);
      CHECK(TypeOf(*settings) == "SECTION_SETTINGS");
    }
  }
}

TEST_CASE("releases/scene: the scene kinds' classes match AutoCAD's declarations")
{
  for (const auto &[name, cpp, flags, entity] :
       {std::tuple{"HELIX", "AcDbHelix", 4095, true}, std::tuple{"MESH", "AcDbSubDMesh", 0, true},
        std::tuple{"LIGHT", "AcDbLight", 1025, true}, std::tuple{"SECTIONOBJECT", "AcDbSection", 1025, true},
        std::tuple{"MATERIAL", "AcDbMaterial", 1153, false}, std::tuple{"VISUALSTYLE", "AcDbVisualStyle", 4095, false}})
  {
    CAPTURE(name);
    const std::optional<Class> declared = Classes::FindDeclared(name);
    REQUIRE(declared);
    CHECK(declared->CppName == cpp);
    CHECK(declared->ProxyFlags == flags);
    CHECK((declared->ItemClass == 0x1F2) == entity);
    CHECK(declared->Application == (std::string_view{name} == "LIGHT" ? "SCENEOE" : "ObjectDBX Classes"));
  }
  // A drawing Create makes, given a light, saves with the class declared and loads it back typed.
  Drawing drawing = Create(Version::R2018);
  Entities::Light light;
  light.Name = "Lamp";
  const Handle lit = *Edit::Add(drawing, light);
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2000});
  REQUIRE(saved);
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  CHECK(Get<Entities::Light>(*Find(back->Drawing, lit)).Name == "Lamp");
  QueueAudit(saved->Bytes, "dwgio-created-light-as-r2000.dwg");
}

TEST_CASE("releases/scene: the audit reports a material or visual style that names nothing of its kind")
{
  Drawing drawing = Load(ReadFixture("kinds-r2018.dwg")).value().Drawing;
  CHECK(Audit::Check(drawing).Findings.empty());
  const Handle nowhere{.Value = 0x999};
  Entities::Line line;
  line.Common.MaterialOrigin = Origin::Record;
  line.Common.Material = nowhere;
  line.Common.FullStyle = nowhere;
  line.Common.FaceStyle = nowhere;
  line.Common.EdgeStyle = nowhere;
  const Handle drawn = *Edit::Add(drawing, line);
  Entities::Line bare; // Its own material named by no handle: nothing to check.
  bare.Common.MaterialOrigin = Origin::Record;
  static_cast<void>(Edit::Add(drawing, bare));
  Tables::Layer layer;
  layer.Record.Name = "Lit";
  layer.Material = nowhere;
  layer.Linetype = drawing.Header.LTypeContinuous;
  const Handle layered = *Edit::Add(drawing, layer);
  Tables::View view;
  view.Record.Name = "Seen";
  view.VisualStyle = nowhere;
  const Handle viewed = *Edit::Add(drawing, view);
  Entities::Viewport port;
  port.VisualStyle = nowhere;
  const Handle ported = *Edit::Add(drawing, port);
  Objects::Layout *layout = nullptr;
  for (Object &object : drawing.Objects)
  {
    layout = layout == nullptr ? GetIf<Objects::Layout>(&object) : layout;
  }
  REQUIRE(layout != nullptr);
  layout->VisualStyle = nowhere;
  const Handle laid = layout->Base.Self;
  const Audit::Report checked = Audit::Check(drawing);
  std::vector<std::string> whats;
  for (const Audit::Finding &finding : checked.Findings)
  {
    whats.push_back(finding.What);
  }
  for (const std::string &what : {std::format("material 999 of LINE {} missing", HexOf(drawn)),
                                  std::format("visual style 999 of LINE {} missing", HexOf(drawn)),
                                  std::format("material 999 of LAYER {} missing", HexOf(layered)),
                                  std::format("visual style 999 of VIEW {} missing", HexOf(viewed)),
                                  std::format("visual style 999 of VIEWPORT {} missing", HexOf(ported)),
                                  std::format("visual style 999 of LAYOUT {} missing", HexOf(laid))})
  {
    CHECK(std::ranges::contains(whats, what));
  }
  CHECK(std::ranges::count(whats, std::format("visual style 999 of LINE {} missing", HexOf(drawn))) == 3);
  CHECK(Audit::Repair(drawing).IsClean());
  const Common &common = *CommonOf(*Find(drawing, drawn));
  CHECK(common.Material == Handle{});
  CHECK(common.MaterialOrigin == Origin::ByLayer);
  CHECK(common.FullStyle == Handle{});
  CHECK(Get<Tables::Layer>(*Find(drawing, layered)).Material == Handle{});
  CHECK(Get<Tables::View>(*Find(drawing, viewed)).VisualStyle == Handle{});
  CHECK(Get<Entities::Viewport>(*Find(drawing, ported)).VisualStyle == Handle{});
  CHECK(Get<Objects::Layout>(*Find(drawing, laid)).VisualStyle == Handle{});
  CHECK(Audit::Check(drawing).Findings.empty());
}

TEST_CASE("releases/scene: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
