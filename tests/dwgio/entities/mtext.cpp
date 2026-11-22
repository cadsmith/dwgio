/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/mtext.cpp
 *  @brief     The MTEXT against its DXF twin, and a default one's reading direction and attachment.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/mtext.hpp"

#include "support/entities.hpp"
#include "support/fixtures.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  The twin's text: every 3 group in turn, then the 1 group.
 */
auto JoinText(const std::vector<Pair> &twin, std::size_t opened) -> std::string
{
  std::string text;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    if (twin[at].Code == 3)
    {
      text += twin[at].Value;
    }
  }
  return text + GroupOf(twin, opened, 1);
}
} // namespace

TEST_CASE("entities/mtext: the fixture's mtext matches the DXF twin with its two paragraphs in one string")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  for (const std::string_view name : {"entities-r2000.dwg", "entities-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *mtext = GetIf<MText>(&object);
      if (mtext == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(mtext->Base.Self));
      CHECK(TypeOf(object) == "MTEXT");
      const std::optional<std::size_t> opened = Entity(twin, mtext->Base.Self);
      REQUIRE(opened);
      CHECK(Near(mtext->Position, ReadPoint(twin, *opened, 10)));
      CHECK(Near(mtext->Extrusion, Normal(twin, *opened)));
      const Vector direction =
          GroupOf(twin, *opened, 11).empty() ? Vector{.X = 1, .Y = 0, .Z = 0} : ReadPoint(twin, *opened, 11);
      CHECK(Near(mtext->Direction, direction));
      CHECK(mtext->Width == doctest::Approx(ReadReal(twin, *opened, 41)));
      CHECK(mtext->Height == doctest::Approx(ReadReal(twin, *opened, 40)));
      CHECK(mtext->Attachment == ReadWhole(twin, *opened, 71));
      CHECK(mtext->Flow == ReadWhole(twin, *opened, 72));
      CHECK(mtext->ExtentHeight > mtext->Height);
      CHECK(mtext->ExtentWidth > 0.0);
      CHECK(mtext->Value == JoinText(twin, *opened));
      CHECK(mtext->Value == "First line\\PSecond line");
      const std::string style = GroupOf(twin, *opened, 7);
      const std::optional<std::size_t> named = FindOpeningNoCase(twin, "STYLE", style.empty() ? "Standard" : style);
      REQUIRE(named);
      CHECK(mtext->Style == Self(twin, *named));
      CHECK(mtext->LineSpacingStyle == (GroupOf(twin, *opened, 73).empty() ? 1 : ReadWhole(twin, *opened, 73)));
      CHECK(mtext->LineSpacingFactor ==
            doctest::Approx(GroupOf(twin, *opened, 44).empty() ? 1.0 : ReadReal(twin, *opened, 44)));
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, mtext->Common, mtext->Base);
    }
    CHECK(count == 1);
  }
}

TEST_CASE("entities/mtext: a default mtext runs left to right from its top left at single spacing")
{
  const MText mtext;
  CHECK(mtext.Direction == Vector{.X = 1, .Y = 0, .Z = 0});
  CHECK(mtext.Attachment == 1);
  CHECK(mtext.Flow == 1);
  CHECK(mtext.LineSpacingStyle == 1);
  CHECK(mtext.LineSpacingFactor == 1.0);
}
} // namespace DwgIO::Entities
