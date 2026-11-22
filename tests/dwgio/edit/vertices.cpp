/***********************************************************************************************************************
 *  @file      tests/dwgio/edit/vertices.cpp
 *  @brief     Adding vertices to an old-style polyline or a mesh through Edit.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/edit/editor.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/flags.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <expected>
#include <format>

namespace DwgIO
{
TEST_CASE("edit/vertices: vertices join a polyline in its release's form and take its kind")
{
  for (const Version release : {Version::R14, Version::R2000, Version::R2004, Version::R2018})
  {
    CAPTURE(NameOf(release));
    Drawing drawing = Create(release);
    Entities::Polyline flat;
    const Handle open = *Edit::Add(drawing, Object{flat});
    Entities::Polyline spatial;
    spatial.Flags = std::to_underlying(Entities::PolylineFlags::Polyline3d);
    const Handle raised = *Edit::Add(drawing, Object{spatial});
    for (int at = 0; at < 3; ++at)
    {
      Entities::Vertex vertex;
      vertex.Position = Vector{.X = static_cast<double>(at), .Y = at * 2.0};
      REQUIRE(Edit::AddVertex(drawing, open, vertex));
      vertex.Position.Z = at;
      REQUIRE(Edit::AddVertex(drawing, raised, vertex));
    }
    const auto &first = Get<Entities::Polyline>(*Find(drawing, open));
    const auto &second = Get<Entities::Polyline>(*Find(drawing, raised));
    CHECK(first.SeqEnd);
    CHECK((release >= Version::R2004 ? first.Vertices.size() == 3 : first.First && first.Last));
    const Handle last = release >= Version::R2004 ? second.Vertices.back() : second.Last;
    CHECK(Has(Get<Entities::Vertex>(*Find(drawing, last)).Flags, Entities::VertexFlags::Polyline3d));
    CHECK(Audit::Check(drawing).Findings.empty());
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    QueueAudit(saved->Bytes, std::format("dwgio-vertex-{}.dwg", NameOf(release)));
  }
}

TEST_CASE("edit/vertices: a vertex of another kind or a missing or non-polyline head is refused")
{
  Drawing drawing = Create();
  const Handle open = *Edit::Add(drawing, Object{Entities::Polyline{}});
  const Handle line = *Edit::Add(drawing, Object{Entities::Line{}});
  Entities::Vertex mesh;
  mesh.Flags = std::to_underlying(Entities::VertexFlags::PolygonMesh);
  CHECK(Edit::AddVertex(drawing, open, mesh).error().Code == FailureCode::Refused);
  CHECK(Edit::AddVertex(drawing, line, Entities::Vertex{}).error().Code == FailureCode::Refused);
  CHECK(Edit::AddVertex(drawing, Handle{.Value = 0xFFFFF}, Entities::Vertex{}).error().Code == FailureCode::Missing);
  Entities::Polyline faces;
  faces.Flags = std::to_underlying(Entities::PolylineFlags::PolyfaceMesh);
  const Handle face = *Edit::Add(drawing, Object{faces});
  Entities::Vertex corner;
  CHECK(Edit::AddVertex(drawing, face, corner));
  Entities::Vertex record;
  record.Flags = std::to_underlying(Entities::VertexFlags::PolyfaceMesh);
  CHECK(Edit::AddVertex(drawing, face, record));
  Edit::Editor edits{drawing};
  CHECK(edits.AddVertex(open, Entities::Vertex{}));
}

TEST_CASE("edit/vertices: a polygon mesh's vertices take its flag and a polyface takes only vertices and faces")
{
  Drawing drawing = Create();
  Entities::Polyline grid;
  grid.Flags = std::to_underlying(Entities::PolylineFlags::PolygonMesh);
  const Handle mesh = *Edit::Add(drawing, Object{grid});
  const Handle added = *Edit::AddVertex(drawing, mesh, Entities::Vertex{});
  CHECK(Get<Entities::Vertex>(*Find(drawing, added)).Flags == std::to_underlying(Entities::VertexFlags::PolygonMesh));
  Entities::Polyline faces;
  faces.Flags = std::to_underlying(Entities::PolylineFlags::PolyfaceMesh);
  const Handle face = *Edit::Add(drawing, Object{faces});
  Entities::Vertex corner;
  corner.Flags =
      std::to_underlying(Entities::VertexFlags::PolygonMesh) | std::to_underlying(Entities::VertexFlags::PolyfaceMesh);
  CHECK(Edit::AddVertex(drawing, face, corner));
  Entities::Vertex raised;
  raised.Flags = std::to_underlying(Entities::VertexFlags::Polyline3d);
  CHECK(Edit::AddVertex(drawing, face, raised).error().Code == FailureCode::Refused);
}

TEST_CASE("edit/vertices: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
