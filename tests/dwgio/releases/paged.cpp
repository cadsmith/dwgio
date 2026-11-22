/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/paged.cpp
 *  @brief     Entities across R2000 and R2004: chains to lists and back, and what R2000 cannot keep.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/object.hpp"

#include "dwgio/common.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/hatch.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <format>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
TEST_CASE("releases/paged: R2000 blocks and inserts and polylines upgrade to R2004 lists and go back to chains")
{
  Drawing drawing = Create();
  const Handle desk = *Edit::AddBlock(drawing, "Desk");
  Entities::Line one;
  Entities::Line two;
  const Handle first = *Edit::Add(drawing, one, "Desk");
  const Handle second = *Edit::Add(drawing, two, "Desk");
  const Tables::BlockRecord &record = *View::Block(drawing, "Desk");
  const Object upgraded = ConvertObject(Object{record}, drawing, Version::R2004).Object;
  CHECK(Get<Tables::BlockRecord>(upgraded).Entities == std::vector<Handle>{first, second});
  CHECK(Get<Tables::BlockRecord>(upgraded).First == Handle{});
  CHECK(Get<Tables::BlockRecord>(upgraded).Last == Handle{});
  const Object lineRisen = ConvertObject(*Find(drawing, second), drawing, Version::R2004).Object;
  CHECK(CommonOf(lineRisen)->Previous == Handle{});
  Drawing asNewer = drawing;
  asNewer.Release = Version::R2004;
  *Find(asNewer, desk) = upgraded;
  for (Object &object : asNewer.Objects)
  {
    if (Common *common = CommonOf(object))
    {
      common->Previous = {};
      common->Next = {};
    }
  }
  const Object downgraded = ConvertObject(*Find(asNewer, desk), asNewer, Version::R2000).Object;
  CHECK(Get<Tables::BlockRecord>(downgraded).First == first);
  CHECK(Get<Tables::BlockRecord>(downgraded).Last == second);
  CHECK(Get<Tables::BlockRecord>(downgraded).Entities.empty());
  // A line lowered alone cannot know its neighbours, so its links come from its owner's list in the drawing.
  const Object lineBack = ConvertObject(*Find(asNewer, second), asNewer, Version::R2000).Object;
  CHECK(CommonOf(lineBack)->Previous == first);
  CHECK(CommonOf(lineBack)->Next == Handle{});
  const Object firstBack = ConvertObject(*Find(asNewer, first), asNewer, Version::R2000).Object;
  CHECK(CommonOf(firstBack)->Previous == Handle{});
  CHECK(CommonOf(firstBack)->Next == second);
}

TEST_CASE("releases/paged: what R2000 cannot keep of an R2004 entity is dropped and noted")
{
  Drawing drawing = Create(Version::R2004);
  Entities::Line line;
  line.Common.Color.Rgb = 0xC2FF0000;
  line.Common.Color.Transparency = 0x02000040;
  const Handle drawn = *Edit::Add(drawing, line);
  const Translation lowered = ConvertObject(*Find(drawing, drawn), drawing, Version::R2000);
  CHECK_FALSE(CommonOf(lowered.Object)->Color.Rgb);
  CHECK_FALSE(CommonOf(lowered.Object)->Color.Transparency);
  CHECK(CommonOf(lowered.Object)->Color.Index == 256);
  REQUIRE(lowered.Issues.size() == 2);
  CHECK(lowered.Issues[0].What == std::format("true color of LINE {} not kept by R2000", HexOf(drawn)));
  CHECK(lowered.Issues[1].What == std::format("transparency of LINE {} not kept by R2000", HexOf(drawn)));
}

TEST_CASE("releases/paged: what R2000 cannot keep of the R2004 kinds is noted and links come only from a list")
{
  Drawing drawing = Create(Version::R2004);
  Entities::MText text;
  text.Background.Fill = 1;
  const Handle noted = *Edit::Add(drawing, text);
  Entities::Hatch hatch;
  hatch.Gradient.Shown = 1;
  const Handle filled = *Edit::Add(drawing, hatch);
  Entities::Viewport viewport;
  viewport.ShadePlotMode = 2;
  const Handle shaded = *Edit::Add(drawing, viewport, Space::Paper);
  const auto whats = [&drawing](Handle self) -> std::vector<std::string>
  {
    std::vector<std::string> out;
    for (const Issue &issue : ConvertObject(*Find(drawing, self), drawing, Version::R2000).Issues)
    {
      out.push_back(issue.What);
    }
    return out;
  };
  CHECK(whats(noted).empty()); // Kept in its extended data.
  CHECK(whats(filled) ==
        std::vector<std::string>{std::format("gradient of HATCH {} not kept by R2000", HexOf(filled))});
  CHECK(whats(shaded) ==
        std::vector<std::string>{std::format("shade plot of VIEWPORT {} not kept by R2000", HexOf(shaded))});
  Object *layout = Find(drawing, View::Layout(drawing, "Layout1")->Base.Self);
  auto &paper = Get<Objects::Layout>(*layout);
  paper.Plot.Shading = 1;
  paper.PlotView = Handle{.Value = 0x29};
  paper.Viewports = {shaded};
  CHECK(whats(paper.Base.Self) == std::vector<std::string>{"shade plot of layout Layout1 not kept by R2000",
                                                           "plot view of layout Layout1 not kept by R2000",
                                                           "viewports of layout Layout1 not kept by R2000"});
  // An entity its owner lists nowhere, or whose owner keeps no list, keeps null links.
  Entities::Line line;
  const Handle unlisted = *Edit::Add(drawing, line);
  Get<Tables::BlockRecord>(*Find(drawing, drawing.Header.BlockRecordModelSpace)).Entities.pop_back();
  CHECK(CommonOf(ConvertObject(*Find(drawing, unlisted), drawing, Version::R2000).Object)->Previous == Handle{});
  BaseOf(*Find(drawing, unlisted)).Owner = drawing.Header.Clayer;
  CHECK(CommonOf(ConvertObject(*Find(drawing, unlisted), drawing, Version::R2000).Object)->Next == Handle{});
  BaseOf(*Find(drawing, unlisted)).Owner = Handle{.Value = 0x999};
  CHECK(CommonOf(ConvertObject(*Find(drawing, unlisted), drawing, Version::R2000).Object)->Next == Handle{});
  // A color named, from a book or by reference is a true color R2000 has not; a shade resolution or DPI alone
  // is a shade plot.
  for (int which = 0; which < 3; ++which)
  {
    Entities::Line colored;
    if (which == 0)
    {
      colored.Common.Color.Names = ColorNames{.Name = "Pantone", .Book = {}};
    }
    else if (which == 1)
    {
      colored.Common.Color.Names = ColorNames{.Name = {}, .Book = "Book"};
    }
    else
    {
      colored.Common.Color.Reference = Handle{.Value = 0x10};
    }
    const Handle tinted = *Edit::Add(drawing, colored);
    CHECK(whats(tinted) ==
          std::vector<std::string>{std::format("true color of LINE {} not kept by R2000", HexOf(tinted))});
  }
  auto &model = Get<Objects::Layout>(*Find(drawing, View::Layout(drawing, "Model")->Base.Self));
  model.Plot.Resolution = 5;
  CHECK(whats(model.Base.Self) == std::vector<std::string>{"shade plot of layout Model not kept by R2000"});
  model.Plot.Resolution = 2;
  model.Plot.Dots = 150;
  CHECK(whats(model.Base.Self) == std::vector<std::string>{"shade plot of layout Model not kept by R2000"});
}

TEST_CASE("releases/paged: a chain that reaches another block's entity or loops upgrades to what it walked")
{
  Drawing drawing = Create();
  REQUIRE(Edit::AddBlock(drawing, "Desk"));
  const Handle first = *Edit::Add(drawing, Entities::Line{}, "Desk");
  const Handle second = *Edit::Add(drawing, Entities::Line{}, "Desk");
  const Handle third = *Edit::Add(drawing, Entities::Line{}, "Desk");
  const Handle elsewhere = *Edit::Add(drawing, Entities::Line{});
  const Handle desk = View::Block(drawing, "Desk")->Record.Base.Self;
  CommonOf(*Find(drawing, second))->Next = elsewhere;
  CHECK(Get<Tables::BlockRecord>(ConvertObject(*Find(drawing, desk), drawing, Version::R2004).Object).Entities ==
        std::vector<Handle>{first, second});
  CommonOf(*Find(drawing, second))->Next = third;
  CommonOf(*Find(drawing, third))->Next = first;
  Get<Tables::BlockRecord>(*Find(drawing, desk)).Last = Handle{.Value = 0x999};
  CHECK(Get<Tables::BlockRecord>(ConvertObject(*Find(drawing, desk), drawing, Version::R2004).Object).Entities ==
        std::vector<Handle>{first, second, third});
}
} // namespace DwgIO::Releases
