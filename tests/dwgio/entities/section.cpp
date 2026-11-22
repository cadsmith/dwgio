/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/section.cpp
 *  @brief     The SECTIONOBJECT against its DXF twin in every release, and written back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/section.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/class.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  The twin's points of that group in its record, in order.
 */
auto Points(const std::vector<Pair> &twin, std::size_t opened, int code) -> std::vector<Vector>
{
  const std::vector<std::string> xs = Every(twin, opened, code);
  const std::vector<std::string> ys = Every(twin, opened, code + 10);
  const std::vector<std::string> zs = Every(twin, opened, code + 20);
  std::vector<Vector> points;
  for (std::size_t at = 0; at < xs.size(); ++at)
  {
    points.push_back(Vector{.X = std::stod(xs[at]), .Y = std::stod(ys[at]), .Z = std::stod(zs[at])});
  }
  return points;
}
} // namespace

TEST_CASE("entities/section: the section plane matches the DXF twin in every release")
{
  for (const auto &[name, twinned] :
       {std::pair{"kinds-r14.dwg", "kinds-r2000.dxf"}, std::pair{"kinds-r2000.dwg", "kinds-r2000.dxf"},
        std::pair{"kinds-r2004.dwg", "kinds-r2004.dxf"}, std::pair{"kinds-r2007.dwg", "kinds-r2007.dxf"},
        std::pair{"kinds-r2010.dwg", "kinds-r2010.dxf"}, std::pair{"kinds-r2013.dwg", "kinds-r2013.dxf"},
        std::pair{"kinds-r2018.dwg", "kinds-r2018.dxf"}})
  {
    CAPTURE(name);
    const std::vector<Pair> twin = ReadTwin(twinned);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const Drawing &drawing = loaded->Drawing;
    std::size_t seen = 0;
    for (const Object &object : drawing.Objects)
    {
      const auto *section = GetIf<SectionObject>(&object);
      if (section == nullptr)
      {
        continue;
      }
      ++seen;
      CHECK(TypeOf(object) == "SECTIONOBJECT");
      const std::optional<std::size_t> entity = Entity(twin, section->Base.Self);
      REQUIRE(entity);
      // The subclass's own groups: R2000's proxy graphics size is a 92 group too, and its color a 62 the entity's
      // common data would be taken for.
      std::optional<std::size_t> at = *entity;
      while (twin[*at].Value != "AcDbSection")
      {
        ++*at;
      }
      std::vector<Pair> common{twin.begin(), twin.begin() + static_cast<std::ptrdiff_t>(*at)};
      common.push_back(Pair{.Code = 0, .Value = "END"});
      CheckCommon(TwinPlace{.Twin = &common, .At = *entity, .Drawing = &drawing}, section->Common, section->Base);
      CHECK(section->State == ReadWhole(twin, *at, 90));
      CHECK(section->Flags == ReadWhole(twin, *at, 91));
      CHECK(section->Name == GroupOf(twin, *at, 1));
      CHECK(Near(section->Vertical, ReadPoint(twin, *at, 10)));
      CHECK(section->Top == doctest::Approx(ReadReal(twin, *at, 40)));
      CHECK(section->Bottom == doctest::Approx(ReadReal(twin, *at, 41)));
      CHECK(section->Transparency == ReadWhole(twin, *at, 70));
      // R2000's DXF leaves the color out; the file has it all the same.
      CHECK(section->Color.Index == (drawing.Release >= Version::R2004 ? ReadWhole(twin, *at, 62) : 9));
      CHECK_FALSE(section->Color.Rgb);
      const std::vector<Vector> vertices = Points(twin, *at, 11);
      REQUIRE(section->Vertices.size() == vertices.size());
      REQUIRE(section->Vertices.size() == static_cast<std::size_t>(ReadWhole(twin, *at, 92)));
      for (std::size_t one = 0; one < vertices.size(); ++one)
      {
        CHECK(Near(section->Vertices[one], vertices[one]));
      }
      CHECK(section->Back.size() == static_cast<std::size_t>(ReadWhole(twin, *at, 93)));
      CHECK(section->Settings == ParseHandle(GroupOf(twin, *at, 360)));
    }
    CHECK(seen == 1);
  }
}

TEST_CASE("entities/section: a section with a back line and a named color round-trips in every release")
{
  std::vector<Issue> issues;
  const std::vector<Class> classes{Class{.Number = 500, .DxfName = "SECTIONOBJECT", .ItemClass = 0x1F2}};
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2007, Version::R2018})
  {
    CAPTURE(NameOf(release));
    const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues, .Classes = &classes};
    const Objects::FileContext reading{
        .Release = release, .Text = &Objects::Latin, .Classes = classes, .Issues = &issues};
    SectionObject section;
    section.Base.Self = Handle{0x40};
    section.Base.Owner = Handle{0x1F};
    section.Common.Layer = Handle{0x10};
    section.Common.Lineweight = release >= Version::R2000 ? std::optional{Lineweight::ByLayer} : std::nullopt;
    section.Common.Previous = Handle{0x3F};
    section.Common.Next = Handle{0x41};
    section.Name = "Cut";
    section.Color = Color{
        .Index = 0, .Rgb = -1040153472, .Names = ColorNames{.Name = std::string{"Teal"}, .Book = std::string{"Book"}}};
    section.Vertices = {Vector{.X = 1}, Vector{.X = 2}};
    section.Back = {Vector{.Y = 3}};
    section.Settings = Handle{0x42};
    if (release >= Version::R2004)
    {
      section.Common.Previous = {};
      section.Common.Next = {};
    }
    const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{section}, with), 0, reading);
    REQUIRE(back);
    SectionObject read = Get<SectionObject>(*back);
    read.Base = section.Base; // The section's own fields, the common ones being every entity's.
    read.Common = section.Common;
    CHECK(read == section);
  }
  CHECK(issues.empty());
}
} // namespace DwgIO::Entities
