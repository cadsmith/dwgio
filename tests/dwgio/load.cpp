/***********************************************************************************************************************
 *  @file      tests/dwgio/load.cpp
 *  @brief     Loading every release's fixtures whole, and failing on what is no drawing.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/load.hpp"

#include "file/paged/layout.hpp"
#include "objects/map.hpp"
#include "octets.hpp"
#include "support/fixtures.hpp"
#include "support/twin.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/dwgio.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO
{
namespace
{
auto Fixture(std::string_view name) -> std::filesystem::path
{
  return std::filesystem::path{DWGIO_FIXTURES_DIR} / "acad" / name;
}

auto Count(const Drawing &drawing, std::string_view name) -> std::size_t
{
  return static_cast<std::size_t>(
      std::ranges::count_if(drawing.Objects, [name](const Object &one) -> bool { return TypeOf(one) == name; }));
}

auto Names(const std::vector<Section> &blocks) -> std::vector<std::string>
{
  std::vector<std::string> names;
  for (const Section &block : blocks)
  {
    names.push_back(block.Name);
  }
  return names;
}
} // namespace

TEST_CASE("load: an AutoCAD R2000 drawing keeps every block and object")
{
  const std::expected<Loaded, Failure> loaded = LoadFile(Fixture("foundation-r2000.dwg"));
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  const Drawing &drawing = loaded->Drawing;
  CHECK(drawing.Release == Version::R2000);
  CHECK(drawing.Codepage == 30);
  CHECK(drawing.Objects.size() == 219);
  CHECK(drawing.Classes.size() == 17);
  CHECK(drawing.Preview.size() == 2);
  CHECK(drawing.Order == std::vector<std::string>{"auxiliary", "preview", "header", "classes", "pad", "objects", "map",
                                                  "second", "measurement"});
  CHECK(Names(drawing.Kept) == std::vector<std::string>{"auxiliary", "pad", "second", "measurement"});
  CHECK(drawing.Header.Handseed == Handle{0x417});
  CHECK(Count(drawing, "LINE") == 1);
  const auto line = std::ranges::find_if(drawing.Objects, [](const Object &one) { return TypeOf(one) == "LINE"; });
  REQUIRE(line != drawing.Objects.end());
  CHECK(BaseOf(*line).Owner == drawing.Header.BlockRecordModelSpace);
  CHECK(BaseOf(drawing.Objects.front()).Self == Handle{0x1});
}

TEST_CASE("load: an AutoCAD R14 drawing loads")
{
  const std::expected<Loaded, Failure> loaded = LoadFile(Fixture("foundation-r14.dwg"));
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  CHECK(loaded->Drawing.Release == Version::R14);
  CHECK(loaded->Drawing.Objects.size() == 305);
  CHECK(Count(loaded->Drawing, "LINE") + Count(loaded->Drawing, "CIRCLE") + Count(loaded->Drawing, "ARC") +
            Count(loaded->Drawing, "LWPOLYLINE") ==
        4);
  CHECK(Names(loaded->Drawing.Kept) == std::vector<std::string>{"pad", "second", "measurement"});
}

TEST_CASE("load: an encrypted R2007 drawing or a truncated coded file fails with a reason")
{
  std::vector<std::byte> file = ReadFixture("empty-r2007.dwg");
  file[0x18] = std::byte{1};
  CHECK(Load(file).error() ==
        Failure{.Code = FailureCode::Encrypted, .Offset = 0x18, .What = "encrypted drawing not read"});
  file.resize(0x100);
  CHECK(Load(file).error() == Failure{.Code = FailureCode::Damaged, .What = "coded header unreadable"});
}

TEST_CASE("load: bytes that are not a DWG file fail")
{
  CHECK(Load(BytesOf("hello")).error() == Failure{.Code = FailureCode::NotDwg, .What = "file too short"});
  CHECK(Load(BytesOf(std::string(100, 'x'))).error() == Failure{.Code = FailureCode::NotDwg, .What = "not a DWG file"});
  CHECK_FALSE(LoadFile(Fixture("missing.dwg")));
}

TEST_CASE("load: a truncated file keeps the objects before the cut")
{
  const std::vector<std::byte> whole = ReadFixture("foundation-r2000.dwg");
  const std::expected<Loaded, Failure> loaded = Load(std::span{whole}.first(60000));
  REQUIRE(loaded);
  CHECK(loaded->Drawing.Objects.size() > 50);
  CHECK(loaded->Drawing.Objects.size() < 219);
  CHECK(loaded->Drawing.Classes.size() == 17);
  REQUIRE_FALSE(loaded->Issues.empty());
  CHECK(std::ranges::any_of(loaded->Issues, [](const Issue &one) { return one.What == "map block beyond the file"; }));
  CHECK(
      std::ranges::any_of(loaded->Issues, [](const Issue &one) { return one.What.starts_with("object map missing"); }));
}

TEST_CASE("load: a stream loads as its bytes do")
{
  const std::vector<std::byte> whole = ReadFixture("foundation-r2000.dwg");
  std::istringstream stream{StringOf(whole)};
  const std::expected<Loaded, Failure> fromStream = Load(stream);
  const std::expected<Loaded, Failure> fromBytes = Load(whole);
  REQUIRE(fromStream);
  REQUIRE(fromBytes);
  CHECK(fromStream->Drawing == fromBytes->Drawing);
  std::ifstream closed{"/nonexistent/none.dwg"};
  CHECK_FALSE(Load(closed));
}

TEST_CASE("load: a code page dwgio lacks falls back to ANSI_1252 with an issue")
{
  std::vector<std::byte> bytes = ReadFixture("foundation-r2000.dwg");
  bytes[0x13] = std::byte{1};
  const std::expected<Loaded, Failure> loaded = Load(bytes);
  REQUIRE(loaded);
  CHECK(loaded->Drawing.Codepage == 1);
  CHECK(std::ranges::any_of(loaded->Issues,
                            [](const Issue &one) { return one.What == "code page 1 not carried, ANSI_1252 used"; }));
  const std::expected<Loaded, Failure> cut = Load(std::span{bytes}.first(21723));
  REQUIRE(cut);
  CHECK(cut->Drawing.Objects.empty());
}

TEST_CASE("load: an object the map points past the end of is left out with an issue")
{
  const std::vector<std::byte> whole = ReadFixture("foundation-r2000.dwg");
  const std::expected<Loaded, Failure> loaded = Load(whole);
  REQUIRE(loaded);
  Drawing drawing = loaded->Drawing;
  Unknown stub;
  stub.Base.Self = Handle{0x1};
  stub.Bytes = {std::byte{0xFF}, std::byte{0x7F}, std::byte{0x00}, std::byte{0x00}};
  drawing.Objects = {Object{stub}};
  const std::expected<Saved, Failure> saved = Save(drawing);
  REQUIRE(saved);
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Drawing.Objects.empty());
  CHECK(std::ranges::any_of(back->Issues, [](const Issue &one) { return one.What.ends_with("runs past the end"); }));
}

namespace
{
/**
 *  The R2000 fixture with its object map replaced, record 2 resized to match.
 */
auto ReplaceMap(std::vector<Objects::Mapped> entries) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes = ReadFixture("foundation-r2000.dwg");
  const std::vector<std::byte> map = Objects::WriteMap(entries);
  bytes.erase(bytes.begin() + 137708, bytes.begin() + 137708 + 708);
  bytes.insert(bytes.begin() + 137708, map.begin(), map.end());
  const auto size = static_cast<std::uint32_t>(map.size());
  for (std::size_t index = 0; index < 4; ++index)
  {
    bytes[0x19 + (9 * 2) + 5 + index] = static_cast<std::byte>(size >> (8 * index));
  }
  return bytes;
}
} // namespace

TEST_CASE("load: a map entry pointing before the objects is noted and the rest is read")
{
  const std::vector<std::byte> whole = ReadFixture("foundation-r2000.dwg");
  std::vector<Issue> scratch;
  std::vector<Objects::Mapped> entries =
      Objects::ReadMap(std::as_bytes(std::span{whole}).subspan(137708, 708), 137708, scratch);
  REQUIRE(entries.size() == 219);
  for (const std::size_t wrong : {std::size_t{50}, std::size_t{100}, std::size_t{300}, std::size_t{20190}})
  {
    std::vector<Objects::Mapped> patched = entries;
    patched.front().Offset = wrong;
    const std::expected<Loaded, Failure> loaded = Load(ReplaceMap(patched));
    REQUIRE(loaded);
    CHECK(loaded->Drawing.Objects.size() >= 218);
    CHECK_FALSE(loaded->Issues.empty());
  }
}

TEST_CASE("load: a preview whose size runs past the header is noted and left out")
{
  std::vector<std::byte> bytes = ReadFixture("foundation-r2000.dwg");
  bytes[237] = std::byte{0x60};
  const std::expected<Loaded, Failure> loaded = Load(bytes);
  REQUIRE(loaded);
  CHECK(loaded->Drawing.Preview.empty());
  CHECK(loaded->Drawing.Objects.size() == 219);
  CHECK(std::ranges::any_of(loaded->Issues,
                            [](const Issue &one) { return one.What == "preview block overlaps another"; }));
}

TEST_CASE("load: an issue gives the file's byte offset even inside a framed block")
{
  std::vector<std::byte> bytes = ReadFixture("foundation-r2000.dwg");
  bytes[20187 + 16] = std::byte{100};
  bytes[20187 + 17] = std::byte{0};
  const std::expected<Loaded, Failure> loaded = Load(bytes);
  REQUIRE(loaded);
  CHECK(std::ranges::any_of(loaded->Issues,
                            [](const Issue &one)
                            {
                              return one == Issue{.Code = IssueCode::Truncated,
                                                  .Offset = 20187 + 20 + 100,
                                                  .What = "header variables cut short"};
                            }));
}

TEST_CASE("load: the tables fixtures load clean with the twin's records")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  const auto counted = [&twin](std::string_view kind) -> std::ptrdiff_t
  {
    return std::ranges::count_if(twin, [kind](const Pair &one) { return one.Code == 0 && one.Value == kind; });
  };
  for (const std::string_view name : {"tables-r14.dwg", "tables-r2000.dwg"})
  {
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const auto typed = [&loaded](std::string_view type) -> std::ptrdiff_t
    {
      return std::ranges::count_if(loaded->Drawing.Objects, [type](const Object &one) { return TypeOf(one) == type; });
    };
    CHECK(typed("LAYER") == counted("LAYER"));
    CHECK(typed("LTYPE") == counted("LTYPE"));
    CHECK(typed("STYLE") == counted("STYLE"));
    CHECK(typed("APPID") == counted("APPID"));
    CHECK(typed("DIMSTYLE") == counted("DIMSTYLE"));
    CHECK(typed("BLOCK_HEADER") == counted("BLOCK_RECORD"));
    CHECK(typed("VPORT") == counted("VPORT"));
    CHECK(typed("VIEW") == counted("VIEW"));
    CHECK(typed("UCS") == counted("UCS"));
    CHECK(typed("LAYER") == 4);
  }
}

TEST_CASE("load: the entities fixtures load clean with the twin's entities")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  // The twin's ENTITIES and BLOCKS records of that kind; an INSERT with a 70 group is a MINSERT.
  const auto counted = [&twin](std::string_view kind) -> std::ptrdiff_t
  {
    std::ptrdiff_t count = 0;
    bool inside = false;
    for (std::size_t at = 0; at + 1 < twin.size(); ++at)
    {
      if (twin[at].Code == 2 && (twin[at].Value == "ENTITIES" || twin[at].Value == "BLOCKS") &&
          twin[at - 1].Value == "SECTION")
      {
        inside = true;
      }
      else if (twin[at].Code == 0 && twin[at].Value == "ENDSEC")
      {
        inside = false;
      }
      if (!inside || twin[at].Code != 0)
      {
        continue;
      }
      const bool grid = twin[at].Value == "INSERT" && !GroupOf(twin, at, 70).empty();
      const std::string_view value = grid ? "MINSERT" : std::string_view{twin[at].Value};
      count += value == kind ? 1 : 0;
    }
    return count;
  };
  for (const std::string_view name : {"entities-r14.dwg", "entities-r2000.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const auto typed = [&loaded](std::string_view type) -> std::ptrdiff_t
    {
      return std::ranges::count_if(loaded->Drawing.Objects, [type](const Object &one) { return TypeOf(one) == type; });
    };
    for (const std::string_view kind :
         {"LINE", "POINT", "CIRCLE", "ARC", "ELLIPSE", "RAY", "XLINE", "3DFACE", "SOLID", "TRACE", "SHAPE", "TEXT",
          "ATTDEF", "ATTRIB", "INSERT", "MINSERT", "MTEXT", "SEQEND"})
    {
      CAPTURE(kind);
      CHECK(typed(kind) == counted(kind));
      CHECK(typed(kind) >= 1);
    }
    CHECK(typed("LINE") == 4);
  }
}

TEST_CASE("load: the curves fixtures load clean with the twin's entities")
{
  const std::vector<Pair> twin = ReadTwin("curves-r2000.dxf");
  // The twin's ENTITIES and BLOCKS records of that kind.
  const auto counted = [&twin](std::string_view kind) -> std::ptrdiff_t
  {
    std::ptrdiff_t count = 0;
    bool inside = false;
    for (std::size_t at = 1; at < twin.size(); ++at)
    {
      if (twin[at].Code == 2 && (twin[at].Value == "ENTITIES" || twin[at].Value == "BLOCKS") &&
          twin[at - 1].Value == "SECTION")
      {
        inside = true;
      }
      else if (twin[at].Code == 0 && twin[at].Value == "ENDSEC")
      {
        inside = false;
      }
      count += inside && twin[at].Code == 0 && twin[at].Value == kind ? 1 : 0;
    }
    return count;
  };
  // The file's types a DXF name covers.
  const std::vector<std::pair<std::string_view, std::vector<std::string_view>>> kinds{
      {"POLYLINE", {"POLYLINE_2D", "POLYLINE_3D", "POLYLINE_PFACE", "POLYLINE_MESH"}},
      {"VERTEX", {"VERTEX_2D", "VERTEX_3D", "VERTEX_MESH", "VERTEX_PFACE", "VERTEX_PFACE_FACE"}},
      {"DIMENSION",
       {"DIMENSION_ORDINATE", "DIMENSION_LINEAR", "DIMENSION_ALIGNED", "DIMENSION_ANG3PT", "DIMENSION_ANG2LN",
        "DIMENSION_RADIUS", "DIMENSION_DIAMETER"}},
      {"LWPOLYLINE", {"LWPOLYLINE"}},
      {"SPLINE", {"SPLINE"}},
      {"HATCH", {"HATCH"}},
      {"LEADER", {"LEADER"}},
      {"TOLERANCE", {"TOLERANCE"}},
      {"MLINE", {"MLINE"}},
      {"VIEWPORT", {"VIEWPORT"}}};
  for (const std::string_view name : {"curves-r14.dwg", "curves-r2000.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    for (const auto &[dxf, types] : kinds)
    {
      CAPTURE(dxf);
      std::ptrdiff_t typed = 0;
      for (const std::string_view type : types)
      {
        typed +=
            std::ranges::count_if(loaded->Drawing.Objects, [type](const Object &one) { return TypeOf(one) == type; });
      }
      CHECK(typed == counted(dxf));
      CHECK(typed >= 1);
    }
  }
}

TEST_CASE("load: the objects fixtures load clean with the twin's objects")
{
  const std::vector<Pair> twin = ReadTwin("objects-r2000.dxf");
  // The twin's OBJECTS records of that kind.
  const auto counted = [&twin](std::string_view kind) -> std::ptrdiff_t
  {
    std::ptrdiff_t count = 0;
    bool inside = false;
    for (std::size_t at = 1; at < twin.size(); ++at)
    {
      if (twin[at].Code == 2 && twin[at].Value == "OBJECTS" && twin[at - 1].Value == "SECTION")
      {
        inside = true;
      }
      else if (twin[at].Code == 0 && twin[at].Value == "ENDSEC")
      {
        inside = false;
      }
      count += inside && twin[at].Code == 0 && twin[at].Value == kind ? 1 : 0;
    }
    return count;
  };
  // AutoCAD keeps one more dictionary and xrecord than it exports.
  const std::vector<std::pair<std::string_view, std::ptrdiff_t>> kinds{
      {"DICTIONARY", 1},      {"ACDBDICTIONARYWDFLT", 0}, {"XRECORD", 1}, {"LAYOUT", 0},
      {"GROUP", 0},           {"MLINESTYLE", 0},          {"SCALE", 0},   {"SORTENTSTABLE", 0},
      {"ACDBPLACEHOLDER", 0}, {"DICTIONARYVAR", 0}};
  for (const std::string_view name : {"objects-r14.dwg", "objects-r2000.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    for (const auto &[dxf, kept] : kinds)
    {
      CAPTURE(dxf);
      const std::ptrdiff_t typed =
          std::ranges::count_if(loaded->Drawing.Objects, [dxf](const Object &one) { return TypeOf(one) == dxf; });
      // An R14 save keeps many more dictionaries and xrecords of AutoCAD's own than it exports.
      if (loaded->Drawing.Release >= Version::R2000)
      {
        CHECK(typed == counted(dxf) + kept);
      }
      CHECK(typed >= counted(dxf));
      CHECK(typed >= 1);
    }
  }
}

TEST_CASE("load: the empty fixtures load clean with the named objects dictionary")
{
  for (const std::string_view name : {"empty-r14.dwg", "empty-r2000.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    CHECK(loaded->Drawing.Objects.size() >= 200);
    CHECK(Find(loaded->Drawing, loaded->Drawing.Header.DictionaryNamedObjects) != nullptr);
    CHECK(Find(loaded->Drawing, loaded->Drawing.Header.BlockRecordModelSpace) != nullptr);
  }
}

TEST_CASE("load: the empty R2004 fixture loads whole")
{
  const std::expected<Loaded, Failure> loaded = LoadFile(Fixture("empty-r2004.dwg"));
  REQUIRE(loaded);
  std::string whats;
  for (const Issue &issue : loaded->Issues)
  {
    whats += std::format("{} {} {}; ", issue.Offset.value_or(0), HexOf(issue.Subject), issue.What);
  }
  CHECK_MESSAGE(loaded->Issues.empty(), whats);
  CHECK(loaded->Drawing.Release == Version::R2004);
  CHECK(loaded->Drawing.Maintenance == 104);
  CHECK(loaded->Drawing.Objects.size() == 207);
  CHECK(loaded->Drawing.Order == std::vector<std::string>{"AcDb:SummaryInfo", "preview", "AcDb:AppInfo",
                                                          "AcDb:AppInfoHistory", "AcDb:RevHistory", "objects",
                                                          "AcDb:ObjFreeSpace", "measurement", "map", "classes",
                                                          "auxiliary", "header"});
  // The application information is not kept: every save makes it again, naming the program that saved.
  CHECK(Names(loaded->Drawing.Kept) == std::vector<std::string>{"AcDb:AppInfoHistory", "AcDb:RevHistory",
                                                                "AcDb:ObjFreeSpace", "measurement", "auxiliary"});
  // The summary typed, AutoCAD's save naming whoever saved it.
  REQUIRE(loaded->Drawing.SummaryInfo);
  CHECK_FALSE(loaded->Drawing.SummaryInfo->LastSavedBy.empty());
  CHECK(loaded->Drawing.Preview.size() == 2);
  CHECK(loaded->Drawing.Classes.size() >= 17);
  CHECK(Count(loaded->Drawing, "UNKNOWN") < 40);
  CHECK(View::Record<Tables::Layer>(loaded->Drawing, "0") != nullptr);
  CHECK(loaded->Drawing.Header.Dimassoc == 2);
}

TEST_CASE("load: every R2007 to R2018 fixture loads whole and typed like its R2004 twin")
{
  for (const std::string_view year : {"2007", "2010", "2013", "2018"})
  {
    for (const std::string_view name : {"empty", "tables", "entities", "curves", "objects", "foundation"})
    {
      CAPTURE(name);
      CAPTURE(year);
      const std::expected<Loaded, Failure> loaded = LoadFile(Fixture(std::format("{}-r{}.dwg", name, year)));
      REQUIRE(loaded);
      std::string whats;
      for (const Issue &issue : loaded->Issues)
      {
        whats += std::format("{} {} {}; ", issue.Offset.value_or(0), HexOf(issue.Subject), issue.What);
      }
      CHECK_MESSAGE(loaded->Issues.empty(), whats);
      CHECK(NameOf(loaded->Drawing.Release) == std::string{"R"} + std::string{year});
      CHECK(loaded->Drawing.Classes.size() >= 17);
      CHECK(Count(loaded->Drawing, "UNKNOWN") <= 32);
      CHECK(View::Record<Tables::Layer>(loaded->Drawing, "0") != nullptr);
      CHECK(loaded->Drawing.Header.Menuname == ".");
      const bool stored = std::ranges::contains(loaded->Drawing.Order, std::string{"AcDb:AcDsPrototype_1b"});
      CHECK(stored == (year == "2013" || year == "2018"));
    }
  }
}

TEST_CASE("load: a paged file with a broken header frame or objects mark or no container is noted")
{
  const std::vector<std::byte> bytes = ReadFixture("empty-r2004.dwg");
  std::vector<Issue> issues;
  File::Paged::Layout layout = *File::Paged::ReadLayout(std::as_bytes(std::span{bytes}), issues);
  for (File::Paged::Section &section : layout.Sections)
  {
    if (section.Name == "header")
    {
      section.Bytes.resize(8); // Cut inside its size field: no frame to cut.
    }
    else if (section.Name == "objects")
    {
      section.Bytes[0] = std::byte{0}; // The 0x0DCA.
    }
  }
  File::Paged::Head head = layout.Head;
  const std::vector<std::byte> rewritten = File::Paged::WriteLayout(head, layout.Sections);
  const std::expected<Loaded, Failure> loaded = Load(rewritten);
  REQUIRE(loaded);
  CHECK(std::ranges::any_of(loaded->Issues, [](const Issue &one) { return one.What.contains("0x0DCA"); }));
  CHECK(loaded->Issues.size() >= 2);
  // A paged file with no objects section at all loads with none.
  std::vector<File::Paged::Section> fewer;
  for (const File::Paged::Section &section : layout.Sections)
  {
    if (section.Name == "classes")
    {
      fewer.push_back(section);
    }
  }
  File::Paged::Head alone = layout.Head;
  const std::vector<std::byte> objectless = File::Paged::WriteLayout(alone, fewer);
  const std::expected<Loaded, Failure> emptied = Load(objectless);
  REQUIRE(emptied);
  CHECK(emptied->Drawing.Objects.empty());
  // Six characters of release and nothing of a container behind them.
  std::vector<std::byte> bare{bytes.begin(), bytes.begin() + 6};
  bare.resize(0x400);
  const std::expected<Loaded, Failure> none = Load(bare);
  REQUIRE_FALSE(none);
  CHECK_FALSE(none.error().What.empty());
}
} // namespace DwgIO
