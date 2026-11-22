/***********************************************************************************************************************
 *  @file      tests/dwgio/file/paged/essential.cpp
 *  @brief     The sections AutoCAD refuses a paged file without, made as it writes them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/essential.hpp"

#include "support/fixtures.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/load.hpp"
#include "dwgio/section.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace DwgIO::File::Paged
{
namespace
{
/**
 *  A kept block of the empty R2004 fixture, by name.
 */
auto KeptBlock(const Drawing &drawing, std::string_view name) -> std::vector<std::byte>
{
  const auto block = std::ranges::find(drawing.Kept, name, &Section::Name);
  REQUIRE(block != drawing.Kept.end());
  return block->Bytes;
}

auto LongAt(std::span<const std::byte> bytes, std::size_t at) -> std::int32_t
{
  std::int32_t value = 0;
  for (std::size_t byte = 0; byte < 4; ++byte)
  {
    value |= static_cast<std::int32_t>(std::to_integer<std::uint32_t>(bytes[at + byte]) << (8 * byte));
  }
  return value;
}
} // namespace

TEST_CASE("file/paged/essential: the sections AutoCAD requires match those of AutoCAD's own file")
{
  // AutoCAD rejects an R2004 file lacking any of the three; it opens one lacking the summary, the application
  // information, the preview or the template.
  const Drawing drawing = Load(ReadFixture("empty-r2004.dwg")).value().Drawing;
  CHECK(WriteRevHistory() == KeptBlock(drawing, "AcDb:RevHistory"));
  CHECK(WriteObjFreeSpace(drawing.Header, Version::R2004, drawing.Objects.size()) ==
        KeptBlock(drawing, "AcDb:ObjFreeSpace"));
  CHECK(WriteTemplate(Version::R2004) == KeptBlock(drawing, "measurement"));
  const std::vector<std::byte> made = WriteAuxHeader(drawing.Header, Version::R2004, drawing.Maintenance);
  const std::vector<std::byte> theirs = KeptBlock(drawing, "auxiliary");
  REQUIRE(made.size() == theirs.size());
  REQUIRE(made.size() == 123);
  // The dates and the seed are the header's, as in AutoCAD's; the version code is the file's, where AutoCAD
  // writes its own (33, for 2026), and the saves count from one.
  CHECK(std::equal(made.begin() + 63, made.begin() + 79, theirs.begin() + 63));
  CHECK(LongAt(made, 79) == static_cast<std::int32_t>(drawing.Header.Handseed.Value));
  CHECK(LongAt(made, 79) == LongAt(theirs, 79));
  CHECK(std::to_integer<int>(made[3]) == 25);
  CHECK(std::to_integer<int>(made[5]) == drawing.Maintenance);
  CHECK(LongAt(made, 7) == 1);
  CHECK(LongAt(made, 103) == 1);
  Header vast = drawing.Header;
  vast.Handseed = Handle{.Value = 0x80000000};
  CHECK(LongAt(WriteAuxHeader(vast, Version::R2004, drawing.Maintenance), 79) == -1);
}

TEST_CASE("file/paged/essential: the made sections take each release's shape from R2010")
{
  for (const std::string_view name : {"empty-r2010.dwg", "empty-r2013.dwg", "empty-r2018.dwg"})
  {
    CAPTURE(name);
    const Drawing drawing = Load(ReadFixture(name)).value().Drawing;
    CHECK(WriteTemplate(drawing.Release) == KeptBlock(drawing, "measurement"));
    // The free space's second 64-bit value is near the object count, which AutoCAD counts its own way.
    std::vector<std::byte> made = WriteObjFreeSpace(drawing.Header, drawing.Release, drawing.Objects.size());
    std::vector<std::byte> theirs = KeptBlock(drawing, "AcDb:ObjFreeSpace");
    REQUIRE(made.size() == theirs.size());
    REQUIRE(made.size() == 89);
    CHECK(LongAt(made, 8) == static_cast<std::int32_t>(drawing.Objects.size()));
    std::fill_n(made.begin() + 8, 8, std::byte{0});
    std::fill_n(theirs.begin() + 8, 8, std::byte{0});
    CHECK(made == theirs);
    const std::vector<std::byte> auxiliary = WriteAuxHeader(drawing.Header, drawing.Release, drawing.Maintenance);
    const std::vector<std::byte> kept = KeptBlock(drawing, "auxiliary");
    REQUIRE(auxiliary.size() == kept.size());
    // The same layout as AutoCAD's: its dates where AutoCAD's are.
    const std::size_t dates = drawing.Release >= Version::R2018 ? 69 : 63;
    CHECK(std::equal(auxiliary.begin() + static_cast<std::ptrdiff_t>(dates),
                     auxiliary.begin() + static_cast<std::ptrdiff_t>(dates) + 16,
                     kept.begin() + static_cast<std::ptrdiff_t>(dates)));
  }
}
} // namespace DwgIO::File::Paged
