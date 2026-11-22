/***********************************************************************************************************************
 *  @file      tests/dwgio/save/sections.cpp
 *  @brief     The sections and blocks a save writes, their order, and those it remakes or leaves out.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "file/coded/layout.hpp"
#include "file/locator/layout.hpp"
#include "file/paged/essential.hpp"
#include "file/paged/layout.hpp"
#include "file/sections/information.hpp"
#include "file/sections/summary.hpp"
#include "objects/map.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/class.hpp"
#include "dwgio/create.hpp"
#include "dwgio/dependencies.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/section.hpp"
#include "dwgio/symbol.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/view.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  Renames the drawing's first untyped object to a class AutoCAD's files never hold, which no save to another release
 *  carries; returns its handle.
 */
auto Unseen(Drawing &drawing) -> Handle
{
  const auto found =
      std::ranges::find_if(drawing.Objects, [](const Object &object) -> bool { return Holds<Unknown>(object); });
  REQUIRE(found != drawing.Objects.end());
  Get<Unknown>(*found).Class = Symbol{"UNSEEN"};
  return BaseOf(*found).Self;
}
} // namespace

TEST_CASE("save/sections: blocks are found by name and an unknown name gives none and no preview")
{
  Drawing drawing = LoadDrawing("foundation-r2000.dwg");
  drawing.Kept.push_back(Section{.Name = "unknown", .Bytes = {std::byte{1}, std::byte{2}}});
  drawing.Kept.push_back(Section{.Name = "unknown", .Bytes = {std::byte{3}}});
  drawing.Order.push_back("unknown");
  drawing.Order.push_back("unknown");
  drawing.Order.push_back("bogus");
  drawing.Preview.clear();
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Drawing.Preview.empty());
  CHECK(std::ranges::count(loaded->Drawing.Order, "unknown") == 1);
  CHECK(loaded->Drawing.Kept.back().Bytes == std::vector{std::byte{1}, std::byte{2}, std::byte{3}});
  CHECK(loaded->Drawing.Objects == drawing.Objects);
}

TEST_CASE("save/sections: an order naming a missing auxiliary header writes five records")
{
  Drawing drawing = LoadDrawing("foundation-r2000.dwg");
  drawing.Kept.clear();
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(std::to_integer<int>(saved->Bytes[0x15]) == 5);
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Drawing.Objects == drawing.Objects);
}

TEST_CASE("save/sections: a paged or coded file's map before the objects stays there")
{
  for (const char *name : {"foundation-r2007.dwg", "foundation-r2018.dwg"})
  {
    CAPTURE(name);
    Drawing drawing = LoadDrawing(name);
    std::vector<std::string> &order = drawing.Order;
    const auto map = std::ranges::find(order, "map");
    REQUIRE(map != order.end());
    order.erase(map);
    order.insert(std::ranges::find(order, "objects"), "map");
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    CHECK(loaded->Drawing.Order == drawing.Order);
    CHECK(loaded->Drawing.Objects == drawing.Objects);
  }
}

TEST_CASE("save/sections: a map before the objects or a second header before the map moves after what it needs")
{
  Drawing drawing = LoadDrawing("foundation-r2000.dwg");
  drawing.Order = {"auxiliary", "preview", "header", "classes", "pad", "second", "map", "objects", "measurement"};
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  CHECK(std::ranges::any_of(saved->Issues,
                            [](const Issue &one) { return one.What == "map block moved after the objects"; }));
  CHECK(std::ranges::any_of(saved->Issues,
                            [](const Issue &one) { return one.What == "second block moved after the map"; }));
  const std::expected<Loaded, Failure> loaded = Load(saved->Bytes);
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  CHECK(saved->Issues.size() == 2);
  CHECK(saved->Bytes == ReadFixture("foundation-r2000.dwg"));
  drawing.Order = {"header", "classes", "objects", "map"};
  const std::expected<Saved, Failure> bare = Save(drawing);
  REQUIRE(bare);
  CHECK(bare->Issues.empty());
}

TEST_CASE("save/sections: the sections AutoCAD requires are made when a drawing lacks them and the template too")
{
  // AutoCAD refuses an R2004 file without a revision history, an object free space or an auxiliary header, and
  // opens one without a summary or application information; a drawing from an R2000 file or from Create has none.
  Drawing drawing = Load(ReadFixture("empty-r2004.dwg")).value().Drawing;
  drawing.SummaryInfo.reset();
  std::erase_if(drawing.Kept,
                [](const Section &block)
                {
                  return block.Name == "AcDb:RevHistory" || block.Name == "AcDb:ObjFreeSpace" ||
                         block.Name == "auxiliary" || block.Name == "measurement";
                });
  std::erase_if(drawing.Order,
                [](const std::string &name)
                {
                  return name == "AcDb:RevHistory" || name == "AcDb:ObjFreeSpace" || name == "auxiliary" ||
                         name == "AcDb:SummaryInfo";
                });
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  std::vector<Issue> issues;
  const File::Paged::Layout ours = *File::Paged::ReadLayout(saved->Bytes, issues);
  const auto named = [&ours](std::string_view name) -> const File::Paged::Section *
  {
    const auto found = std::ranges::find(ours.Sections, name, &File::Paged::Section::Name);
    return found == ours.Sections.end() ? nullptr : &*found;
  };
  CHECK(named("AcDb:SummaryInfo") == nullptr);
  REQUIRE(named("AcDb:RevHistory") != nullptr);
  CHECK(named("AcDb:RevHistory")->Bytes == File::Paged::WriteRevHistory());
  REQUIRE(named("AcDb:ObjFreeSpace") != nullptr);
  CHECK(named("AcDb:ObjFreeSpace")->Bytes == File::Paged::WriteObjFreeSpace(drawing.Header, Version::R2004, 0));
  REQUIRE(named("auxiliary") != nullptr);
  CHECK(named("auxiliary")->Bytes.size() == 123);
  const auto measurement = std::ranges::find(ours.Sections, "measurement", &File::Paged::Section::Name);
  REQUIRE(measurement != ours.Sections.end());
  CHECK(measurement->Bytes == std::vector<std::byte>{std::byte{0}, std::byte{0}, std::byte{1}, std::byte{0}});
  QueueAudit(saved->Bytes, "dwgio-empty-r2004-required.dwg");
  // A section under a name no table knows is written and read back under it, with no id.
  Drawing strange = Load(ReadFixture("empty-r2004.dwg")).value().Drawing;
  strange.Kept.push_back(Section{.Name = "AcDb:Future", .Bytes = std::vector<std::byte>(10, std::byte{7})});
  strange.Order.insert(strange.Order.begin() + 1, "AcDb:Future");
  const std::expected<Saved, Failure> odd = Save(strange);
  REQUIRE(odd);
  const Drawing reread = Load(odd->Bytes).value().Drawing;
  const auto future = std::ranges::find(reread.Kept, "AcDb:Future", &Section::Name);
  REQUIRE(future != reread.Kept.end());
  CHECK(future->Bytes == std::vector<std::byte>(10, std::byte{7}));
  // Bytes no block of an R2000 file claimed have no place in a paged file; a measurement the file lacks is made.
  Drawing claimed = Load(ReadFixture("empty-r2000.dwg")).value().Drawing;
  claimed.Kept.push_back(Section{.Name = "unknown", .Bytes = std::vector<std::byte>(3, std::byte{1})});
  claimed.Order.push_back("unknown");
  std::erase_if(claimed.Kept, [](const Section &block) { return block.Name == "measurement"; });
  std::erase(claimed.Order, std::string{"measurement"});
  const std::expected<Saved, Failure> placed = Save(claimed, SaveOptions{.Release = Version::R2004});
  REQUIRE(placed);
  CHECK(std::ranges::any_of(placed->Issues,
                            [](const Issue &one) { return one.What.contains("unknown block left out"); }));
  const File::Paged::Layout again = *File::Paged::ReadLayout(placed->Bytes, issues);
  CHECK(std::ranges::find(again.Sections, "measurement", &File::Paged::Section::Name) != again.Sections.end());
  // An R2000 drawing's measurement block and a paged file's template are the same four bytes, carried across.
  const Drawing older = Load(ReadFixture("empty-r2000.dwg")).value().Drawing;
  const std::expected<Saved, Failure> raised = Save(older, SaveOptions{.Release = Version::R2004});
  REQUIRE(raised);
  const File::Paged::Layout paged = *File::Paged::ReadLayout(raised->Bytes, issues);
  const auto template_ = std::ranges::find(paged.Sections, "measurement", &File::Paged::Section::Name);
  REQUIRE(template_ != paged.Sections.end());
  CHECK(template_->Bytes == std::vector<std::byte>{std::byte{0}, std::byte{0}, std::byte{1}, std::byte{0}});
  const std::expected<Loaded, Failure> back = Load(raised->Bytes);
  REQUIRE(back);
  CHECK(back->Drawing.Release == Version::R2004);
  const std::expected<Saved, Failure> lowered = Save(back->Drawing, SaveOptions{.Release = Version::R2000});
  REQUIRE(lowered);
  const Drawing round = Load(lowered->Bytes).value().Drawing;
  const auto kept = std::ranges::find(round.Kept, "measurement", &Section::Name);
  REQUIRE(kept != round.Kept.end());
  CHECK(kept->Bytes == std::vector<std::byte>{std::byte{0}, std::byte{0}, std::byte{1}, std::byte{0}});
}

TEST_CASE("save/sections: an order missing a block every file has gives way to AutoCAD's usual order")
{
  Drawing made = Create(Version::R2004);
  made.Order = {"header"};
  REQUIRE(Save(made));
  CHECK(Load(Save(made)->Bytes).value().Drawing.Objects.size() == made.Objects.size());
  made.Order = {"AcDb:SummaryInfo"};
  REQUIRE(Save(made));
  Drawing older = Create();
  older.Order = {"header"};
  REQUIRE(Save(older));
  CHECK(Load(Save(older)->Bytes).value().Drawing.Objects.size() == older.Objects.size());
}

TEST_CASE("save/sections: saving to another release remakes the fixed-shape sections and the summary")
{
  const Drawing drawing = Load(ReadFixture("empty-r2018.dwg")).value().Drawing;
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2010});
  REQUIRE(saved);
  std::vector<Issue> issues;
  const File::Paged::Layout ours = *File::Paged::ReadLayout(saved->Bytes, issues);
  const auto named = [&ours](std::string_view name) -> const File::Paged::Section *
  {
    const auto found = std::ranges::find(ours.Sections, name, &File::Paged::Section::Name);
    return found == ours.Sections.end() ? nullptr : &*found;
  };
  REQUIRE(named("AcDb:SummaryInfo") != nullptr);
  REQUIRE(drawing.SummaryInfo);
  CHECK(named("AcDb:SummaryInfo")->Bytes ==
        File::Sections::WriteSummary(*drawing.SummaryInfo, Version::R2010, Encoding::MakeScheme(Encoding::Standard())));
  // The application information names dwgio; its history crosses as it was.
  REQUIRE(named("AcDb:AppInfo") != nullptr);
  CHECK(named("AcDb:AppInfo")->Bytes == File::Sections::WriteInformation());
  REQUIRE(named("AcDb:AppInfoHistory") != nullptr);
  CHECK(named("AcDb:AppInfoHistory")->Bytes ==
        std::ranges::find(drawing.Kept, "AcDb:AppInfoHistory", &Section::Name)->Bytes);
  CHECK(named("AcDb:AcDsPrototype_1b") == nullptr);
  REQUIRE(named("auxiliary") != nullptr);
  // The header of the drawing written, whose handle seed is past the records its visual styles keep in R2010.
  const Header written = Load(saved->Bytes).value().Drawing.Header;
  CHECK(named("auxiliary")->Bytes == File::Paged::WriteAuxHeader(written, Version::R2010, 226));
  REQUIRE(named("measurement") != nullptr);
  CHECK(named("measurement")->Bytes == File::Paged::WriteTemplate(Version::R2010));
  CHECK(ours.Head.Maintenance == 226);
}

TEST_CASE("save/sections: the data store converts between R2013 and R2018 and without it no object claims data")
{
  const auto stored = [](const Drawing &drawing) -> std::vector<Handle>
  {
    std::vector<Handle> handles;
    for (const Object &object : drawing.Objects)
    {
      if (BaseOf(object).Stored)
      {
        handles.push_back(BaseOf(object).Self);
      }
    }
    return handles;
  };
  const auto store = [](const Drawing &drawing) -> const Section *
  {
    const auto found = std::ranges::find(drawing.Kept, "AcDb:AcDsPrototype_1b", &Section::Name);
    return found == drawing.Kept.end() ? nullptr : &*found;
  };
  for (const auto &[from, to] :
       {std::pair{"tables-r2013.dwg", Version::R2018}, std::pair{"tables-r2018.dwg", Version::R2013}})
  {
    CAPTURE(from);
    const Drawing drawing = Load(ReadFixture(from)).value().Drawing;
    REQUIRE(stored(drawing).size() == 2);
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = to});
    REQUIRE(saved);
    CHECK(std::ranges::none_of(saved->Issues,
                               [](const Issue &issue) -> bool { return issue.What.starts_with("data store"); }));
    const Drawing back = Load(saved->Bytes).value().Drawing;
    CHECK(stored(back) == stored(drawing));
    REQUIRE(store(back) != nullptr);
    CHECK(store(back)->Bytes == store(drawing)->Bytes);
    QueueAudit(saved->Bytes, std::format("dwgio-{}-as-{}.dwg", from, NameOf(to)));
  }
  // Saved as R2010, which has no data store, the data is lost and said.
  const Drawing newer = Load(ReadFixture("tables-r2018.dwg")).value().Drawing;
  const std::expected<Saved, Failure> lowered = Save(newer, SaveOptions{.Release = Version::R2010});
  REQUIRE(lowered);
  CHECK(std::ranges::count(lowered->Issues, std::string{"data store left out, the data of 2 objects in it lost"},
                           &Issue::What) == 1);
  // A drawing of its own release whose data store is gone, or whose order leaves it out, clears the bit.
  Drawing bare = newer;
  std::erase_if(bare.Kept, [](const Section &block) -> bool { return block.Name == "AcDb:AcDsPrototype_1b"; });
  const Handle kept = stored(newer).front();
  for (Object &object : bare.Objects)
  {
    BaseOf(object).Stored = BaseOf(object).Self == kept;
  }
  const std::expected<Saved, Failure> unstored = Save(bare);
  REQUIRE(unstored);
  CHECK(std::ranges::count(unstored->Issues, std::string{"data store left out, the data of 1 object in it lost"},
                           &Issue::What) == 1);
  CHECK(stored(Load(unstored->Bytes).value().Drawing).empty());
  Drawing unordered = newer;
  std::erase(unordered.Order, "AcDb:AcDsPrototype_1b");
  const std::expected<Saved, Failure> unplaced = Save(unordered);
  REQUIRE(unplaced);
  CHECK(stored(Load(unplaced->Bytes).value().Drawing).empty());
  QueueAudit(unplaced->Bytes, "dwgio-tables-r2018-without-store.dwg");
}

TEST_CASE("save/sections: a kept template and auxiliary header convert unchanged only between alike shapes")
{
  const auto kept = [](const Drawing &drawing, std::string_view name) -> std::vector<std::byte>
  {
    const auto found = std::ranges::find(drawing.Kept, name, &Section::Name);
    return found == drawing.Kept.end() ? std::vector<std::byte>{} : found->Bytes;
  };

  struct Crossing
  {
    std::string_view From;
    Version To;
    std::size_t Auxiliary;
  };

  for (const Crossing crossing : {Crossing{.From = "curves-r2018.dwg", .To = Version::R2000, .Auxiliary = 123},
                                  Crossing{.From = "curves-r2000.dwg", .To = Version::R2018, .Auxiliary = 129},
                                  Crossing{.From = "curves-r2010.dwg", .To = Version::R14, .Auxiliary = 0}})
  {
    CAPTURE(crossing.From);
    const Drawing drawing = Load(ReadFixture(crossing.From)).value().Drawing;
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = crossing.To});
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(kept(back->Drawing, "measurement") == File::Paged::WriteTemplate(crossing.To));
    CHECK(kept(back->Drawing, "auxiliary").size() == crossing.Auxiliary);
    if (crossing.To >= Version::R2000)
    {
      CHECK(kept(back->Drawing, "auxiliary") ==
            File::Paged::WriteAuxHeader(back->Drawing.Header, crossing.To, back->Drawing.Maintenance));
    }
    QueueAudit(saved->Bytes, std::format("dwgio-shaped-{}-as-{}.dwg", crossing.From, NameOf(crossing.To)));
  }
  // Of one shape in both, they are written as they were.
  const Drawing newer = Load(ReadFixture("curves-r2004.dwg")).value().Drawing;
  const Drawing lowered = Load(Save(newer, SaveOptions{.Release = Version::R2000})->Bytes).value().Drawing;
  CHECK(kept(lowered, "auxiliary") == kept(newer, "auxiliary"));
  CHECK(kept(lowered, "measurement") == kept(newer, "measurement"));
}

TEST_CASE("save/sections: an empty template is remade because AutoCAD refuses an empty template section")
{
  // AutoCAD's own R2004 sample drawings describe a template of four bytes and no pages, which reads as none.
  for (const Version release : {Version::R2004, Version::R2000})
  {
    CAPTURE(NameOf(release));
    Drawing drawing = Load(ReadFixture("curves-r2004.dwg")).value().Drawing;
    std::ranges::find(drawing.Kept, "measurement", &Section::Name)->Bytes.clear();
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = release});
    REQUIRE(saved);
    const Drawing back = Load(saved->Bytes).value().Drawing;
    CHECK(std::ranges::find(back.Kept, "measurement", &Section::Name)->Bytes == File::Paged::WriteTemplate(release));
    QueueAudit(saved->Bytes, std::format("dwgio-untemplated-as-{}.dwg", NameOf(release)));
  }
}

TEST_CASE("save/sections: the free space made for another release counts only the objects written")
{
  Drawing drawing = Load(ReadFixture("objects-r2018.dwg")).value().Drawing;
  Unseen(drawing); // Left out of a save to another release.
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2013});
  REQUIRE(saved);
  const std::size_t written = Load(saved->Bytes).value().Drawing.Objects.size();
  // One left out, and the record of the other view style made in a dictionary of its own.
  REQUIRE(written == drawing.Objects.size() + 1);
  std::vector<Issue> issues;
  const File::Paged::Layout layout = *File::Paged::ReadLayout(saved->Bytes, issues);
  const auto free = std::ranges::find(layout.Sections, "AcDb:ObjFreeSpace", &File::Paged::Section::Name);
  REQUIRE(free != layout.Sections.end());
  CHECK(free->Bytes ==
        File::Paged::WriteObjFreeSpace(Load(saved->Bytes).value().Drawing.Header, Version::R2013, written));
}

TEST_CASE("save/sections: a section another release cannot carry such as macros or dependencies is noted")
{
  Drawing drawing = Load(ReadFixture("curves-r2018.dwg")).value().Drawing;
  for (const std::string name : {"AcDb:VBAProject", "AcDb:FileDepList"})
  {
    drawing.Kept.push_back(Section{.Name = name, .Bytes = std::vector<std::byte>(16, std::byte{1})});
    drawing.Order.insert(drawing.Order.end() - 1, name);
  }
  const auto sections = [](const std::vector<Issue> &issues) -> std::vector<std::string>
  {
    std::vector<std::string> said;
    for (const Issue &issue : issues)
    {
      if (issue.What.starts_with("section "))
      {
        said.push_back(issue.What);
      }
    }
    return said;
  };
  const std::expected<Saved, Failure> same = Save(drawing);
  REQUIRE(same);
  CHECK(sections(same->Issues).empty());
  const Drawing again = Load(same->Bytes).value().Drawing;
  CHECK(std::ranges::contains(again.Kept, "AcDb:VBAProject", &Section::Name));
  const std::expected<Saved, Failure> paged = Save(drawing, SaveOptions{.Release = Version::R2013});
  REQUIRE(paged);
  CHECK(sections(paged->Issues) ==
        std::vector<std::string>{"section AcDb:VBAProject left out, its R2018 bytes cannot be written as R2013",
                                 "section AcDb:FileDepList left out, its R2018 bytes cannot be written as R2013"});
  const std::expected<Saved, Failure> located = Save(drawing, SaveOptions{.Release = Version::R2000});
  REQUIRE(located);
  CHECK(sections(located->Issues) ==
        std::vector<std::string>{"section AcDb:VBAProject left out, a locator file has no place for it",
                                 "section AcDb:FileDepList left out, a locator file has no place for it"});
  // A name in the order with no section kept under it has nothing to lose.
  drawing.Kept.pop_back();
  CHECK(sections(Save(drawing, SaveOptions{.Release = Version::R2000}).value().Issues).size() == 1);
}

TEST_CASE("save/sections: the file dependencies convert to every release from R2004 in its form and none before")
{
  Drawing drawing = Load(ReadFixture("curves-r2018.dwg")).value().Drawing;
  drawing.FileDependencies = FileDependencies{.Features = {"Acad:Text"},
                                              .Files = {FileDependency{.FullFilename = "caf\xc3\xa9.shx",
                                                                       .FoundPath = "C:\\Fonts\\",
                                                                       .FingerprintGuid = "",
                                                                       .VersionGuid = "",
                                                                       .FeatureIndex = 0,
                                                                       .Timestamp = 1,
                                                                       .Filesize = 2,
                                                                       .AffectsGraphics = true,
                                                                       .ReferenceCount = 1}}};
  for (const Version release : {Version::R2004, Version::R2007, Version::R2010, Version::R2013, Version::R2018})
  {
    CAPTURE(NameOf(release));
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = release});
    REQUIRE(saved);
    const Drawing back = Load(saved->Bytes).value().Drawing;
    CHECK(back.FileDependencies == drawing.FileDependencies);
    CHECK_FALSE(std::ranges::contains(back.Kept, "AcDb:FileDepList", &Section::Name));
    QueueAudit(saved->Bytes, std::format("dwgio-dependencies-as-{}.dwg", NameOf(release)));
  }
  for (const Version release : {Version::R2000, Version::R14})
  {
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = release});
    REQUIRE(saved);
    CHECK(std::ranges::contains(
        saved->Issues, std::string{"file dependencies left out, a locator file has no place for them"}, &Issue::What));
    CHECK_FALSE(Load(saved->Bytes).value().Drawing.FileDependencies);
  }
}

TEST_CASE("save/sections: a summary that would not write back unchanged stays as bytes in its own release only")
{
  Drawing drawing = Load(ReadFixture("curves-r2018.dwg")).value().Drawing;
  drawing.SummaryInfo.reset();
  drawing.Kept.push_back(Section{.Name = "AcDb:SummaryInfo", .Bytes = std::vector<std::byte>(16, std::byte{1})});
  const std::expected<Saved, Failure> same = Save(drawing);
  REQUIRE(same);
  const Drawing back = Load(same->Bytes).value().Drawing;
  CHECK_FALSE(back.SummaryInfo);
  CHECK(std::ranges::find(back.Kept, "AcDb:SummaryInfo", &Section::Name)->Bytes ==
        std::vector<std::byte>(16, std::byte{1}));
  const std::expected<Saved, Failure> other = Save(drawing, SaveOptions{.Release = Version::R2013});
  REQUIRE(other);
  CHECK(std::ranges::contains(
      other->Issues, std::string{"section AcDb:SummaryInfo left out, its R2018 bytes cannot be written as R2013"},
      &Issue::What));
}

TEST_CASE("save/sections: a signature is left out of every save")
{
  Drawing drawing = Load(ReadFixture("curves-r2018.dwg")).value().Drawing;
  drawing.Kept.push_back(Section{.Name = "AcDb:Signature", .Bytes = std::vector<std::byte>(16, std::byte{1})});
  drawing.Order.insert(drawing.Order.begin(), "AcDb:Signature");
  for (const Version release : {Version::R2018, Version::R2004})
  {
    CAPTURE(NameOf(release));
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = release});
    REQUIRE(saved);
    CHECK(std::ranges::contains(saved->Issues,
                                std::string{"section AcDb:Signature left out, it signs bytes a save does not keep"},
                                &Issue::What));
    CHECK_FALSE(std::ranges::contains(Load(saved->Bytes).value().Drawing.Kept, "AcDb:Signature", &Section::Name));
  }
  // A name in the order with no section under it says nothing.
  drawing.Kept.pop_back();
  CHECK(std::ranges::none_of(Save(drawing).value().Issues,
                             [](const Issue &issue) -> bool { return issue.What.contains("Signature"); }));
}

TEST_CASE("save/sections: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
