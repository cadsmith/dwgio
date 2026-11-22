/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/layout.cpp
 *  @brief     The LAYOUT against its DXF twin, written back, and its fixed type in R2004 files.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/layout.hpp"

#include "bits/reader.hpp"
#include "classes/layout.hpp"
#include "fields/context.hpp"
#include "file/paged/layout.hpp"
#include "objects/map.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/cut.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"
#include "support/vector.hpp"

#include "dwgio/class.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/placeholder.hpp"
#include "dwgio/view.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
/**
 *  The groups of one part of a LAYOUT record: the plot settings before the AcDbLayout marker, the layout after.
 */
auto Part(const std::vector<Pair> &twin, std::size_t opened, bool layout) -> std::vector<Pair>
{
  std::vector<Pair> part;
  bool after = false;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    if (twin[at].Code == 100 && twin[at].Value == "AcDbLayout")
    {
      after = true;
      continue;
    }
    if (after == layout)
    {
      part.push_back(twin[at]);
    }
  }
  return part;
}

auto Text(const std::vector<Pair> &part, int code) -> std::string
{
  for (const Pair &pair : part)
  {
    if (pair.Code == code)
    {
      return pair.Value;
    }
  }
  return {};
}

auto Number(const std::vector<Pair> &part, int code) -> double
{
  const std::string value = Text(part, code);
  return value.empty() ? 0.0 : std::stod(value);
}

auto Point(const std::vector<Pair> &part, int code) -> Vector
{
  return Vector{.X = Number(part, code), .Y = Number(part, code + 10), .Z = Number(part, code + 20)};
}

void CheckPlot(const std::vector<Pair> &part, const Plot &plot, const Drawing &drawing)
{
  CHECK(SameName(plot.Name, Text(part, 1), drawing));
  CHECK(plot.Device == Text(part, 2));
  CHECK(plot.Size == Text(part, 4));
  CHECK(plot.View == Text(part, 6));
  CHECK(plot.Style == Text(part, 7));
  CHECK(plot.Left == doctest::Approx(Number(part, 40)));
  CHECK(plot.Bottom == doctest::Approx(Number(part, 41)));
  CHECK(plot.Right == doctest::Approx(Number(part, 42)));
  CHECK(plot.Top == doctest::Approx(Number(part, 43)));
  CHECK(plot.Width == doctest::Approx(Number(part, 44)));
  CHECK(plot.Height == doctest::Approx(Number(part, 45)));
  CHECK(plot.Origin.X == doctest::Approx(Number(part, 46)));
  CHECK(plot.Origin.Y == doctest::Approx(Number(part, 47)));
  CHECK(plot.Lower.X == doctest::Approx(Number(part, 48)));
  CHECK(plot.Lower.Y == doctest::Approx(Number(part, 49)));
  CHECK(plot.Upper.X == doctest::Approx(Number(part, 140)));
  CHECK(plot.Upper.Y == doctest::Approx(Number(part, 141)));
  CHECK(plot.PaperUnits == doctest::Approx(Number(part, 142)));
  CHECK(plot.DrawingUnits == doctest::Approx(Number(part, 143)));
  CHECK(plot.Flags == static_cast<int>(Number(part, 70)));
  CHECK(plot.Units == static_cast<int>(Number(part, 72)));
  CHECK(plot.Rotation == static_cast<int>(Number(part, 73)));
  CHECK(plot.Type == static_cast<int>(Number(part, 74)));
  CHECK(plot.Standard == static_cast<int>(Number(part, 75)));
  CHECK(plot.Factor == doctest::Approx(Number(part, 147)));
  CHECK(plot.Image.X == doctest::Approx(Number(part, 148)));
  CHECK(plot.Image.Y == doctest::Approx(Number(part, 149)));
}
} // namespace

TEST_CASE("objects/layout: every layout of the fixture matches the DXF twin with the Sheet layout's viewports")
{
  const std::vector<Pair> twin = ReadTwin("objects-r2000.dxf");
  for (const std::string_view name : {"objects-r2000.dwg", "objects-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    std::size_t count = 0;
    for (const Object &object : drawing.Objects)
    {
      const auto *layout = GetIf<Layout>(&object);
      if (layout == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(layout->Base.Self));
      CHECK(TypeOf(object) == "LAYOUT");
      const std::optional<std::size_t> opened = FindHandled(twin, layout->Base.Self);
      REQUIRE(opened);
      CheckPlot(Part(twin, *opened, false), layout->Plot, drawing);
      const std::vector<Pair> part = Part(twin, *opened, true);
      CHECK(SameName(layout->Name, Text(part, 1), drawing));
      CHECK(layout->Flags == static_cast<int>(Number(part, 70)));
      CHECK(layout->Order == static_cast<int>(Number(part, 71)));
      CHECK(Near(layout->Limmin, Point(part, 10)));
      CHECK(Near(layout->Limmax, Point(part, 11)));
      CHECK(Near(layout->Insbase, Point(part, 12)));
      CHECK(Near(layout->Axes.Origin, Point(part, 13)));
      CHECK(Near(layout->Extmin, Point(part, 14)));
      CHECK(Near(layout->Extmax, Point(part, 15)));
      CHECK(Near(layout->Axes.X, Point(part, 16)));
      CHECK(Near(layout->Axes.Y, Point(part, 17)));
      CHECK(layout->Axes.Elevation == doctest::Approx(Number(part, 146)));
      CHECK(layout->Axes.Ortho == static_cast<int>(Number(part, 76)));
      CHECK(layout->Block == ParseHandle(Text(part, 330)).value_or(Handle{}));
      CHECK(layout->Viewport == ParseHandle(Text(part, 331)).value_or(Handle{}));
      CHECK(layout->Axes.Named == ParseHandle(Text(part, 345)).value_or(Handle{}));
      CHECK(layout->Axes.Basis == ParseHandle(Text(part, 346)).value_or(Handle{}));
      CHECK(layout->Base.Owner ==
            (drawing.Release >= Version::R2000 ? drawing.Header.DictionaryLayouts : layout->Base.Owner));
      if (SameName(layout->Name, "Sheet", drawing))
      {
        CHECK(layout->Order == 3);
        const auto *viewport = GetIf<Entities::Viewport>(FindObject(drawing, layout->Viewport));
        REQUIRE(viewport);
        CHECK(viewport->Base.Owner == layout->Block);
        std::size_t viewports = 0;
        for (const Object &other : drawing.Objects)
        {
          const auto *one = GetIf<Entities::Viewport>(&other);
          viewports += one != nullptr && one->Base.Owner == layout->Block ? 1U : 0U;
        }
        CHECK(viewports == 4);
      }
    }
    CHECK(count == 4);
    const auto *root = GetIf<Dictionary>(FindObject(drawing, drawing.Header.DictionaryNamedObjects));
    REQUIRE(root);
    const Handle *layouts = EntryNamed(*root, "ACAD_LAYOUT", drawing);
    REQUIRE(layouts);
    const auto *dictionary = GetIf<Dictionary>(FindObject(drawing, *layouts));
    REQUIRE(dictionary);
    CHECK(dictionary->Entries.size() == 4);
    for (const Entry &entry : dictionary->Entries)
    {
      CHECK(Holds<Layout>(*FindObject(drawing, entry.Object)));
    }
  }
}

TEST_CASE("objects/layout: a layout with every handle set round-trips under its class")
{
  std::vector<Issue> issues;
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "LAYOUT"}};
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Classes = &classes};
  const Objects::FileContext reading{.Release = Version::R2000, .Text = &Latin, .Classes = classes, .Issues = &issues};
  Layout layout;
  layout.Base.Self = Handle{0x100};
  layout.Base.Owner = Handle{0x1A};
  layout.Plot.Name = "Setup";
  layout.Plot.Device = "none_device";
  layout.Plot.Size = "ISO_A4";
  layout.Plot.View = "Front";
  layout.Plot.Style = "acad.ctb";
  layout.Plot.Left = 7.5;
  layout.Plot.Width = 210;
  layout.Plot.Origin = Vector{.X = 1, .Y = 2, .Z = 0};
  layout.Plot.Lower = Vector{.X = 3, .Y = 4, .Z = 0};
  layout.Plot.Upper = Vector{.X = 5, .Y = 6, .Z = 0};
  layout.Plot.Flags = 11952;
  layout.Plot.Type = 4;
  layout.Plot.Standard = 0;
  layout.Plot.Factor = 0.5;
  layout.Plot.Image = Vector{.X = 7, .Y = 8, .Z = 0};
  layout.Name = "Sheet";
  layout.Flags = 3;
  layout.Order = 3;
  layout.Limmin = Vector{.X = -20, .Y = -7.5, .Z = 0};
  layout.Limmax = Vector{.X = 277, .Y = 202.5, .Z = 0};
  layout.Insbase = Vector{.X = 1, .Y = 1, .Z = 1};
  layout.Extmin = Vector{.X = 10, .Y = 10, .Z = 0};
  layout.Extmax = Vector{.X = 231.3, .Y = 175.5, .Z = 0};
  layout.Axes.Origin = Vector{.X = 2, .Y = 2, .Z = 2};
  layout.Axes.Elevation = 5;
  layout.Axes.Ortho = 1;
  layout.Axes.Named = Handle{0x60};
  layout.Axes.Basis = Handle{0x61};
  layout.Block = Handle{0x324};
  layout.Viewport = Handle{0x332};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{layout}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{layout});
  CHECK(TypeOf(*back) == "LAYOUT");
  CHECK(issues.empty());
}
} // namespace DwgIO::Objects

namespace DwgIO::Objects
{
TEST_CASE("objects/layout: R2004 stores layouts and placeholders under fixed types and R2000 under classes")
{
  // AutoCAD's R2004 files carry LAYOUT as type 82 and ACDBPLACEHOLDER as type 80, the classes still declared;
  // its R2000 files store both under their classes. Read either way, written as the release wants.
  const Drawing newer = Load(ReadFixture("empty-r2004.dwg")).value().Drawing;
  const std::vector<const Layout *> layouts = View::Layouts(newer);
  REQUIRE(layouts.size() == 3);
  CHECK(layouts[0]->Name == "Model");
  CHECK(layouts[1]->Name == "Layout1");
  CHECK(layouts[2]->Name == "Layout2");
  CHECK(GetIf<Placeholder>(Find(newer, Handle{.Value = 0xF})) != nullptr);
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2004, .Text = &Latin, .Issues = &issues, .Classes = &newer.Classes};
  const std::vector<std::byte> written = WriteObject(*Find(newer, layouts[0]->Base.Self), with);
  Bits::Reader in{written};
  in.MS();
  CHECK(in.OT(Version::R2004) == 82);
  const Drawing older = Load(ReadFixture("empty-r2000.dwg")).value().Drawing;
  const Fields::Context before{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Classes = &older.Classes};
  const std::vector<std::byte> older22 = WriteObject(*Find(older, Handle{.Value = 0x22}), before);
  Bits::Reader was{older22};
  was.MS();
  CHECK(was.OT(Version::R2000) >= 500);
  CHECK(issues.empty());
}

TEST_CASE("objects/layout: a viewport count beyond the handle stream is refused before allocating")
{
  // The count's two bits (a BL of zero) sit right before the handles; cleared to a long's code, the count reads
  // the handle stream's first bits as a number far beyond any list.
  std::vector<Issue> issues;
  const File::Paged::Layout layout = *File::Paged::ReadLayout(ReadFixture("empty-r2004.dwg"), issues);
  const auto objects = std::ranges::find(layout.Sections, "objects", &File::Paged::Section::Name);
  const auto map = std::ranges::find(layout.Sections, "map", &File::Paged::Section::Name);
  const auto classes = std::ranges::find(layout.Sections, "classes", &File::Paged::Section::Name);
  REQUIRE(objects != layout.Sections.end());
  const std::vector<Class> declared =
      Classes::Read(std::span{classes->Bytes}.first(classes->Bytes.size() - Classes::Trailer),
                    Fields::Context{.Release = Version::R2004, .Text = &Latin, .Issues = &issues});
  const Objects::FileContext reading{.Release = Version::R2004, .Text = &Latin, .Classes = declared, .Issues = &issues};
  for (const Mapped &entry : ReadMap(map->Bytes, 0, issues))
  {
    if (entry.Self != Handle{.Value = 0x22})
    {
      continue;
    }
    std::vector<std::byte> bytes(objects->Bytes.begin() + static_cast<std::ptrdiff_t>(entry.Offset),
                                 objects->Bytes.end());
    Bits::Reader in{bytes};
    in.MS();
    const std::size_t start = in.Position();
    in.OT(Version::R2004);
    const std::uint32_t handles = in.RL();
    const std::size_t bit = start + handles - 2;
    bytes[bit / 8] &= static_cast<std::byte>(~(0x80U >> (bit % 8)));
    issues.clear();
    const std::optional<Object> read = ReadObject(bytes, 0, reading);
    REQUIRE(read);
    CHECK(Holds<Layout>(*read));
    CHECK(std::ranges::any_of(issues, [](const Issue &issue) { return issue.What.contains("beyond the handles"); }));
    return;
  }
  FAIL("layout 22 not in the map");
}

TEST_CASE("objects/layout: an R2004 layout cut before its viewport count is noted and not sized")
{
  Layout layout;
  layout.Base.Self = Handle{.Value = 0x22};
  layout.Name = "Model";
  CHECK(Objects::CutShort<Layout>(Object{layout}, Version::R2004) > 0);
}
} // namespace DwgIO::Objects
