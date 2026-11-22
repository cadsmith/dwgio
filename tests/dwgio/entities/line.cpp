/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/line.cpp
 *  @brief     The LINE against its DXF twin, an R14 line written as R2000, and every common field.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/line.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
TEST_CASE("entities/line: the fixture's lines match the DXF twin with their states and xdata")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  const std::optional<std::size_t> registered = FindOpening(twin, "APPID", "DWGIO");
  REQUIRE(registered);
  const Handle application = Self(twin, *registered);
  for (const std::string_view name : {"entities-r2000.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    std::size_t invisible = 0;
    std::size_t extended = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *line = GetIf<Line>(&object);
      if (line == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(line->Base.Self));
      CHECK(TypeOf(object) == "LINE");
      const std::optional<std::size_t> opened = Entity(twin, line->Base.Self);
      REQUIRE(opened);
      CHECK(Near(line->Start, ReadPoint(twin, *opened, 10)));
      CHECK(Near(line->End, ReadPoint(twin, *opened, 11)));
      CHECK(line->Thickness == doctest::Approx(ReadReal(twin, *opened, 39)));
      CHECK(Near(line->Extrusion, Normal(twin, *opened)));
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, line->Common, line->Base);
      invisible += line->Common.Invisible ? 1U : 0U;
      for (const Xdata &record : line->Base.Extended)
      {
        if (record.AppId != application)
        {
          continue;
        }
        ++extended;
        REQUIRE(record.Tags.size() == 3);
        CHECK(record.Tags.front() == DwgIO::Tag{.Code = 1000, .Value = std::string{"note"}});
        CHECK(line->Base.Reactors.size() == 1);
      }
    }
    CHECK(count == 4);
    CHECK(invisible == 1);
    CHECK(extended == 1);
  }
}

// An R14 file keeps a lineweight of the entity's own in extended data, so only the lines at their layer's width
// compare; an object read on its own has no header to take a space's owner from, so the owner is set as Load does.
TEST_CASE("entities/line: an R14 line written as R2000 reads back like the R2000 fixture's")
{
  const std::expected<Loaded, Failure> older = Load(ReadFixture("entities-r14.dwg"));
  const std::expected<Loaded, Failure> newer = Load(ReadFixture("entities-r2000.dwg"));
  REQUIRE(older);
  REQUIRE(newer);
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  std::size_t compared = 0;
  for (const Object &object : older->Drawing.Objects)
  {
    const auto *line = GetIf<Line>(&object);
    if (line == nullptr)
    {
      continue;
    }
    std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(object, with), 0, reading);
    REQUIRE(back);
    auto *read = GetIf<Line>(&*back);
    REQUIRE(read);
    read->Base.Owner = line->Base.Owner;
    for (const Object &other : newer->Drawing.Objects)
    {
      const auto *same = GetIf<Line>(&other);
      if (same != nullptr && same->Base.Self == line->Base.Self && same->Common.Lineweight == Lineweight::ByLayer)
      {
        CHECK(*read == *same);
        ++compared;
      }
    }
  }
  CHECK(compared == 3);
  CHECK(issues.empty());
}

TEST_CASE("entities/line: graphics and a named plot style and far links and a height round-trip")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Line line;
  line.Base.Self = Handle{0x100};
  line.Base.Owner = Handle{0x20};
  line.Base.Reactors = {Handle{0x30}};
  line.Common.Space = Space::Owner;
  line.Common.Graphics = {std::byte{1}, std::byte{2}, std::byte{3}};
  line.Common.LinetypeOrigin = Origin::Record;
  line.Common.Linetype = Handle{0x16};
  line.Common.PlotstyleOrigin = Origin::Record;
  line.Common.Plotstyle = Handle{0xF};
  line.Common.Previous = Handle{0x50};
  line.Common.Next = Handle{0x200};
  line.Common.Lineweight = Lineweight::Mm050;
  line.Common.Layer = Handle{0x10};
  line.Start = Vector{.X = 1, .Y = 2, .Z = 3};
  line.End = Vector{.X = 4, .Y = 5, .Z = 6};
  line.Thickness = 2;
  std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{line}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{line});
  line.Start.Z = 0;
  back = Objects::ReadObject(Objects::WriteObject(Object{line}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{line});
  line.Common.Space = Space::Model;
  line.Base.Owner = Handle{};
  line.Common.Previous = Handle{0xFF};
  line.Common.Next = Handle{0x101};
  line.Common.Lineweight = std::nullopt;
  back = Objects::ReadObject(Objects::WriteObject(Object{line}, with), 0, reading);
  REQUIRE(back);
  line.Common.Lineweight = Lineweight::ByLayer;
  CHECK(*back == Object{line});
  CHECK(issues.empty());
}
} // namespace DwgIO::Entities
