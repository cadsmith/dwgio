/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/plotsettings.cpp
 *  @brief     The PLOTSETTINGS against its DXF twin, written in every release, and what older releases drop.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/plotsettings.hpp"

#include "classes/catalog.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "releases/object.hpp"
#include "releases/omission.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"

#include "dwgio/class.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
auto Number(const std::vector<Pair> &twin, std::size_t opened, int code) -> double
{
  const std::string value = GroupOf(twin, opened, code);
  return value.empty() ? 0.0 : std::stod(value);
}

/**
 *  The page setup's plot against the twin's AcDbPlotSettings groups; the shade plot groups R2000's twin leaves out are
 *  a new layout's.
 */
void CheckPlot(const std::vector<Pair> &twin, std::size_t at, const Plot &plot)
{
  CHECK(plot.Name == GroupOf(twin, at, 1));
  CHECK(plot.Device == GroupOf(twin, at, 2));
  CHECK(plot.Size == GroupOf(twin, at, 4));
  CHECK(plot.View == GroupOf(twin, at, 6));
  CHECK(plot.Style == GroupOf(twin, at, 7));
  CHECK(plot.Left == doctest::Approx(Number(twin, at, 40)));
  CHECK(plot.Bottom == doctest::Approx(Number(twin, at, 41)));
  CHECK(plot.Right == doctest::Approx(Number(twin, at, 42)));
  CHECK(plot.Top == doctest::Approx(Number(twin, at, 43)));
  CHECK(plot.Width == doctest::Approx(Number(twin, at, 44)));
  CHECK(plot.Height == doctest::Approx(Number(twin, at, 45)));
  CHECK(plot.Origin.X == doctest::Approx(Number(twin, at, 46)));
  CHECK(plot.Origin.Y == doctest::Approx(Number(twin, at, 47)));
  CHECK(plot.Lower.X == doctest::Approx(Number(twin, at, 48)));
  CHECK(plot.Lower.Y == doctest::Approx(Number(twin, at, 49)));
  CHECK(plot.Upper.X == doctest::Approx(Number(twin, at, 140)));
  CHECK(plot.Upper.Y == doctest::Approx(Number(twin, at, 141)));
  CHECK(plot.PaperUnits == doctest::Approx(Number(twin, at, 142)));
  CHECK(plot.DrawingUnits == doctest::Approx(Number(twin, at, 143)));
  CHECK(plot.Flags == ReadWhole(twin, at, 70));
  CHECK(plot.Units == ReadWhole(twin, at, 72));
  CHECK(plot.Rotation == ReadWhole(twin, at, 73));
  CHECK(plot.Type == ReadWhole(twin, at, 74));
  CHECK(plot.Standard == ReadWhole(twin, at, 75));
  CHECK(plot.Factor == doctest::Approx(Number(twin, at, 147)));
  CHECK(plot.Image.X == doctest::Approx(Number(twin, at, 148)));
  CHECK(plot.Image.Y == doctest::Approx(Number(twin, at, 149)));
  const bool shaded = !GroupOf(twin, at, 77).empty();
  CHECK(plot.Shading == ReadWhole(twin, at, 76));
  CHECK(plot.Resolution == (shaded ? ReadWhole(twin, at, 77) : 2));
  CHECK(plot.Dots == (shaded ? ReadWhole(twin, at, 78) : 300));
}
} // namespace

TEST_CASE("objects/plotsettings: every release's page setup matches the DXF twin")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    const std::vector<Pair> twin = ReadTwin(std::format("dimensions-{}.dxf", release == "r14" ? "r2000" : release));
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("dimensions-{}.dwg", release)));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    std::size_t count = 0;
    for (const Object &object : drawing.Objects)
    {
      const auto *settings = GetIf<PlotSettings>(&object);
      if (settings == nullptr)
      {
        continue;
      }
      ++count;
      CHECK(TypeOf(object) == "PLOTSETTINGS");
      const std::optional<std::size_t> opened = FindHandled(twin, settings->Base.Self);
      REQUIRE(opened);
      CHECK(twin[*opened].Value == "PLOTSETTINGS");
      CheckPlot(twin, *opened, settings->Plot);
      CHECK(settings->PlotView == Handle{});
      CHECK(settings->VisualStyle == Handle{});
      const auto *setups = GetIf<Dictionary>(FindObject(drawing, settings->Base.Owner));
      REQUIRE(setups);
      const Handle *entry = EntryNamed(*setups, "Check", drawing);
      REQUIRE(entry);
      CHECK(*entry == settings->Base.Self);
    }
    CHECK(count == 1);
  }
}

TEST_CASE("objects/plotsettings: a page setup with a plot view and a visual style round-trips in every release")
{
  PlotSettings settings;
  settings.Base.Self = Handle{0x100};
  settings.Base.Owner = Handle{0x19};
  settings.Plot.Name = "Check";
  settings.Plot.Device = "none_device";
  settings.Plot.View = "Front";
  settings.Plot.Type = 3;
  settings.Plot.Shading = 2;
  settings.Plot.Resolution = 5;
  settings.Plot.Dots = 600;
  settings.PlotView = Handle{0x60};
  settings.VisualStyle = Handle{0x61};
  const std::vector<Class> classes{*Classes::FindDeclared("PLOTSETTINGS")};
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2007, Version::R2018})
  {
    CAPTURE(NameOf(release));
    std::vector<Issue> issues;
    const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = classes, .Issues = &issues};
    const std::optional<Object> back = ReadObject(WriteObject(Object{settings}, with), 0, reading);
    REQUIRE(back);
    CHECK(TypeOf(*back) == "PLOTSETTINGS");
    const auto &read = Get<PlotSettings>(*back);
    // The view goes by name up to R2000 and by handle from R2004, the shade plot from R2004, the style from R2007.
    CHECK(read.Plot.View == (release <= Version::R2000 ? "Front" : ""));
    CHECK(read.PlotView == (release >= Version::R2004 ? settings.PlotView : Handle{}));
    CHECK(read.Plot.Dots == (release >= Version::R2004 ? 600 : 300));
    CHECK(read.VisualStyle == (release >= Version::R2007 ? settings.VisualStyle : Handle{}));
    CHECK(issues.empty());
  }
}

TEST_CASE("objects/plotsettings: an older release notes and drops what it cannot keep of a page setup")
{
  Drawing drawing = Load(ReadFixture("dimensions-r2018.dwg")).value().Drawing;
  auto &settings = Get<PlotSettings>(*Find(drawing, Handle{0x303}));
  const auto whats = [&drawing](Version to) -> std::vector<std::string>
  {
    std::vector<std::string> out;
    for (const Issue &issue : Releases::ConvertObject(*Find(drawing, Handle{0x303}), drawing, to).Issues)
    {
      out.push_back(issue.What);
    }
    return out;
  };
  CHECK(whats(Version::R2000).empty());
  settings.Plot.Dots = 600;
  settings.PlotView = Handle{0x9999}; // No object, let alone a VIEW: the dimensions fixture has none.
  settings.VisualStyle = Handle{0xF5};
  CHECK(whats(Version::R2010).empty());
  CHECK(whats(Version::R2004) == std::vector<std::string>{"visual style of PLOTSETTINGS 303 not kept by R2004"});
  const Object downgraded = Releases::ConvertObject(*Find(drawing, Handle{0x303}), drawing, Version::R2000).Object;
  CHECK(Get<PlotSettings>(downgraded).Plot.Dots == 300);
  CHECK(Get<PlotSettings>(downgraded).Plot.View.empty());
  CHECK(Get<PlotSettings>(downgraded).PlotView == Handle{});
  CHECK(Get<PlotSettings>(downgraded).VisualStyle == Handle{});
  CHECK(whats(Version::R2000) == std::vector<std::string>{"plot view 9999 of PLOTSETTINGS 303 not in its table",
                                                          "shade plot of PLOTSETTINGS 303 not kept by R2000",
                                                          "visual style of PLOTSETTINGS 303 not kept by R2004"});
  // The visual style F5 is of a kind the library leaves out of a save to another release: the pointer is cleared.
  Object kept{settings};
  std::vector<Issue> issues;
  const std::array<Handle, 1> leftOut{Handle{0xF5}};
  Releases::ClearPointers(kept, leftOut, issues);
  CHECK(Get<PlotSettings>(kept).VisualStyle == Handle{});
  REQUIRE(issues.size() == 1);
  CHECK(issues[0].What == "visual style of PLOTSETTINGS 303 cleared, F5 left out");
  // A plot view naming an object that is no VIEW is no view R2000 can name.
  settings.PlotView = settings.Base.Self;
  CHECK(whats(Version::R2000).front() == "plot view 303 of PLOTSETTINGS 303 not in its table");
}

TEST_CASE("objects/plotsettings: a page setup's plot view links its VIEW record from R2004 and its name below")
{
  // The tables fixture has a view; its page setup is made here, the fixture having none.
  for (const std::string_view release : {"r2000", "r2004"})
  {
    CAPTURE(release);
    const Drawing drawing = Load(ReadFixture(std::format("tables-{}.dwg", release))).value().Drawing;
    const std::vector<const Tables::View *> views = View::Records<Tables::View>(drawing);
    REQUIRE(views.size() == 1);
    const Tables::View &view = *views.front();
    PlotSettings settings;
    settings.Base.Self = Handle{0x999};
    settings.Plot.Type = 3;
    if (drawing.Release == Version::R2000)
    {
      settings.Plot.View = view.Record.Name;
      const Releases::Translation upgraded = Releases::ConvertObject(Object{settings}, drawing, Version::R2004);
      CHECK(upgraded.Issues.empty());
      CHECK(Get<PlotSettings>(upgraded.Object).PlotView == view.Record.Base.Self);
      CHECK(Get<PlotSettings>(upgraded.Object).Plot.View.empty());
      settings.Plot.View = "Nowhere";
      const Releases::Translation lost = Releases::ConvertObject(Object{settings}, drawing, Version::R2018);
      REQUIRE(lost.Issues.size() == 1);
      CHECK(lost.Issues[0].What == "plot view Nowhere of PLOTSETTINGS 999 not in its table");
      CHECK(Get<PlotSettings>(lost.Object).PlotView == Handle{});
    }
    else
    {
      settings.PlotView = view.Record.Base.Self;
      const Releases::Translation downgraded = Releases::ConvertObject(Object{settings}, drawing, Version::R14);
      CHECK(downgraded.Issues.empty());
      CHECK(Get<PlotSettings>(downgraded.Object).Plot.View == view.Record.Name);
      CHECK(Get<PlotSettings>(downgraded.Object).PlotView == Handle{});
    }
  }
}
} // namespace DwgIO::Objects
