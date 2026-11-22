/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/mlinestyle.cpp
 *  @brief     The MLINESTYLE against its DXF twin, and written back as R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/mlinestyle.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/objects.hpp"
#include "support/reader.hpp"

#include "dwgio/class.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/mlinestyle: the fixture's Standard style matches the DXF twin with angles in radians")
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
      const auto *style = GetIf<MLineStyle>(&object);
      if (style == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(style->Base.Self));
      CHECK(TypeOf(object) == "MLINESTYLE");
      const std::optional<std::size_t> opened = FindHandled(twin, style->Base.Self);
      REQUIRE(opened);
      CHECK(SameName(style->Name, GroupOf(twin, *opened, 2), drawing));
      CHECK(style->Description == GroupOf(twin, *opened, 3));
      CHECK(style->Flags == ReadWhole(twin, *opened, 70));
      CHECK(style->Fill.Index == ReadWhole(twin, *opened, 62));
      CHECK(style->Start == doctest::Approx(ReadReal(twin, *opened, 51) * std::numbers::pi / 180.0));
      CHECK(style->End == doctest::Approx(ReadReal(twin, *opened, 52) * std::numbers::pi / 180.0));
      const std::vector<std::string> offsets = Every(twin, *opened, 49);
      const std::vector<std::string> colors = Every(twin, *opened, 62);
      REQUIRE(style->Elements.size() == offsets.size());
      REQUIRE(colors.size() == offsets.size() + 1);
      for (std::size_t at = 0; at < offsets.size(); ++at)
      {
        CHECK(style->Elements[at].Offset == doctest::Approx(std::stod(offsets[at])));
        CHECK(style->Elements[at].Color.Index == std::stoi(colors[at + 1]));
      }
      CHECK(style->Base.Owner == drawing.Header.DictionaryAcadMLineStyle);
    }
    CHECK(count == 1);
  }
}

TEST_CASE("objects/mlinestyle: a style with every cap bit and three elements round-trips as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const Objects::FileContext reading{.Release = Version::R2000, .Text = &Latin, .Classes = {}, .Issues = &issues};
  MLineStyle style;
  style.Base.Self = Handle{0x100};
  style.Base.Owner = Handle{0x17};
  style.Name = "WIDE";
  style.Description = "three lines";
  style.Flags = 1 | 2 | 16 | 32 | 64 | 256 | 512 | 1024;
  style.Fill = Color{.Index = 3};
  style.Start = 1.25;
  style.End = 1.75;
  style.Elements = {MLineStyles::Element{.Offset = 1, .Color = Color{.Index = 1}, .Linetype = 2},
                    MLineStyles::Element{.Offset = 0, .Color = Color{.Index = 256}, .Linetype = 32767},
                    MLineStyles::Element{.Offset = -1, .Color = Color{.Index = 2}, .Linetype = 0}};
  std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{style}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{style});
  for (const int flags : {1, 2, 32, 64, 512, 1024})
  {
    style.Flags = flags;
    back = Objects::ReadObject(Objects::WriteObject(Object{style}, with), 0, reading);
    REQUIRE(back);
    CHECK(*back == Object{style});
  }
  CHECK(issues.empty());
}
} // namespace DwgIO::Objects
