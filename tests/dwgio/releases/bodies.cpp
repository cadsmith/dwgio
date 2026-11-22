/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/bodies.cpp
 *  @brief     ACIS bodies saved across releases as AutoCAD writes them, histories with them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "acis/read.hpp"
#include "octets.hpp"
#include "releases/omission.hpp"
#include "store/entry.hpp"
#include "store/layout.hpp"
#include "store/read.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/geometry.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/section.hpp"
#include "dwgio/symbol.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The bodies of a drawing's ACIS entities by handle: in the object below R2013, in the data store from it.
 */
auto Bodies(const Drawing &drawing) -> std::map<std::uint64_t, std::string>
{
  std::map<std::uint64_t, std::string> bodies;
  for (const Object &object : drawing.Objects)
  {
    Visit(
        [&bodies](const auto &kind) -> void
        {
          if constexpr (requires { kind.Geometry; })
          {
            if (kind.Geometry.Body.Version != 0)
            {
              bodies[kind.Base.Self.Value] = TextOf(kind.Geometry.Body.Data);
            }
          }
        },
        object);
  }
  for (const Section &block : drawing.Kept)
  {
    if (block.Name == "AcDb:AcDsPrototype_1b")
    {
      const std::vector<Store::Entry> entries = Store::Read(block.Bytes).value();
      for (const Store::Entry &entry : entries)
      {
        if (entry.Schema == Store::Bodies)
        {
          bodies[entry.Handle] = entry.Data;
        }
      }
    }
  }
  return bodies;
}

/**
 *  A body with its save's date blotted out, which AutoCAD writes as it saves.
 */
auto Dateless(std::string data) -> std::string
{
  const bool binary = data.starts_with("ACIS BinaryFile") || data.starts_with("ASM BinaryFile");
  const auto model = Acis::Read(Entities::Acis{.Version = binary ? 2 : 1, .Textual = !binary, .Data = BytesOf(data)});
  REQUIRE(model);
  const std::string &date = model->Head.Date;
  if (const std::size_t at = data.find(date); !date.empty() && at != std::string::npos)
  {
    data.replace(at, date.size(), date.size(), '*');
  }
  return data;
}

/**
 *  The fixture holding AutoCAD's save of the kinds fixture `from` as `to`.
 *
 *  Saved to an older release, or upgraded from R2010, it is the target's own, since AutoCAD remakes no body (R2013 to
 *  R2018 keeps R2013's bodies); else AutoCAD's upgrade of the fixture, R14's being R2000's, which R14 upgrades to
 *  unchanged.
 */
auto Oracle(std::size_t from, std::size_t to) -> std::string
{
  if (to < from || FixtureReleases.at(from) >= Version::R2010)
  {
    return std::format("kinds-{}.dwg", FixtureSuffixes.at(from) == "r2013" && FixtureSuffixes.at(to) == "r2018"
                                           ? "r2013"
                                           : FixtureSuffixes.at(to));
  }
  const std::string_view source = FixtureSuffixes.at(from) == "r14" ? "r2000" : FixtureSuffixes.at(from);
  return source == FixtureSuffixes.at(to) ? std::format("kinds-{}.dwg", source)
                                          : std::format("kinds-{}-as-{}.dwg", source, FixtureSuffixes.at(to));
}
} // namespace

TEST_CASE("releases/bodies: every kinds fixture saved to another release keeps AutoCAD's bodies")
{
  for (std::size_t from = 0; from < FixtureSuffixes.size(); ++from)
  {
    const Drawing drawing = Load(ReadFixture(std::format("kinds-{}.dwg", FixtureSuffixes.at(from)))).value().Drawing;
    for (std::size_t to = 0; to < FixtureSuffixes.size(); ++to)
    {
      if (to == from)
      {
        continue;
      }
      CAPTURE(FixtureSuffixes.at(from));
      CAPTURE(FixtureSuffixes.at(to));
      const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = FixtureReleases.at(to)});
      REQUIRE(saved);
      for (const Issue &issue : saved->Issues)
      {
        CHECK(issue.What.find("body") == std::string::npos);
      }
      const std::expected<Loaded, Failure> back = Load(saved->Bytes);
      REQUIRE(back);
      CHECK(back->Issues.empty());
      const std::map<std::uint64_t, std::string> ours = Bodies(back->Drawing);
      const std::map<std::uint64_t, std::string> theirs = Bodies(Load(ReadFixture(Oracle(from, to))).value().Drawing);
      REQUIRE(ours.size() == theirs.size());
      REQUIRE(ours.size() == 13);
      for (const auto &[handle, body] : theirs)
      {
        CAPTURE(handle);
        REQUIRE(ours.contains(handle));
        CHECK(Dateless(ours.at(handle)) == Dateless(body));
      }
      QueueAudit(saved->Bytes,
                 std::format("dwgio-kinds-{}-as-{}.dwg", FixtureSuffixes.at(from), FixtureSuffixes.at(to)));
    }
  }
}

TEST_CASE("releases/bodies: the solids fixture converts to and from R2000 like AutoCAD's saves")
{
  for (std::size_t from = 0; from < FixtureSuffixes.size(); ++from)
  {
    const Drawing drawing = Load(ReadFixture(std::format("solids-{}.dwg", FixtureSuffixes.at(from)))).value().Drawing;
    for (std::size_t to = 0; to < FixtureSuffixes.size(); ++to)
    {
      if (to == from)
      {
        continue;
      }
      CAPTURE(FixtureSuffixes.at(from));
      CAPTURE(FixtureSuffixes.at(to));
      const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = FixtureReleases.at(to)});
      REQUIRE(saved);
      const std::expected<Loaded, Failure> back = Load(saved->Bytes);
      REQUIRE(back);
      CHECK(back->Issues.empty());
      const std::map<std::uint64_t, std::string> ours = Bodies(back->Drawing);
      const std::map<std::uint64_t, std::string> theirs =
          Bodies(Load(ReadFixture(std::format("solids-{}.dwg", FixtureSuffixes.at(to)))).value().Drawing);
      REQUIRE(ours.size() == theirs.size());
      // Saved to an older release, or upgraded across R2007 from below it, where AutoCAD gives each vertex a long it knows nothing
      // of, these bodies are the target's own.
      const bool alike = to < from || to <= 2 || from >= 3;
      for (const auto &[handle, body] : theirs)
      {
        CAPTURE(handle);
        REQUIRE(ours.contains(handle));
        if (alike && !(FixtureSuffixes.at(from) == "r2013" && FixtureSuffixes.at(to) == "r2018"))
        {
          CHECK(Dateless(ours.at(handle)) == Dateless(body));
        }
      }
      QueueAudit(saved->Bytes,
                 std::format("dwgio-solids-{}-as-{}.dwg", FixtureSuffixes.at(from), FixtureSuffixes.at(to)));
    }
  }
}

TEST_CASE("releases/bodies: a solid's history is dropped with an evaluation graph that cannot convert")
{
  Drawing drawing = Load(ReadFixture("solids-r2018.dwg")).value().Drawing;
  for (Object &object : drawing.Objects)
  {
    // History nodes of a class not found in AutoCAD's files, whose bits are not known in another release.
    if (auto *unknown = GetIf<Unknown>(&object); unknown != nullptr && unknown->Class.View().starts_with("ACSH_") &&
                                                 unknown->Class.View() != "ACSH_HISTORY_CLASS")
    {
      unknown->Class = Symbol{"ACSH_FILLET_CLASS"};
    }
  }
  const std::vector<Handle> leftOut = CollectLeftOut(drawing, Version::R2013);
  for (const std::uint64_t history : {0x2E5U, 0x2E9U, 0x2EDU})
  {
    CAPTURE(history);
    CHECK(std::ranges::binary_search(leftOut, history, {}, &Handle::Value));
    CHECK(Refusal(*Find(drawing, Handle{history}), drawing, Version::R2013) ==
          std::format("ACSH_HISTORY_CLASS {:X} left out with its evaluation graph, which cannot be written whole as "
                      "R2013",
                      history));
  }
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = Version::R2013});
  REQUIRE(saved);
  const Drawing back = Load(saved->Bytes).value().Drawing;
  for (const Object &object : back.Objects)
  {
    if (const auto *solid = GetIf<Entities::Solid3d>(&object); solid != nullptr && solid->History)
    {
      CHECK(Find(back, solid->History) != nullptr);
    }
  }
}

TEST_CASE("releases/bodies: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
