/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/text.cpp
 *  @brief     The TEXT against its DXF twin, its R14 alignment, and its data flags.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/text.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/text.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
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
TEST_CASE("entities/text: the fixture's texts match the DXF twin including a mirrored one and a justified one")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  for (const std::string_view name : {"entities-r2000.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    std::size_t backwards = 0;
    std::size_t justified = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *text = GetIf<Text>(&object);
      if (text == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(text->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, text->Base.Self);
      REQUIRE(opened);
      CHECK(TypeOf(object) == "TEXT");
      CheckLettering(twin, *opened, text->Lettering);
      backwards += text->Lettering.Generation == 2 ? 1U : 0U;
      if (text->Lettering.Horizontal == HorizontalAlignment::Center &&
          text->Lettering.Vertical == VerticalAlignment::Middle)
      {
        ++justified;
        CHECK_FALSE(Near(text->Lettering.Alignment, text->Lettering.Position));
      }
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, text->Common, text->Base);
    }
    CHECK(count == 3);
    CHECK(backwards == 1);
    CHECK(justified == 1);
  }
}

namespace
{
/**
 *  Writes the lettering through the TEXT layout as R2000 and reads it back; the first byte is the data flags.
 */
auto Written(const Text &text, std::uint8_t expected) -> Text
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  Bits::Writer data;
  Bits::Writer handles;
  Fields::Output to{.To = &data, .Handles = &handles, .With = with};
  REQUIRE(Fields::Write(Objects::Shape<Text>::Layout(), text, to));
  REQUIRE_FALSE(data.Bytes().empty());
  CHECK(static_cast<std::uint8_t>(data.Bytes().front()) == expected);
  Bits::Reader dataIn{data.Bytes()};
  Bits::Reader handlesIn{handles.Bytes()};
  Fields::Input in{.In = &dataIn, .Handles = &handlesIn, .With = with};
  Text back;
  REQUIRE(Fields::Read(Objects::Shape<Text>::Layout(), back, in));
  CHECK(issues.empty());
  return back;
}

/**
 *  Writes the text through the TEXT layout as R14 and reads it back.
 */
auto Older(const Text &text) -> Text
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R14, .Text = &Objects::Latin, .Issues = &issues};
  Bits::Writer data;
  Bits::Writer handles;
  Fields::Output to{.To = &data, .Handles = &handles, .With = with};
  REQUIRE(Fields::Write(Objects::Shape<Text>::Layout(), text, to));
  Bits::Reader dataIn{data.Bytes()};
  Bits::Reader handlesIn{handles.Bytes()};
  Fields::Input in{.In = &dataIn, .Handles = &handlesIn, .With = with};
  Text back;
  REQUIRE(Fields::Read(Objects::Shape<Text>::Layout(), back, in));
  CHECK(issues.empty());
  return back;
}
} // namespace

TEST_CASE("entities/text: an R14 baseline-left text takes its position as its alignment point")
{
  Text left;
  left.Lettering.Position = Vector{.X = 1, .Y = 2, .Z = 0};
  left.Lettering.Height = 2.5;
  left.Lettering.Value = "left";
  left.Lettering.Style = Handle{0x11};
  const Text read = Older(left);
  CHECK(read.Lettering.Alignment == left.Lettering.Position);
  Text middle = left;
  middle.Lettering.Vertical = VerticalAlignment::Middle;
  middle.Lettering.Alignment = Vector{.X = 3, .Y = 4, .Z = 0};
  CHECK(Older(middle) == middle);
  Text centered = middle;
  centered.Lettering.Vertical = VerticalAlignment::Baseline;
  centered.Lettering.Horizontal = HorizontalAlignment::Center;
  CHECK(Older(centered) == centered);
}

TEST_CASE("entities/text: the data flags mark which values are left out with 0xFF for all defaults")
{
  Text plain;
  plain.Lettering.Height = 2.5;
  plain.Lettering.Value = "plain";
  plain.Lettering.Style = Handle{0x11};
  CHECK(Written(plain, 0xFF) == plain);
  Text full;
  full.Lettering.Position = Vector{.X = 1, .Y = 2, .Z = 3};
  full.Lettering.Alignment = Vector{.X = 4, .Y = 5, .Z = 3};
  full.Lettering.Height = 2.5;
  full.Lettering.Value = "full";
  full.Lettering.Rotation = 0.5;
  full.Lettering.Width = 0.8;
  full.Lettering.Oblique = 0.25;
  full.Lettering.Generation = 2;
  full.Lettering.Horizontal = HorizontalAlignment::Center;
  full.Lettering.Vertical = VerticalAlignment::Middle;
  full.Lettering.Thickness = 1;
  full.Lettering.Style = Handle{0x11};
  CHECK(Written(full, 0x00) == full);
  Text above = plain;
  above.Lettering.Position = Vector{.X = 1, .Y = 2, .Z = 0};
  above.Lettering.Alignment = Vector{.X = 1, .Y = 5, .Z = 0};
  CHECK(Written(above, 0xFD) == above);
}
} // namespace DwgIO::Entities
