/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/mesh.cpp
 *  @brief     The MESH against its DXF twin in every release, its overrides, and its counts cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/mesh.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/mesh.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"
#include "support/twin.hpp"

#include "dwgio/class.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/opaque.hpp"
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
 *  The mesh's subclass in the twin, read in its order: each count, then what it counts.
 */
struct Net
{
  std::vector<Vector> Vertices;
  std::vector<int> Faces;
  std::vector<int> Edges;
  std::vector<double> Creases;
  int Overridden{};
};

auto NetOf(const std::vector<Pair> &twin, std::size_t opened) -> Net
{
  Net net;
  std::size_t at = opened + 1;
  while (twin[at].Value != "AcDbSubDMesh") // R2000's proxy graphics size is a 92 group too.
  {
    ++at;
  }
  while (twin[at].Code != 92)
  {
    ++at;
  }
  const auto count = [&twin, &at]() -> std::size_t
  {
    return static_cast<std::size_t>(std::stoi(twin[at++].Value));
  };
  for (std::size_t left = count(); left > 0; --left, at += 3)
  {
    net.Vertices.push_back(
        Vector{.X = std::stod(twin[at].Value), .Y = std::stod(twin[at + 1].Value), .Z = std::stod(twin[at + 2].Value)});
  }
  for (std::size_t left = count(); left > 0; --left)
  {
    net.Faces.push_back(std::stoi(twin[at++].Value));
  }
  for (std::size_t left = 2 * count(); left > 0; --left)
  {
    net.Edges.push_back(std::stoi(twin[at++].Value));
  }
  for (std::size_t left = count(); left > 0; --left)
  {
    net.Creases.push_back(std::stod(twin[at++].Value));
  }
  net.Overridden = std::stoi(twin[at].Value);
  return net;
}

auto Context(const std::vector<Class> &classes, std::vector<Issue> &issues, Version release) -> Fields::Context
{
  return Fields::Context{.Release = release, .Text = &Objects::Latin, .Issues = &issues, .Classes = &classes};
}

auto Reading(const std::vector<Class> &classes, std::vector<Issue> &issues, Version release) -> Objects::FileContext
{
  return Objects::FileContext{.Release = release, .Text = &Objects::Latin, .Classes = classes, .Issues = &issues};
}

const std::vector<Class> Classes{Class{.Number = 500, .DxfName = "MESH", .ItemClass = 0x1F2}};
} // namespace

TEST_CASE("entities/mesh: the box mesh matches the DXF twin in every release")
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
      const auto *mesh = GetIf<Mesh>(&object);
      if (mesh == nullptr)
      {
        continue;
      }
      ++seen;
      CHECK(TypeOf(object) == "MESH");
      const std::optional<std::size_t> at = Entity(twin, mesh->Base.Self);
      REQUIRE(at);
      CheckCommon(TwinPlace{.Twin = &twin, .At = *at, .Drawing = &drawing}, mesh->Common, mesh->Base);
      CHECK(mesh->Version == ReadWhole(twin, *at, 71));
      CHECK(mesh->Blend == (ReadWhole(twin, *at, 72) == 1));
      CHECK(mesh->Level == ReadWhole(twin, *at, 91));
      const Net net = NetOf(twin, *at);
      REQUIRE(mesh->Vertices.size() == net.Vertices.size());
      for (std::size_t one = 0; one < net.Vertices.size(); ++one)
      {
        CHECK(Near(mesh->Vertices[one], net.Vertices[one]));
      }
      CHECK(mesh->Faces == net.Faces);
      CHECK(mesh->Edges == net.Edges);
      CHECK(mesh->Creases == net.Creases);
      CHECK(mesh->Overridden == net.Overridden);
      CHECK(mesh->Overrides == Opaque{});
    }
    CHECK(seen == 1);
  }
}

TEST_CASE("entities/mesh: overrides keep the file's bits and an edge index without its pair is not written")
{
  std::vector<Issue> issues;
  Mesh mesh;
  mesh.Base.Self = Handle{0x40};
  mesh.Base.Owner = Handle{0x1F};
  mesh.Common.Layer = Handle{0x10};
  mesh.Common.Lineweight = Lineweight::ByLayer;
  mesh.Vertices = {Vector{}, Vector{.X = 1}, Vector{.Y = 1}};
  mesh.Faces = {3, 0, 1, 2};
  mesh.Edges = {0, 1, 1, 2, 2, 0};
  mesh.Creases = {0, 0.5, 1};
  mesh.Overridden = 1;
  Bits::Writer bits;
  bits.BL(7);
  bits.B(true);
  mesh.Overrides =
      Opaque{.Release = Version::R2018,
             .Data = BitString{.Bytes = {bits.Bytes().begin(), bits.Bytes().end()}, .Size = bits.Position()},
             .Handles = {},
             .Strings = {}};
  std::optional<Object> back =
      Objects::ReadObject(Objects::WriteObject(Object{mesh}, Context(Classes, issues, Version::R2018)), 0,
                          Reading(Classes, issues, Version::R2018));
  REQUIRE(back);
  const Mesh &read = Get<Mesh>(*back);
  CHECK(read.Faces == mesh.Faces);
  CHECK(read.Edges == mesh.Edges);
  CHECK(read.Creases == mesh.Creases);
  CHECK(read.Overrides.Release == Version::R2018);
  REQUIRE(read.Overrides.Data.Size >= mesh.Overrides.Data.Size);
  CHECK(read.Overrides.Data.Bytes.front() == mesh.Overrides.Data.Bytes.front());
  CHECK(issues.empty());

  mesh.Overridden = 0;
  mesh.Edges.push_back(5);
  back = Objects::ReadObject(Objects::WriteObject(Object{mesh}, Context(Classes, issues, Version::R2000)), 0,
                             Reading(Classes, issues, Version::R2000));
  REQUIRE(back);
  CHECK(Get<Mesh>(*back).Edges == std::vector<int>{0, 1, 1, 2, 2, 0});
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "mesh edge index 5 without its pair not written");
}

TEST_CASE("entities/mesh: an edge count beyond the data stops the read")
{
  std::vector<Issue> issues;
  Bits::Writer data;
  data.BS(2);
  data.B(false);
  data.BL(0);
  data.BL(0);       // No vertices,
  data.BL(0);       // no faces,
  data.BL(1000000); // and a million edges in a few bits.
  const std::vector<std::byte> payload = data.Take();
  Bits::Reader in{payload};
  Fields::Input reading{.In = &in, .With = Context(Classes, issues, Version::R2000)};
  Mesh mesh;
  mesh.Edges = {4, 5};
  CHECK_FALSE(Fields::Read(Objects::Shape<Mesh>::Layout(), mesh, reading));
  CHECK(mesh.Edges.empty());
  // A negative count, and a count the data ends inside.
  for (const bool negative : {true, false})
  {
    Bits::Writer cut;
    cut.BS(2);
    cut.B(false);
    cut.BL(0);
    cut.BL(0);
    cut.BL(0);
    if (negative)
    {
      cut.BL(-1);
    }
    else
    {
      cut.Bits(0, 1); // The first bit of the count's two.
    }
    const std::vector<std::byte> bytes = cut.Take();
    Bits::Reader cutIn{bytes};
    Fields::Input shortly{.In = &cutIn, .With = Context(Classes, issues, Version::R2000)};
    Mesh other;
    other.Edges = {1, 2};
    CHECK_FALSE(Fields::Read(Objects::Shape<Mesh>::Layout(), other, shortly));
    CHECK(other.Edges.empty());
  }
}
} // namespace DwgIO::Entities
