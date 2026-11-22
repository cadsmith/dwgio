/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/ltype.cpp
 *  @brief     The LTYPE against its DXF twin, its dashes, shapes and texts, and cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/ltype.hpp"

#include "bits/reference.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Tables
{
namespace
{
constexpr int Upright = 1;

constexpr int TextDash = 2;

constexpr int ShapeDash = 4;

constexpr int UprightText = 8;

auto Ltypes(const Drawing &drawing) -> std::vector<const LType *>
{
  std::vector<const LType *> ltypes;
  for (const Object &object : drawing.Objects)
  {
    if (const auto *ltype = GetIf<LType>(&object))
    {
      ltypes.push_back(ltype);
    }
  }
  return ltypes;
}

auto FindByName(const std::vector<const LType *> &ltypes, std::string_view name) -> const LType &
{
  const auto found = std::ranges::find(ltypes, name, [](const LType *one) { return one->Record.Name; });
  REQUIRE(found != ltypes.end());
  return **found;
}

/**
 *  Every value of a group inside the record opened at that index, in order.
 */
auto Each(const std::vector<Pair> &twin, std::size_t opened, int code) -> std::vector<std::string>
{
  std::vector<std::string> values;
  for (std::size_t at = opened + 1; at < twin.size() && twin[at].Code != 0; ++at)
  {
    if (twin[at].Code == code)
    {
      values.push_back(twin[at].Value);
    }
  }
  return values;
}
} // namespace

TEST_CASE("tables/ltype: the fixture's linetypes carry their dashes and shapes and texts")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  // The STYLE record with that font, since the shape file's record has no name to open it by.
  const auto style = [&twin](std::string_view font) -> Handle
  {
    for (std::size_t at = 0; at < twin.size(); ++at)
    {
      if (twin[at].Code == 0 && twin[at].Value == "STYLE" && GroupOf(twin, at, 3) == font)
      {
        return Self(twin, at);
      }
    }
    FAIL("no style with that font");
    return {};
  };
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const std::vector<const LType *> ltypes = Ltypes(loaded->Drawing);
    REQUIRE(ltypes.size() == 8);
    const LType &dashed = FindByName(ltypes, "DASHED");
    REQUIRE(dashed.Dashes.size() == 2);
    CHECK(dashed.Dashes[0].Length == doctest::Approx(12.7));
    CHECK(dashed.Dashes[1].Length == doctest::Approx(-6.35));
    CHECK(dashed.Length == doctest::Approx(19.05));
    CHECK(dashed.Alignment == 'A');
    CHECK(dashed.Dashes[0].Style == Handle{});
    CHECK(dashed.Strings.size() == 256);
    const LType &gas = FindByName(ltypes, "GAS_LINE");
    REQUIRE(gas.Dashes.size() == 3);
    CHECK((gas.Dashes[1].Flags & TextDash) != 0);
    CHECK(gas.Dashes[1].Text == "GAS");
    CHECK(gas.Dashes[1].Style == style("Arial.ttf"));
    CHECK(gas.Dashes[1].Along == doctest::Approx(-2.54));
    CHECK(gas.Dashes[0].Text.empty());
    const LType &fence = FindByName(ltypes, "FENCELINE1");
    REQUIRE(fence.Dashes.size() == 4);
    CHECK((fence.Dashes[1].Flags & ShapeDash) != 0);
    CHECK(fence.Dashes[1].Shape == 133);
    CHECK(fence.Dashes[1].Style == style("ltypeshp.shx"));
    CHECK(fence.Dashes[1].Text.empty());
    CHECK(TypeOf(Object{fence}) == "LTYPE");
    for (const LType *ltype : ltypes)
    {
      CAPTURE(ltype->Record.Name);
      const std::optional<std::size_t> opened = FindOpeningNoCase(twin, "LTYPE", ltype->Record.Name);
      REQUIRE(opened);
      CHECK(ltype->Record.Base.Self == Self(twin, *opened));
      CHECK(ltype->Description == GroupOf(twin, *opened, 3));
      CHECK(ltype->Length == doctest::Approx(std::stod(GroupOf(twin, *opened, 40))));
      CHECK(ltype->Dashes.size() == static_cast<std::size_t>(std::stoi(GroupOf(twin, *opened, 73))));
      const std::vector<std::string> lengths = Each(twin, *opened, 49);
      const std::vector<std::string> flags = Each(twin, *opened, 74);
      REQUIRE(lengths.size() == ltype->Dashes.size());
      REQUIRE(flags.size() == ltype->Dashes.size());
      for (std::size_t at = 0; at < ltype->Dashes.size(); ++at)
      {
        CAPTURE(at);
        const Dash &dash = ltype->Dashes[at];
        CHECK(dash.Length == doctest::Approx(std::stod(lengths[at])));
        CHECK((dash.Flags & (Upright | TextDash | ShapeDash)) == std::stoi(flags[at]));
        CHECK((dash.Flags & ~(Upright | TextDash | ShapeDash | UprightText)) == 0);
        if ((dash.Flags & (ShapeDash | TextDash)) == 0)
        {
          CHECK(dash.Scale == 1.0);
          CHECK(dash.Style == Handle{});
          CHECK(dash.Text.empty());
        }
      }
    }
  }
}

TEST_CASE("tables/ltype: a text index beyond the strings area is noted and the text left empty")
{
  LType ltype;
  ltype.Record.Base.Self = Handle{0x2D2};
  ltype.Record.Name = "ODD";
  ltype.Dashes = {Dash{.Length = 1.0, .Shape = 300, .Flags = TextDash}, Dash{.Length = -1.0},
                  Dash{.Length = -1.0, .Shape = 250, .Flags = TextDash}};
  ltype.Strings.assign(256, std::byte{'x'});
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const std::vector<std::byte> bytes = Objects::WriteObject(Object{ltype}, with);
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> read = Objects::ReadObject(bytes, 0, reading);
  REQUIRE(read);
  const auto &back = Get<LType>(*read);
  CHECK(back.Dashes.size() == 3);
  CHECK(back.Dashes[0].Text.empty());
  CHECK(back.Dashes[2].Text == "xxxxxx");
  CHECK(back.Strings.size() == 256);
  LType bare;
  bare.Record.Base.Self = Handle{0x2D3};
  bare.Record.Name = "BARE";
  const std::optional<Object> again = Objects::ReadObject(Objects::WriteObject(Object{bare}, with), 0, reading);
  REQUIRE(again);
  CHECK(Get<LType>(*again).Strings == std::vector<std::byte>(256));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "dash text beyond the strings area");
  CHECK(issues.front().Subject == Handle{0x2D2});
}

TEST_CASE("tables/ltype: a linetype cut inside its dashes is kept with what was read and noted")
{
  const auto build = [](std::uint32_t handles, std::uint32_t *start) -> std::vector<std::byte>
  {
    Bits::Writer data;
    data.BS(57);
    data.RL(handles);
    data.H(Bits::Reference{.Code = 0, .Value = Handle{0x16}});
    data.BS(0);
    data.BL(0);
    data.TV("ODD", Version::R2000, Objects::Latin);
    data.B(false);
    data.BS(0);
    data.B(false);
    data.TV("", Version::R2000, Objects::Latin);
    data.BD(1.0);
    data.RC('A');
    data.RC(200);
    data.Fill();
    *start = static_cast<std::uint32_t>(data.Position());
    data.H(Bits::Reference{.Code = 4, .Value = Handle{0x5}});
    data.H(Bits::Reference{.Code = 3, .Value = Handle{}});
    data.H(Bits::Reference{.Code = 5, .Value = Handle{}});
    data.Fill();
    return Objects::FrameObject(data);
  };
  std::uint32_t start = 0;
  static_cast<void>(build(0, &start));
  std::vector<Issue> issues;
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> read = Objects::ReadObject(build(start, &start), 0, reading);
  REQUIRE(read);
  REQUIRE(Holds<LType>(*read));
  CHECK(Get<LType>(*read).Record.Name == "ODD");
  CHECK(Get<LType>(*read).Dashes.empty());
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "object 16 data cut short");
}
} // namespace DwgIO::Tables
