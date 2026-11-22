/***********************************************************************************************************************
 *  @file      tests/dwgio/tables/dimstyle.cpp
 *  @brief     The DIMSTYLE against its DXF twin, every variable it writes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/tables/dimstyle.hpp"

#include "support/fixtures.hpp"
#include "support/twin.hpp"

#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Tables
{
namespace
{
/**
 *  A DXF group of a DIMSTYLE and the member that holds it; the twin writes only the groups off their defaults.
 */
template <typename ValueT>
struct Group
{
  int Code{};
  ValueT (*Get)(const DimStyle &one){nullptr};
  bool Modern{};
};

const std::array<Group<double>, 38> Wholes{
    Group<double>{.Code = 71,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtol); },
                  .Modern = false},
    Group<double>{.Code = 72,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimlim); },
                  .Modern = false},
    Group<double>{.Code = 73,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtih); },
                  .Modern = false},
    Group<double>{.Code = 74,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtoh); },
                  .Modern = false},
    Group<double>{.Code = 75,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimse1); },
                  .Modern = false},
    Group<double>{.Code = 76,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimse2); },
                  .Modern = false},
    Group<double>{.Code = 77,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtad); },
                  .Modern = false},
    Group<double>{.Code = 78,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimzin); },
                  .Modern = false},
    Group<double>{.Code = 79,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimazin); },
                  .Modern = true},
    Group<double>{.Code = 170,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimalt); },
                  .Modern = false},
    Group<double>{.Code = 171,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimaltd); },
                  .Modern = false},
    Group<double>{.Code = 172,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtofl); },
                  .Modern = false},
    Group<double>{.Code = 173,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimsah); },
                  .Modern = false},
    Group<double>{.Code = 174,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtix); },
                  .Modern = false},
    Group<double>{.Code = 175,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimsoxd); },
                  .Modern = false},
    Group<double>{.Code = 176,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimclrd.Index); },
                  .Modern = false},
    Group<double>{.Code = 177,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimclre.Index); },
                  .Modern = false},
    Group<double>{.Code = 178,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimclrt.Index); },
                  .Modern = false},
    Group<double>{.Code = 179,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimadec); },
                  .Modern = true},
    Group<double>{.Code = 271,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimdec); },
                  .Modern = false},
    Group<double>{.Code = 272,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtdec); },
                  .Modern = false},
    Group<double>{.Code = 273,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimaltu); },
                  .Modern = false},
    Group<double>{.Code = 274,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimalttd); },
                  .Modern = false},
    Group<double>{.Code = 275,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimaunit); },
                  .Modern = false},
    Group<double>{.Code = 276,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimfrac); },
                  .Modern = true},
    Group<double>{.Code = 277,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimlunit); },
                  .Modern = true},
    Group<double>{.Code = 278,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimdsep); },
                  .Modern = true},
    Group<double>{.Code = 279,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtmove); },
                  .Modern = true},
    Group<double>{.Code = 280,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimjust); },
                  .Modern = false},
    Group<double>{.Code = 281,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimsd1); },
                  .Modern = false},
    Group<double>{.Code = 282,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimsd2); },
                  .Modern = false},
    Group<double>{.Code = 283,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtolj); },
                  .Modern = false},
    Group<double>{.Code = 284,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtzin); },
                  .Modern = false},
    Group<double>{.Code = 285,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimaltz); },
                  .Modern = false},
    Group<double>{.Code = 286,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimalttz); },
                  .Modern = false},
    Group<double>{.Code = 288,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimupt); },
                  .Modern = false},
    Group<double>{.Code = 371,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimlwd); },
                  .Modern = true},
    Group<double>{.Code = 372,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimlwe); },
                  .Modern = true}};

const std::array<Group<double>, 18> Reals{
    Group<double>{.Code = 40,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimscale); },
                  .Modern = false},
    Group<double>{.Code = 41,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimasz); },
                  .Modern = false},
    Group<double>{.Code = 42,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimexo); },
                  .Modern = false},
    Group<double>{.Code = 43,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimdli); },
                  .Modern = false},
    Group<double>{.Code = 44,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimexe); },
                  .Modern = false},
    Group<double>{.Code = 45,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimrnd); },
                  .Modern = false},
    Group<double>{.Code = 46,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimdle); },
                  .Modern = false},
    Group<double>{.Code = 47,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtp); },
                  .Modern = false},
    Group<double>{.Code = 48,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtm); },
                  .Modern = false},
    Group<double>{.Code = 140,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtxt); },
                  .Modern = false},
    Group<double>{.Code = 141,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimcen); },
                  .Modern = false},
    Group<double>{.Code = 142,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtsz); },
                  .Modern = false},
    Group<double>{.Code = 143,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimaltf); },
                  .Modern = false},
    Group<double>{.Code = 144,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimlfac); },
                  .Modern = false},
    Group<double>{.Code = 145,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtvp); },
                  .Modern = false},
    Group<double>{.Code = 146,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimtfac); },
                  .Modern = false},
    Group<double>{.Code = 147,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimgap); },
                  .Modern = false},
    Group<double>{.Code = 148,
                  .Get = [](const DimStyle &one) -> double { return static_cast<double>(one.Dimaltrnd); },
                  .Modern = true}};

const std::array<Group<Handle>, 5> Handles{
    Group<Handle>{.Code = 340, .Get = [](const DimStyle &one) -> Handle { return (one.Dimtxsty); }, .Modern = false},
    Group<Handle>{.Code = 341, .Get = [](const DimStyle &one) -> Handle { return (one.Dimldrblk); }, .Modern = true},
    Group<Handle>{.Code = 342, .Get = [](const DimStyle &one) -> Handle { return (one.Dimblk); }, .Modern = true},
    Group<Handle>{.Code = 343, .Get = [](const DimStyle &one) -> Handle { return (one.Dimblk1); }, .Modern = true},
    Group<Handle>{.Code = 344, .Get = [](const DimStyle &one) -> Handle { return (one.Dimblk2); }, .Modern = true}};
} // namespace

TEST_CASE("tables/dimstyle: the fixture's dimension styles carry every variable the DXF twin writes")
{
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  for (const std::string_view name : {"tables-r2000.dwg", "tables-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const bool modern = loaded->Drawing.Release >= Version::R2000;
    std::size_t count = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *style = GetIf<DimStyle>(&object);
      if (style == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(style->Record.Name);
      CHECK(TypeOf(object) == "DIMSTYLE");
      const std::optional<std::size_t> opened = FindOpeningNoCase(twin, "DIMSTYLE", style->Record.Name);
      REQUIRE(opened);
      CHECK(style->Record.Base.Self == Self(twin, *opened));
      CHECK_FALSE(style->Unknown1);
      for (const Group<double> &group : Wholes)
      {
        const std::string value = GroupOf(twin, *opened, group.Code);
        if (value.empty() || (group.Modern && !modern))
        {
          continue;
        }
        CAPTURE(group.Code);
        CHECK(group.Get(*style) == std::stod(value));
      }
      for (const Group<double> &group : Reals)
      {
        const std::string value = GroupOf(twin, *opened, group.Code);
        if (value.empty() || (group.Modern && !modern))
        {
          continue;
        }
        CAPTURE(group.Code);
        CHECK(group.Get(*style) == doctest::Approx(std::stod(value)));
      }
      for (const Group<Handle> &group : Handles)
      {
        if (group.Modern && !modern)
        {
          continue;
        }
        CAPTURE(group.Code);
        CHECK(group.Get(*style) == ParseHandle(GroupOf(twin, *opened, group.Code)).value_or(Handle{}));
      }
      CHECK(style->Dimpost == GroupOf(twin, *opened, 3));
      CHECK(style->Dimapost == GroupOf(twin, *opened, 4));
      if (!modern && style->Record.Name == "METRIC")
      {
        // R14 names the built-in arrow without the underscore the block record carries.
        CHECK(Upper(style->Arrows.Dimblk) == "DOT");
      }
    }
    CHECK(count == 4);
  }
}

TEST_CASE("tables/dimstyle: the metric style scales by two with 3.5 high text and a dot arrow")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("tables-r2000.dwg"));
  REQUIRE(loaded);
  const std::vector<Pair> twin = ReadTwin("tables-r2000.dxf");
  const std::optional<std::size_t> dot = FindOpening(twin, "BLOCK_RECORD", "_Dot");
  REQUIRE(dot);
  for (const Object &object : loaded->Drawing.Objects)
  {
    const auto *style = GetIf<DimStyle>(&object);
    if (style == nullptr || style->Record.Name != "Metric")
    {
      continue;
    }
    CHECK(style->Dimscale == 2.0);
    CHECK(style->Dimtxt == 3.5);
    CHECK(style->Dimblk == Self(twin, *dot));
    CHECK(style->Dimblk1 == Handle{});
    CHECK(style->Dimtad == 1);
    CHECK(style->Dimdsep == ',');
    return;
  }
  FAIL("no Metric style");
}
} // namespace DwgIO::Tables
