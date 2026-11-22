/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/common.cpp
 *  @brief     An object's common data: entity links, plot style flags and relative references.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/common.hpp"

#include "bits/reference.hpp"
#include "support/fixtures.hpp"
#include "support/twin.hpp"

#include "dwgio/entities/line.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/unknown.hpp"

#include <doctest/doctest.h>

#include <expected>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
namespace
{
auto ObjectOf(const Drawing &drawing, Handle self) -> const Object &
{
  for (const Object &object : drawing.Objects)
  {
    if (BaseOf(object).Self == self)
    {
      return object;
    }
  }
  FAIL("no such object");
  static const Object none{};
  return none;
}

auto CommonFound(const Drawing &drawing, Handle self) -> const Common &
{
  const Common *common = CommonOf(ObjectOf(drawing, self));
  REQUIRE(common);
  return *common;
}
} // namespace

TEST_CASE("objects/common: CommonOf gives an entity's common data typed or not and nothing for other objects")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("entities-r2000.dwg"));
  REQUIRE(loaded);
  const Object &line = ObjectOf(loaded->Drawing, Handle{0x2D3});
  REQUIRE(Holds<Entities::Line>(line));
  REQUIRE(CommonOf(line));
  CHECK(CommonOf(line)->Space == Space::Model);
  Unknown proxy;
  proxy.Entity = true;
  proxy.Common.Space = Space::Model;
  const Object untyped{proxy};
  REQUIRE(CommonOf(untyped));
  CHECK(CommonOf(untyped)->Space == Space::Model);
  proxy.Entity = false;
  CHECK(CommonOf(Object{proxy}) == nullptr);
  const Object &layer = ObjectOf(loaded->Drawing, Handle{0x10});
  CHECK(TypeOf(layer) == "LAYER");
  CHECK(CommonOf(layer) == nullptr);
  const Object &dictionary = ObjectOf(loaded->Drawing, Handle{0xC});
  CHECK(TypeOf(dictionary) == "DICTIONARY");
  CHECK(CommonOf(dictionary) == nullptr);
}

TEST_CASE("objects/common: a relative reference is the code AutoCAD writes for a soft pointer")
{
  CHECK(Bits::Relative(Handle{}, Handle{0x10}) == Bits::Reference{.Code = 4, .Value = Handle{}});
  CHECK(Bits::Relative(Handle{0x11}, Handle{0x10}) == Bits::Reference{.Code = 6, .Value = Handle{}});
  CHECK(Bits::Relative(Handle{0x0F}, Handle{0x10}) == Bits::Reference{.Code = 8, .Value = Handle{}});
  CHECK(Bits::Relative(Handle{0x15}, Handle{0x10}) == Bits::Reference{.Code = 0xA, .Value = Handle{5}});
  CHECK(Bits::Relative(Handle{0x0C}, Handle{0x10}) == Bits::Reference{.Code = 0xC, .Value = Handle{4}});
  // An offset only while it is smaller than the handle itself: AutoCAD's dictionaries, scales and layouts say so.
  CHECK(Bits::Relative(Handle{0x1CE}, Handle{0x323}) == Bits::Reference{.Code = 0xC, .Value = Handle{0x155}});
  CHECK(Bits::Relative(Handle{0x10C}, Handle{0x313}) == Bits::Reference{.Code = 4, .Value = Handle{0x10C}});
  CHECK(Bits::Relative(Handle{0x1A}, Handle{0x325}) == Bits::Reference{.Code = 4, .Value = Handle{0x1A}});
  CHECK(Bits::Relative(Handle{0x500}, Handle{0x10}) == Bits::Reference{.Code = 0xA, .Value = Handle{0x4F0}});
  CHECK(Bits::Relative(Handle{0x20}, Handle{0x100}) == Bits::Reference{.Code = 4, .Value = Handle{0x20}});
  for (const Handle target : {Handle{}, Handle{0x11}, Handle{0x0F}, Handle{0x15}, Handle{0x0C}})
  {
    CHECK(Bits::Resolve(Bits::Relative(target, Handle{0x10}), Handle{0x10}) == target);
  }
}

TEST_CASE("objects/common: the entity links are read whole with the assumed ones filled in")
{
  const std::expected<Loaded, Failure> foundation = Load(ReadFixture("foundation-r2000.dwg"));
  REQUIRE(foundation);
  const Common &line = CommonFound(foundation->Drawing, Handle{0x2CC});
  CHECK(line.Previous == Handle{});
  CHECK(line.Next == Handle{0x2CD});
  const Common &arc = CommonFound(foundation->Drawing, Handle{0x2CE});
  CHECK(arc.Previous == Handle{0x2CD});
  CHECK(arc.Next == Handle{0x2CF});
  const Common &polyline = CommonFound(foundation->Drawing, Handle{0x2CF});
  CHECK(polyline.Previous == Handle{0x2CE});
  CHECK(polyline.Next == Handle{});
  const std::expected<Loaded, Failure> older = Load(ReadFixture("foundation-r14.dwg"));
  REQUIRE(older);
  CHECK(CommonFound(older->Drawing, Handle{0x2CE}).Previous == Handle{0x2CD});
  CHECK(CommonFound(older->Drawing, Handle{0x2CE}).Next == Handle{0x2CF});
}

TEST_CASE("objects/common: the linetype and plot style flags say where the entity takes them from")
{
  const std::vector<Pair> twin = ReadTwin("entities-r2000.dxf");
  const std::optional<std::size_t> dashed = FindOpening(twin, "LTYPE", "DASHED");
  REQUIRE(dashed);
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("entities-r2000.dwg"));
  REQUIRE(loaded);
  const Common &named = CommonFound(loaded->Drawing, Handle{0x2F8});
  CHECK(named.LinetypeOrigin == Origin::Record);
  CHECK(named.Linetype == Self(twin, *dashed));
  CHECK(named.Lineweight == Lineweight::Mm050);
  const Common &block = CommonFound(loaded->Drawing, Handle{0x2F9});
  CHECK(block.LinetypeOrigin == Origin::ByBlock);
  CHECK(block.Linetype == Handle{});
  const Common &plain = CommonFound(loaded->Drawing, Handle{0x2FA});
  CHECK(plain.LinetypeOrigin == Origin::Continuous);
  const Common &layered = CommonFound(loaded->Drawing, Handle{0x2D3});
  CHECK(layered.LinetypeOrigin == Origin::ByLayer);
  CHECK(layered.PlotstyleOrigin == Origin::ByLayer);
  CHECK(layered.Plotstyle == Handle{});
  CHECK(layered.Graphics.empty());
  CHECK(BaseOf(ObjectOf(loaded->Drawing, Handle{0x2D3})).Reactors.size() == 1);
  const std::expected<Loaded, Failure> tables = Load(ReadFixture("tables-r2000.dwg"));
  REQUIRE(tables);
  CHECK(BaseOf(ObjectOf(tables->Drawing, Handle{0x2EB})).Owner == Handle{0x2EA});
}
} // namespace DwgIO::Objects
