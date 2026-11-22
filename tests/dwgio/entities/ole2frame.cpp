/***********************************************************************************************************************
 *  @file      tests/dwgio/entities/ole2frame.cpp
 *  @brief     The OLE2FRAME written back in every release, saved across them, and its data cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/entities/ole2frame.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "entities/shapes/ole2frame.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "objects/context.hpp"
#include "objects/read.hpp"
#include "objects/shape.hpp"
#include "objects/write.hpp"
#include "support/acad.hpp"
#include "support/entities.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <variant>
#include <vector>

namespace DwgIO::Entities
{
namespace
{
/**
 *  A frame of made-up data, what an OLE object's bytes stand for here; the mode and the byte after the data only from
 *  R2000.
 */
auto Made(Version release) -> Ole2Frame
{
  Ole2Frame frame;
  frame.Common.Space = Space::Model;
  frame.Common.Layer = Handle{0x10};
  frame.Version = 2;
  for (int at = 0; at < 300; ++at)
  {
    frame.Data.push_back(static_cast<std::byte>(at));
  }
  if (release >= Version::R2000)
  {
    frame.Mode = 1;
    frame.Unknown1 = 0x80;
  }
  return frame;
}

/**
 *  The frame's own fields, the rest left out.
 */
auto Own(const Ole2Frame &frame) -> Ole2Frame
{
  return Ole2Frame{.Base = {},
                   .Common = {},
                   .Version = frame.Version,
                   .Mode = frame.Mode,
                   .Data = frame.Data,
                   .Unknown1 = frame.Unknown1};
}
} // namespace

TEST_CASE("entities/ole2frame: a frame round-trips under its fixed type in every release")
{
  for (const Version release : FixtureReleases)
  {
    CAPTURE(NameOf(release));
    std::vector<Issue> issues;
    const Fields::Context with{.Release = release, .Text = &Objects::Latin, .Issues = &issues};
    const Objects::FileContext reading{.Release = release, .Text = &Objects::Latin, .Classes = {}, .Issues = &issues};
    Ole2Frame frame = Made(release);
    frame.Base.Self = Handle{0x100};
    const std::optional<Object> back = Objects::ReadObject(Objects::WriteObject(Object{frame}, with), 0, reading);
    REQUIRE(back);
    CHECK(TypeOf(*back) == "OLE2FRAME");
    CHECK(Own(Get<Ole2Frame>(*back)) == Own(frame));
    CHECK(issues.empty());
  }
}

TEST_CASE("entities/ole2frame: a frame made in each release saves and converts to the other releases")
{
  for (const Version release : FixtureReleases)
  {
    CAPTURE(NameOf(release));
    Drawing drawing = Create(release);
    const std::expected<Handle, Failure> added = Edit::Add(drawing, Object{Made(release)});
    REQUIRE(added);
    const std::expected<Saved, Failure> saved = Save(drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(back->Drawing == Load(Save(back->Drawing)->Bytes).value().Drawing);
    const auto *frame = GetIf<Ole2Frame>(Find(back->Drawing, *added));
    REQUIRE(frame != nullptr);
    CHECK(Own(*frame) == Own(Made(release)));
    QueueAudit(saved->Bytes, std::format("dwgio-ole2frame-{}.dwg", NameOf(release)));
    for (const Version other : FixtureReleases)
    {
      CAPTURE(NameOf(other));
      const std::expected<Saved, Failure> crossed = Save(back->Drawing, SaveOptions{.Release = other});
      REQUIRE(crossed);
      const std::expected<Loaded, Failure> again = Load(crossed->Bytes);
      REQUIRE(again);
      CHECK(again->Issues.empty());
      const auto *carried = GetIf<Ole2Frame>(Find(again->Drawing, *added));
      REQUIRE(carried != nullptr);
      CHECK(carried->Data == frame->Data);
      CHECK(carried->Mode == (release >= Version::R2000 && other >= Version::R2000 ? 1 : 0));
    }
  }
}

TEST_CASE("entities/ole2frame: a negative or oversized data size or missing data stops the read")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R14, .Text = &Objects::Latin, .Issues = &issues};
  const auto read = [&with](const Bits::Writer &data) -> bool
  {
    Bits::Reader in{data.Bytes()};
    Fields::Input reading{.In = &in, .With = with};
    Ole2Frame frame;
    return Fields::Read(Objects::Shape<Ole2Frame>::Layout(), frame, reading);
  };
  Bits::Writer big;
  big.BS(2);
  big.BL(1000);
  CHECK_FALSE(read(big));
  Bits::Writer negative;
  negative.BS(2);
  negative.BL(-1);
  CHECK_FALSE(read(negative));
  Bits::Writer sized;
  sized.BS(2);
  CHECK_FALSE(read(sized));
  REQUIRE(issues.size() == 2);
  CHECK(issues[0].What == "OLE data size 1000 out of range");
  CHECK(issues[1].What == "OLE data size -1 out of range");
}

TEST_CASE("entities/ole2frame: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Entities
