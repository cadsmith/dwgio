/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/streams.cpp
 *  @brief     The three streams of R2010 and later objects, written back byte for byte.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/crc.hpp"
#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "objects/map.hpp"
#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/reader.hpp"

#include "dwgio/common.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/streams: every object of the R2010 R2013 and R2018 fixtures writes back the section's bytes")
{
  for (const std::string_view year : {"2010", "2013", "2018"})
  {
    for (const std::string_view name : {"tables", "entities", "curves", "objects", "empty", "foundation"})
    {
      CAPTURE(name);
      CAPTURE(year);
      Fixture fixture = OpenPaged(std::format("{}-r{}.dwg", name, year));
      std::size_t typed = 0;
      std::string differing;
      for (const Mapped &entry : fixture.Entries)
      {
        const std::optional<Object> object = ReadObject(fixture.File, entry.Offset, fixture.Reading());
        REQUIRE_MESSAGE(object, HexOf(entry.Self));
        const std::vector<std::byte> written = WriteObject(*object, fixture.Context());
        REQUIRE(written.size() <= fixture.File.size() - entry.Offset);
        const std::span<const std::byte> kept = std::span{fixture.File}.subspan(entry.Offset, written.size());
        if (!std::ranges::equal(written, kept))
        {
          differing += std::format("{} {}; ", HexOf(entry.Self), TypeOf(*object));
        }
        typed += Holds<Unknown>(*object) ? 0U : 1U;
      }
      CHECK_MESSAGE(differing.empty(), differing);
      CHECK(typed >= 100);
      CHECK(fixture.Issues.empty());
    }
  }
}

namespace
{
/**
 *  An object written as R2018 and read back as it.
 */
auto RoundTrip(const Object &object, Version release = Version::R2018) -> Object
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues};
  const Objects::FileContext reading{.Release = release, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> back = ReadObject(WriteObject(object, with), 0, reading);
  REQUIRE(back);
  CHECK(issues.empty());
  return *back;
}

/**
 *  The bits of an R2010 object after its size and handle-stream size: the type, the handle, no extended data, the
 *  common data of an object with no reactors and no dictionary, then those bits.
 */
auto WriteOpening(int type, const Bits::Writer &rest) -> Bits::Writer
{
  Bits::Writer data;
  data.OT(type, Version::R2010);
  data.H(Bits::Reference{.Code = 0, .Value = Handle{0x10}});
  data.BS(0);
  data.BL(0);
  data.B(true);
  data.Append(rest);
  return data;
}

/**
 *  An object of R2010 framed by its size, the handle-stream size given and the CRC.
 */
auto FrameObject(const Bits::Writer &data, std::uint64_t handles) -> std::vector<std::byte>
{
  Bits::Writer whole;
  whole.MS(data.Bytes().size());
  whole.UMC(handles);
  whole.Raw(data.Bytes());
  whole.RS(Bits::Crc(whole.Bytes(), Bits::Seed));
  return whole.Take();
}
} // namespace

TEST_CASE("objects/streams: an MTEXT needing more than 0x7FFF bits of strings round-trips")
{
  Entities::MText mtext;
  mtext.Base.Self = Handle{0x40};
  mtext.Common.Layer = Handle{0x10};
  mtext.Common.Lineweight = Lineweight::ByLayer;
  mtext.Value = std::string(4000, 'w');
  mtext.Style = Handle{0x11};
  const Object back = RoundTrip(mtext);
  REQUIRE(Holds<Entities::MText>(back));
  CHECK(Get<Entities::MText>(back) == mtext);
}

TEST_CASE("objects/streams: an R2018 entity keeps its material and shadow and visual styles and data store bit")
{
  Entities::Line line;
  line.Base.Self = Handle{0x40};
  line.Base.Stored = true;
  line.Common.Layer = Handle{0x10};
  line.Common.Lineweight = Lineweight::ByLayer;
  line.Common.MaterialOrigin = Origin::Record;
  line.Common.Material = Handle{0x21};
  line.Common.ShadowFlags = 3;
  line.Common.Shadow = Handle{0x22};
  line.Common.FullStyle = Handle{0x23};
  line.Common.FaceStyle = Handle{0x24};
  line.Common.EdgeStyle = Handle{0x25};
  line.Common.Graphics = std::vector<std::byte>(3, std::byte{7});
  line.End.X = 4;
  const Object back = RoundTrip(line);
  REQUIRE(Holds<Entities::Line>(back));
  CHECK(Get<Entities::Line>(back) == line);
  // R2007 keeps the material and the shadow and drops what R2010 and R2013 added; R2004 none of them.
  Entities::Line older = line;
  older.Base.Stored = false;
  older.Common.FullStyle = Handle{};
  older.Common.FaceStyle = Handle{};
  older.Common.EdgeStyle = Handle{};
  CHECK(Get<Entities::Line>(RoundTrip(line, Version::R2007)) == older);
  older.Common.MaterialOrigin = Origin::ByLayer;
  older.Common.Material = Handle{};
  older.Common.ShadowFlags = 0;
  older.Common.Shadow = Handle{};
  CHECK(Get<Entities::Line>(RoundTrip(line, Version::R2004)) == older);
}

TEST_CASE("objects/streams: a handle stream size past the object's data is noted with the bytes kept")
{
  Bits::Writer rest;
  rest.B(false);
  const Bits::Writer data = WriteOpening(0x33, rest);
  const std::vector<std::byte> object = FrameObject(data, 8 * data.Bytes().size() + 9);
  std::vector<Issue> issues;
  const Objects::FileContext reading{.Release = Version::R2010, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> read = ReadObject(object, 0, reading);
  REQUIRE(read);
  REQUIRE(Holds<Unknown>(*read));
  CHECK(Get<Unknown>(*read).Bytes == object);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "object 10 handles beyond its data");
}

TEST_CASE("objects/streams: a string stream reaching before the object's data is noted and its text left empty")
{
  // A layer whose name and the rest are left out: the flag says there are strings, the size says more than there is.
  Bits::Writer rest;
  rest.RS(0x7000);
  rest.B(true);
  const Bits::Writer data = WriteOpening(0x33, rest);
  const std::vector<std::byte> object = FrameObject(data, 8 * data.Bytes().size() - data.Position());
  std::vector<Issue> issues;
  const Objects::FileContext reading{.Release = Version::R2010, .Text = &Latin, .Classes = {}, .Issues = &issues};
  const std::optional<Object> read = ReadObject(object, 0, reading);
  REQUIRE(read);
  REQUIRE_FALSE(issues.empty());
  CHECK(issues.front().What == "object 10 string stream before the data");
  REQUIRE(Holds<Tables::Layer>(*read));
  CHECK(Get<Tables::Layer>(*read).Record.Name.empty());
}
} // namespace DwgIO::Objects
