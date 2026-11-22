/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/face.cpp
 *  @brief     The 3DFACE against its DXF twin, and written back as R2000.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/face.hpp"

#include "fields/context.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
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
TEST_CASE("entities/face: the fixture's 3D face matches the DXF twin with its invisible edge flagged")
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
      const auto *face3d = GetIf<Face3d>(&object);
      if (face3d == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(face3d->Base.Self));
      const std::optional<std::size_t> opened = Entity(twin, face3d->Base.Self);
      REQUIRE(opened);
      CHECK(TypeOf(object) == "3DFACE");
      for (std::size_t corner = 0; corner < 4; ++corner)
      {
        CHECK(Near(face3d->Corners.at(corner), ReadPoint(twin, *opened, 10 + static_cast<int>(corner))));
      }
      CHECK(face3d->Flags == ReadWhole(twin, *opened, 70));
      CHECK(face3d->Flags == 4);
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, face3d->Common, face3d->Base);
    }
    CHECK(count == 1);
  }
}

TEST_CASE("entities/face: a face above the plane with every edge visible round-trips as R2000")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  Face3d face;
  face.Base.Self = Handle{0x100};
  face.Common.Space = Space::Model;
  face.Common.Layer = Handle{0x10};
  face.Common.Lineweight = Lineweight::ByLayer;
  face.Corners = {Vector{.X = 1, .Y = 2, .Z = 3}, Vector{.X = 4, .Y = 5, .Z = 6}, Vector{.X = 7, .Y = 8, .Z = 9},
                  Vector{.X = 1, .Y = 2, .Z = 3}};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{face}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{face});
  CHECK(issues.empty());
}
} // namespace DwgIO::Entities
