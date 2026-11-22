/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/mline.cpp
 *  @brief     The MLINE against its DXF twin, written back, and its counts cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/mline.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/mline.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/cursor.hpp"
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
namespace
{
auto Point3(Cursor &cursor, int code) -> Vector
{
  Vector point;
  point.X = cursor.Real(code);
  point.Y = cursor.Real(code + 10);
  point.Z = cursor.Real(code + 20);
  return point;
}

void CheckMLine(const std::vector<Pair> &twin, std::size_t opened, const MLine &mline)
{
  Cursor cursor{twin, opened};
  cursor.Until(2);
  cursor.Take(2);
  CHECK(mline.Style == ParseHandle(cursor.Take(340)));
  CHECK(mline.Scale == doctest::Approx(cursor.Real(40)));
  CHECK(mline.Justification == cursor.Whole(70));
  CHECK(mline.Flags == cursor.Whole(71));
  REQUIRE(mline.Joints.size() == static_cast<std::size_t>(cursor.Whole(72)));
  CHECK(mline.Lines == cursor.Whole(73));
  CHECK(Near(mline.Start, Point3(cursor, 10)));
  CHECK(Near(mline.Normal, Point3(cursor, 210)));
  for (const Joint &joint : mline.Joints)
  {
    CHECK(Near(joint.Position, Point3(cursor, 11)));
    CHECK(Near(joint.Direction, Point3(cursor, 12)));
    CHECK(Near(joint.Miter, Point3(cursor, 13)));
    REQUIRE(joint.Strands.size() == static_cast<std::size_t>(mline.Lines));
    for (const Strand &strand : joint.Strands)
    {
      REQUIRE(strand.Spans.size() == static_cast<std::size_t>(cursor.Whole(74)));
      for (const double span : strand.Spans)
      {
        CHECK(span == doctest::Approx(cursor.Real(41)));
      }
      REQUIRE(strand.Fills.size() == static_cast<std::size_t>(cursor.Whole(75)));
      for (const double fill : strand.Fills)
      {
        CHECK(fill == doctest::Approx(cursor.Real(42)));
      }
    }
  }
}
} // namespace

TEST_CASE("entities/mline: the fixture's two multilines match the DXF twin joint by joint and strand by strand")
{
  const std::vector<Pair> twin = ReadTwin("curves-r2000.dxf");
  for (const std::string_view name : {"curves-r2000.dwg", "curves-r14.dwg"})
  {
    CAPTURE(name);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    std::size_t count = 0;
    std::size_t closed = 0;
    for (const Object &object : loaded->Drawing.Objects)
    {
      const auto *mline = GetIf<MLine>(&object);
      if (mline == nullptr)
      {
        continue;
      }
      ++count;
      CAPTURE(HexOf(mline->Base.Self));
      CHECK(TypeOf(object) == "MLINE");
      const std::optional<std::size_t> opened = Entity(twin, mline->Base.Self);
      REQUIRE(opened);
      CheckMLine(twin, *opened, *mline);
      closed += (static_cast<unsigned>(mline->Flags) & 2U) != 0 ? 1U : 0U;
      CHECK(mline->Lines == 2);
      CheckCommon(TwinPlace{.Twin = &twin, .At = *opened, .Drawing = &loaded->Drawing}, mline->Common, mline->Base);
    }
    CHECK(count == 2);
    CHECK(closed == 1);
  }
}

TEST_CASE("entities/mline: a multiline round-trips as R2000 with short joints padded to the style's lines")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  MLine mline;
  mline.Base.Self = Handle{0x100};
  mline.Common.Space = Space::Model;
  mline.Common.Layer = Handle{0x10};
  mline.Common.Lineweight = Lineweight::ByLayer;
  mline.Scale = 2;
  mline.Justification = 1;
  mline.Start = Vector{.X = 1, .Y = 2, .Z = 0};
  mline.Flags = 3;
  mline.Lines = 2;
  mline.Style = Handle{0x18};
  const Strand strand{.Spans = {0, 1.5}, .Fills = {0.5}};
  mline.Joints = {Joint{.Position = Vector{.X = 1, .Y = 2, .Z = 0},
                        .Direction = Vector{.X = 1, .Y = 0, .Z = 0},
                        .Miter = Vector{.X = 0, .Y = 1, .Z = 0},
                        .Strands = {strand, strand}},
                  Joint{.Position = Vector{.X = 5, .Y = 2, .Z = 0},
                        .Direction = Vector{.X = 0, .Y = 1, .Z = 0},
                        .Miter = Vector{.X = -1, .Y = 0, .Z = 0},
                        .Strands = {strand, Strand{}}}};
  std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{mline}, with), 0, reading);
  REQUIRE(back);
  CHECK(*back == Object{mline});
  mline.Joints.back().Strands.pop_back();
  back = Objects::ReadObject(Objects::WriteObject(Object{mline}, with), 0, reading);
  REQUIRE(back);
  const auto &padded = Get<MLine>(*back);
  REQUIRE(padded.Joints.back().Strands.size() == 2);
  CHECK(padded.Joints.back().Strands.back() == Strand{});
  CHECK(issues.empty());
}

TEST_CASE("entities/mline: a line count past a byte is written as 255 and noted with the strands cut to match")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin, .Issues = &issues};
  const Objects::FileContext reading{
      .Release = Version::R2000, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
  MLine mline;
  mline.Base.Self = Handle{0x100};
  mline.Common.Space = Space::Model;
  mline.Common.Layer = Handle{0x10};
  mline.Common.Lineweight = Lineweight::ByLayer;
  mline.Lines = 300;
  mline.Joints = {Joint{}};
  const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{mline}, with), 0, reading);
  REQUIRE(back);
  const auto &read = Get<MLine>(*back);
  CHECK(read.Lines == 255);
  CHECK(read.Joints.front().Strands.size() == 255);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "lines 300 out of range, 255 written");
}

TEST_CASE("entities/mline: a joint or span or fill count the data cannot hold stops the read")
{
  const Fields::Context with{.Release = Version::R2000, .Text = &Objects::Latin};
  const auto head = [](Bits::Writer &data) -> void
  {
    data.BD(1);
    data.RC(0);
    data.Point3(Vector{});
    data.Point3(Up);
    data.BS(1);
    data.RC(1);
  };
  for (const int cut : {0, 1, 2})
  {
    CAPTURE(cut);
    Bits::Writer data;
    head(data);
    data.BS(cut == 0 ? 30000 : 1);
    if (cut > 0)
    {
      data.Point3(Vector{});
      data.Point3(Vector{});
      data.Point3(Vector{});
      data.BS(cut == 1 ? 30000 : 0);
      if (cut == 2)
      {
        data.BS(30000);
      }
    }
    Bits::Reader dataIn{data.Bytes()};
    Fields::Input in{.In = &dataIn, .With = with};
    MLine mline;
    CHECK_FALSE(Fields::Read(Objects::Shape<MLine>::Layout(), mline, in));
  }
}
} // namespace DwgIO::Entities
