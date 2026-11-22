/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/untyped.cpp
 *  @brief     Untyped objects carried across releases where AutoCAD writes their class alike.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/untyped.hpp"

#include "bits/writer.hpp"
#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "releases/object.hpp"
#include "releases/omission.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/save.hpp"
#include "dwgio/symbol.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace DwgIO::Releases
{
namespace
{
auto Standard() -> const Encoding::Scheme &
{
  static const Encoding::Scheme scheme = Encoding::MakeScheme(Encoding::Standard());
  return scheme;
}

auto RunOf(const Bits::Writer &writer) -> BitString
{
  const std::span<const std::byte> bytes = writer.Bytes();
  return BitString{.Bytes = {bytes.begin(), bytes.end()}, .Size = writer.Position()};
}

/**
 *  An untyped object of a class, its own bits of a release.
 */
auto MakeUnknown(std::string_view name, Version release, const Bits::Writer &data, const Bits::Writer &strings)
    -> Unknown
{
  return Unknown{.Type = 500,
                 .Class = Symbol{std::string{name}},
                 .Entity = false,
                 .Base = {},
                 .Common = {},
                 .Bytes = {},
                 .Body = Opaque{.Release = release, .Data = RunOf(data), .Handles = {}, .Strings = RunOf(strings)}};
}

auto Carry(const Unknown &unknown, Version from, Version to) -> std::optional<Opaque>
{
  return Carry(unknown, CarryContext{.From = from, .To = to, .Text = &Standard()});
}

/**
 *  A grip location component as AutoCAD writes it up to R2004 or from R2007.
 */
auto Component(Version release) -> Unknown
{
  const bool wide = release >= Version::R2007;
  Bits::Writer data;
  Bits::Writer strings;
  data.BL(-1);
  data.BL(33);
  data.BL(377);
  data.BS(-9999);
  data.BL(3);
  data.BL(1);
  static_cast<void>(wide ? strings.TU("UpdatedX") : data.T("UpdatedX", Standard()));
  return MakeUnknown("BLOCKGRIPLOCATIONCOMPONENT", release, data, strings);
}
} // namespace

TEST_CASE("releases/untyped: a class AutoCAD writes alike in both releases converts unchanged")
{
  Bits::Writer data;
  data.BL(1);
  const Unknown graph = MakeUnknown("ACAD_EVALUATION_GRAPH", Version::R2018, data, Bits::Writer{});
  const std::optional<Opaque> down = Carry(graph, Version::R2018, Version::R14);
  REQUIRE(down);
  CHECK(down->Release == Version::R14);
  CHECK(down->Data == graph.Body->Data);
  // A table AutoCAD remakes at each save crosses within the releases that write it alike.
  CHECK(Carry(MakeUnknown("TABLECONTENT", Version::R2007, data, Bits::Writer{}), Version::R2007, Version::R2018));
  CHECK_FALSE(Carry(MakeUnknown("TABLECONTENT", Version::R2007, data, Bits::Writer{}), Version::R2007, Version::R2004));
  // A class that changes at a release crosses where it does not change.
  CHECK(Carry(MakeUnknown("BREAKDATA", Version::R14, data, Bits::Writer{}), Version::R14, Version::R2000));
  CHECK_FALSE(Carry(MakeUnknown("BREAKDATA", Version::R14, data, Bits::Writer{}), Version::R14, Version::R2004));
}

TEST_CASE("releases/untyped: string stream text of a class that keeps none does not convert across R2007")
{
  Bits::Writer strings;
  static_cast<void>(strings.TU("lost"));
  const Unknown graph = MakeUnknown("ACAD_EVALUATION_GRAPH", Version::R2010, Bits::Writer{}, strings);
  CHECK_FALSE(Carry(graph, Version::R2010, Version::R2004));
  CHECK(Carry(graph, Version::R2010, Version::R2018));
}

TEST_CASE("releases/untyped: a dynamic block's grip location converts across R2007 by its layout")
{
  const Unknown older = Component(Version::R2004);
  const Unknown newer = Component(Version::R2007);
  CHECK(Carry(older, Version::R2004, Version::R2007) == newer.Body);
  CHECK(Carry(newer, Version::R2007, Version::R2004) == older.Body);
}

TEST_CASE("releases/untyped: nothing converts without own bits or for a class AutoCAD's saves did not show")
{
  Unknown bare = Component(Version::R2004);
  CHECK_FALSE(Carry(bare, Version::R2007, Version::R2010));
  bare.Body.reset();
  CHECK_FALSE(Carry(bare, Version::R2004, Version::R2007));
  CHECK_FALSE(
      Carry(MakeUnknown("ACME_THING", Version::R2004, Bits::Writer{}, Bits::Writer{}), Version::R2004, Version::R2000));
  CHECK_FALSE(Carry(MakeUnknown("", Version::R2004, Bits::Writer{}, Bits::Writer{}), Version::R2004, Version::R2000));
}

TEST_CASE("releases/untyped: an action parameter converts across R2007 by its layout below R2013")
{
  const Drawing drawing = Load(ReadFixture("kinds-r2010.dwg")).value().Drawing;
  const auto &edge = Get<Unknown>(*Find(drawing, Handle{0x34D}));
  REQUIRE(edge.Class.View() == "ACDBASSOCEDGEACTIONPARAM");
  const std::optional<Opaque> downgraded = Carry(edge, Version::R2010, Version::R2004);
  REQUIRE(downgraded);
  CHECK(downgraded->Release == Version::R2004);
  CHECK(downgraded->Strings.Size == 0);
  CHECK_FALSE(Carry(edge, Version::R2010, Version::R2013));
}

TEST_CASE("releases/untyped: a conversion rebuilds an untyped object that converts and keeps one that does not")
{
  const Drawing drawing = Create(Version::R2004);
  Unknown component = Component(Version::R2004);
  component.Bytes = {std::byte{1}};
  const Translation upgraded = ConvertObject(Object{component}, drawing, Version::R2007);
  const auto &up = Get<Unknown>(upgraded.Object);
  CHECK(up.Bytes.empty());
  CHECK(up.Body == Component(Version::R2007).Body);
  CHECK(upgraded.Issues.empty());
  Unknown foreign = MakeUnknown("ACME_THING", Version::R2004, Bits::Writer{}, Bits::Writer{});
  foreign.Bytes = {std::byte{1}};
  CHECK(Get<Unknown>(ConvertObject(Object{foreign}, drawing, Version::R2007).Object) == foreign);
  // An untyped entity's common data cross as a typed entity's do; an object's are not looked at.
  foreign.Common.Color.Rgb = static_cast<int>(0xC2FF0000U);
  CHECK(ConvertObject(Object{foreign}, drawing, Version::R2000).Issues.empty());
  foreign.Entity = true;
  const Translation downgraded = ConvertObject(Object{foreign}, drawing, Version::R2000);
  REQUIRE(downgraded.Issues.size() == 1);
  CHECK(downgraded.Issues.front().What.starts_with("true color of"));
  foreign.Common.Color = {};
  foreign.Common.Lineweight = Lineweight::Mm018;
  const Unknown older = Get<Unknown>(ConvertObject(Object{foreign}, Create(Version::R2000), Version::R14).Object);
  CHECK(older.Common.Lineweight == std::nullopt); // R14 keeps it in extended data.
  CHECK(Get<Unknown>(ConvertObject(Object{older}, Create(Version::R14), Version::R2000).Object).Common.Lineweight ==
        Lineweight::Mm018);
  foreign.Common.Previous = Handle{0x10};
  CHECK(Get<Unknown>(ConvertObject(Object{foreign}, Create(Version::R2000), Version::R2004).Object).Common.Previous ==
        Handle{});
}

TEST_CASE("releases/untyped: an untyped object whose text overflows its count in the code page is dropped")
{
  // A grip location component of R2007 named with 5000 é: a byte each in ANSI 1252, the seven of \U+00E9 in ANSI
  // 932, which lacks it, past the 32767 a T holds.
  Bits::Writer data;
  Bits::Writer strings;
  data.BL(-1);
  data.BL(33);
  data.BL(377);
  data.BS(-9999);
  data.BL(3);
  data.BL(1);
  std::string name;
  for (int at = 0; at < 5000; ++at)
  {
    name += "é";
  }
  static_cast<void>(strings.TU(name));
  Unknown component = MakeUnknown("BLOCKGRIPLOCATIONCOMPONENT", Version::R2007, data, strings);
  component.Base.Self = Handle{0x4000};
  component.Bytes = {std::byte{1}};
  Drawing drawing = Create(Version::R2007);
  drawing.Objects.emplace_back(component);
  CHECK_FALSE(std::ranges::contains(CollectLeftOut(drawing, Version::R2004), Handle{0x4000}));
  drawing.Codepage = 38; // ANSI 932.
  CHECK(std::ranges::contains(CollectLeftOut(drawing, Version::R2004), Handle{0x4000}));
  // Its bytes, of R2007, are no R2004 object: the save writes them nowhere.
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2004});
  REQUIRE(saved);
  CHECK(std::ranges::any_of(saved->Issues,
                            [](const Issue &issue) { return issue.What.starts_with("object 4000 left out"); }));
}

TEST_CASE("releases/untyped: the associative network and an evaluation graph convert whole or not at all")
{
  Drawing drawing = Create(Version::R2004);
  const auto add = [&drawing](std::string_view name, std::uint64_t self, std::uint64_t owner) -> void
  {
    Unknown unknown = MakeUnknown(name, Version::R2004, Bits::Writer{}, Bits::Writer{});
    unknown.Base.Self = Handle{self};
    unknown.Base.Owner = Handle{owner};
    drawing.Objects.emplace_back(unknown);
  };
  add("ACDBASSOCNETWORK", 0x900, 0xC);
  add("ASSOCDIMDEPENDENCYBODY", 0x901, 0x900);
  add("ACAD_EVALUATION_GRAPH", 0x910, 0xC);
  add("BLOCKMOVEACTION", 0x911, 0x910);
  add("BLOCKMOVEGRIP", 0x912, 0x910);
  add("ACAD_EVALUATION_GRAPH", 0x920, 0xC);
  add("BLOCKMOVEACTION", 0x921, 0x920);
  CHECK(CollectTorn(drawing, std::vector<Handle>{}).empty());
  // One of the network out takes the whole of it; one object of a graph its graph and all the graph owns.
  CHECK(CollectTorn(drawing, std::vector{Handle{0x901}, Handle{0x912}}) ==
        std::vector{Handle{0x900}, Handle{0x901}, Handle{0x910}, Handle{0x911}, Handle{0x912}});
  CHECK(CollectTorn(drawing, std::vector{Handle{0x920}}) == std::vector{Handle{0x920}, Handle{0x921}});
  CHECK(KinOf(*Find(drawing, Handle{0x900}), drawing) == "associative network");
  CHECK(KinOf(*Find(drawing, Handle{0x920}), drawing) == "evaluation graph");
  CHECK(KinOf(*Find(drawing, Handle{0x921}), drawing) == "evaluation graph");
  CHECK(KinOf(drawing.Objects.front(), drawing).empty());
  add("ACAD_THING", 0x930, 0x999);
  CHECK(KinOf(drawing.Objects.back(), drawing).empty());
  // Saved as R2007, the move action's empty bits do not read by its layout: its graph is dropped with it, and says so.
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2007});
  REQUIRE(saved);
  CHECK(std::ranges::contains(saved->Issues,
                              "ACAD_EVALUATION_GRAPH 910 left out with its evaluation graph, which cannot be written "
                              "whole as R2007",
                              &Issue::What));
  CHECK(std::ranges::contains(saved->Issues,
                              "ACDBASSOCNETWORK 900 left out with its associative network, which cannot be written "
                              "whole as R2007",
                              &Issue::What));
}

TEST_CASE("releases/untyped: every fixture saved to another release keeps the untyped objects AutoCAD keeps")
{
  // The saves AutoCAD audits: down from R2018 across each boundary the table knows, up from R14, and both ways
  // across the text's move at R2007.
  constexpr std::array<std::pair<Version, Version>, 7> Audits{{{Version::R2018, Version::R14},
                                                               {Version::R2018, Version::R2004},
                                                               {Version::R2018, Version::R2007},
                                                               {Version::R2018, Version::R2010},
                                                               {Version::R14, Version::R2018},
                                                               {Version::R2004, Version::R2007},
                                                               {Version::R2007, Version::R2004}}};
  for (const std::string_view name : {"dimbreaks", "histories", "kinds", "dimensions", "mleaders", "objects", "table"})
  {
    for (std::size_t from = 0; from < FixtureReleases.size(); ++from)
    {
      const Drawing source =
          Load(ReadFixture(std::format("{}-{}.dwg", name, FixtureSuffixes.at(from)))).value().Drawing;
      for (std::size_t to = 0; to < FixtureReleases.size(); ++to)
      {
        if (from == to)
        {
          continue;
        }
        CAPTURE(std::format("{} {} to {}", name, FixtureSuffixes.at(from), FixtureSuffixes.at(to)));
        const Drawing acad = Load(ReadFixture(std::format("{}-{}.dwg", name, FixtureSuffixes.at(to)))).value().Drawing;
        const std::expected<Saved, Failure> saved = Save(source, SaveOptions{.Release = FixtureReleases.at(to)});
        REQUIRE(saved);
        const std::expected<Loaded, Failure> back = Load(saved->Bytes);
        REQUIRE(back);
        CHECK(back->Issues.empty());
        for (const Object &object : source.Objects)
        {
          const auto *unknown = GetIf<Unknown>(&object);
          const Object *theirs = unknown != nullptr ? Find(acad, unknown->Base.Self) : nullptr;
          const auto *their = theirs != nullptr ? GetIf<Unknown>(theirs) : nullptr;
          if (their == nullptr || their->Class != unknown->Class ||
              !Carry(*unknown, source.Release, FixtureReleases.at(to)))
          {
            continue;
          }
          CAPTURE(std::format("{} {}", unknown->Class.View(), HexOf(unknown->Base.Self)));
          const Object *ours = Find(back->Drawing, unknown->Base.Self);
          if (ours == nullptr)
          {
            // Left out only with the part of its network or graph that cannot cross.
            CHECK(std::ranges::any_of(
                saved->Issues, [unknown](const Issue &issue) -> bool
                { return issue.Subject == unknown->Base.Self && issue.What.contains("with its"); }));
            continue;
          }
          const auto *mine = GetIf<Unknown>(ours);
          REQUIRE(mine != nullptr);
          CHECK(mine->Body == their->Body);
          CHECK(mine->Base.Extended == their->Base.Extended);
        }
        const auto across = std::pair{FixtureReleases.at(from), FixtureReleases.at(to)};
        if (std::ranges::contains(Audits, across))
        {
          QueueAudit(saved->Bytes,
                     std::format("dwgio-{}-{}-carried-{}.dwg", name, FixtureSuffixes.at(from), FixtureSuffixes.at(to)));
        }
      }
    }
  }
}

TEST_CASE("releases/untyped: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
