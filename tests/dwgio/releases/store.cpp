/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/store.cpp
 *  @brief     ACIS bodies moved between their entities and the R2013 data store.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/store.hpp"

#include "acis/model.hpp"
#include "acis/sab/writer.hpp"
#include "octets.hpp"
#include "releases/acis.hpp"
#include "store/entry.hpp"
#include "store/layout.hpp"
#include "store/read.hpp"
#include "store/write.hpp"
#include "support/fixtures.hpp"
#include "support/pass.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/geometry.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/section.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
auto Store(Drawing &drawing) -> std::vector<std::byte> &
{
  const auto found = std::ranges::find(drawing.Kept, std::string{"AcDb:AcDsPrototype_1b"}, &Section::Name);
  REQUIRE(found != drawing.Kept.end());
  return found->Bytes;
}

/**
 *  The bodies of a data store by handle.
 */
auto Bodies(const std::vector<std::byte> &store) -> std::map<std::uint64_t, std::string>
{
  std::map<std::uint64_t, std::string> bodies;
  const std::vector<DwgIO::Store::Entry> entries = DwgIO::Store::Read(store).value();
  for (const DwgIO::Store::Entry &entry : entries)
  {
    if (entry.Schema == DwgIO::Store::Bodies)
    {
      bodies[entry.Handle] = entry.Data;
    }
  }
  return bodies;
}
} // namespace

TEST_CASE("releases/store: below R2013 each body moves from the data store into its entity")
{
  Drawing drawing = LoadFixture("kinds-r2013.dwg").Drawing;
  CHECK_FALSE(Pass(TakeOutOfStore, drawing, Version::R2018));
  CHECK_FALSE(Pass(TakeOutOfStore, LoadFixture("kinds-r2010.dwg").Drawing, Version::R2004));
  const std::optional<Drawing> unstored = Pass(TakeOutOfStore, drawing, Version::R2010);
  REQUIRE(unstored);
  const std::map<std::uint64_t, std::string> bodies = Bodies(Store(drawing));
  std::size_t moved = 0;
  for (const Object &object : unstored->Objects)
  {
    if (const Entities::Geometry *geometry = GeometryOf(object))
    {
      CHECK(geometry->Body.Version == 2);
      CHECK(TextOf(geometry->Body.Data) == bodies.at(BaseOf(object).Self.Value));
      CHECK_FALSE(BaseOf(object).Stored);
      ++moved;
    }
  }
  CHECK(moved == 13);
  // An entity with a body of its own keeps it; one the store does not hold stays as it was.
  Drawing partial = drawing;
  GeometryOf(*Find(partial, Handle{0x2CE}))->Body = Entities::Acis{.Version = 1, .Textual = true, .Data = BytesOf("x")};
  BaseOf(*Find(partial, Handle{0x2D2})).Self = Handle{0x9999};
  BaseOf(*Find(partial, Handle{0x2FB})).Stored = false;
  const std::optional<Drawing> some = Pass(TakeOutOfStore, partial, Version::R2007);
  REQUIRE(some);
  CHECK(TextOf(GeometryOf(*Find(*some, Handle{0x2CE}))->Body.Data) == "x");
  CHECK(BaseOf(*Find(*some, Handle{0x9999})).Stored);
  CHECK(GeometryOf(*Find(*some, Handle{0x9999}))->Body.Version == 0);
  CHECK(GeometryOf(*Find(*some, Handle{0x2FB}))->Body.Version == 0);
  // A broken store gives nothing, nor does a drawing without one.
  Drawing broken = drawing;
  Store(broken).resize(16);
  CHECK_FALSE(Pass(TakeOutOfStore, broken, Version::R2010));
  Drawing none = drawing;
  std::erase_if(none.Kept, [](const Section &block) -> bool { return block.Name == "AcDb:AcDsPrototype_1b"; });
  CHECK_FALSE(Pass(TakeOutOfStore, none, Version::R2010));
}

TEST_CASE("releases/store: upgrading to R2013 puts each written body in the store")
{
  const Drawing drawing = LoadFixture("kinds-r2010.dwg").Drawing;
  CHECK_FALSE(MakeStore(drawing, Version::R2007, {}));
  CHECK_FALSE(MakeStore(LoadFixture("kinds-r2013.dwg").Drawing, Version::R2018, {}));
  const std::optional<std::vector<std::byte>> store = MakeStore(drawing, Version::R2013, {});
  REQUIRE(store);
  const std::map<std::uint64_t, std::string> bodies = Bodies(*store);
  CHECK(bodies.size() == 13);
  CHECK(bodies.at(0x2CE).starts_with("ACIS BinaryFile"));
  CHECK(Bodies(*MakeStore(drawing, Version::R2018, {})).at(0x2CE).starts_with("ASM BinaryFile4"));
  // An entity left out, or whose body cannot be written, is not in it; none at all makes no store.
  Drawing unwritten = drawing;
  GeometryOf(*Find(unwritten, Handle{0x2D2}))->Body.Data = BytesOf("broken");
  const std::vector<Handle> leftOut{Handle{0x2CE}};
  const std::map<std::uint64_t, std::string> fewer = Bodies(*MakeStore(unwritten, Version::R2013, leftOut));
  CHECK(fewer.size() == 11);
  CHECK_FALSE(fewer.contains(0x2CE));
  CHECK_FALSE(fewer.contains(0x2D2));
  Drawing empty = drawing;
  for (Object &object : empty.Objects)
  {
    if (Entities::Geometry *geometry = GeometryOf(object))
    {
      geometry->Body = {};
    }
  }
  CHECK_FALSE(MakeStore(empty, Version::R2013, {}));
}

TEST_CASE("releases/store: an R2018 store saved as R2013 keeps each body in place")
{
  Drawing newer = LoadFixture("kinds-r2018.dwg").Drawing;
  Drawing older = LoadFixture("kinds-r2013.dwg").Drawing;
  std::vector<Issue> issues;
  const std::vector<std::byte> restored = LowerStore(Store(newer), issues);
  CHECK(issues.empty());
  CHECK(restored.size() == Store(newer).size());
  CHECK(Bodies(restored) == Bodies(Store(older)));
  // An R2013 store, or a broken one, stays as it is.
  CHECK(LowerStore(Store(older), issues) == Store(older));
  const std::vector<std::byte> broken(32, std::byte{1});
  CHECK(LowerStore(broken, issues) == broken);
  CHECK(issues.empty());
  // A body of ACIS 22300 holding what cannot be told in 21800 stays, and is noted.
  Acis::Sab::Writer writer;
  writer.WriteHead(Acis::Head{.Release = Acis::Releases::R2018, .Bodies = 2, .Flags = 4, .Named = true});
  writer.Name("blend-surface");
  writer.End();
  const std::string unknown = writer.Finish();
  const std::vector<std::byte> store = DwgIO::Store::Write(std::vector<DwgIO::Store::Entry>{
      {.Handle = 0x10, .Schema = std::string{DwgIO::Store::Bodies}, .Data = unknown, .Ranges = {}}});
  CHECK(LowerStore(store, issues) == store);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "body of 10 kept as R2018 holds it");
  CHECK(issues.front().Subject == Handle{0x10});
}

TEST_CASE("releases/store: an ACIS 22300 body AutoCAD would resize for R2013 stays and is noted")
{
  // Its product's text behind two bytes of length, which a rewritten body holds behind one: a byte shorter.
  Acis::Sab::Writer writer;
  writer.WriteHead(
      Acis::Head{.Release = Acis::Releases::R2018, .Bodies = 2, .Flags = 4, .Named = true, .Product = "P"});
  writer.Name("asmheader");
  writer.Write(Acis::Spec{.Kind = Acis::Kind::Pointer}, Acis::Value{.Kind = Acis::Kind::Pointer, .Integer = -1});
  writer.Write(Acis::Spec{.Kind = Acis::Kind::Long}, Acis::Value{.Kind = Acis::Kind::Long, .Integer = -1});
  writer.Write(Acis::Spec{.Kind = Acis::Kind::Text}, Acis::Value{.Kind = Acis::Kind::Text, .Text = "231.0.0.65535"});
  writer.End();
  std::string body = writer.Finish();
  const std::size_t product = body.find(std::string{"\x07\x01P"});
  REQUIRE(product != std::string::npos);
  body.replace(product, 3, std::string{"\x08\x01\x00P", 4});
  const std::vector<std::byte> store = DwgIO::Store::Write(std::vector<DwgIO::Store::Entry>{
      {.Handle = 0x10, .Schema = std::string{DwgIO::Store::Bodies}, .Data = body, .Ranges = {}}});
  std::vector<Issue> issues;
  CHECK(LowerStore(store, issues) == store);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "body of 10 kept as R2018 holds it");
}
} // namespace DwgIO::Releases
