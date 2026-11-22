/***********************************************************************************************************************
 *  @file      tests/dwgio/store/store.cpp
 *  @brief     The R2013 data store: AutoCAD's read, bodies written back, and broken stores refused.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "store/entry.hpp"
#include "store/layout.hpp"
#include "store/read.hpp"
#include "store/write.hpp"
#include "support/fixtures.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/load.hpp"
#include "dwgio/section.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace DwgIO::Store
{
namespace
{
/**
 *  The data store of a fixture.
 */
auto Kept(const std::string &name) -> std::vector<std::byte>
{
  const Drawing drawing = Load(ReadFixture(name)).value().Drawing;
  const auto found = std::ranges::find(drawing.Kept, std::string{"AcDb:AcDsPrototype_1b"}, &Section::Name);
  REQUIRE(found != drawing.Kept.end());
  return found->Bytes;
}

auto Body(std::uint64_t handle, std::size_t size) -> Entry
{
  std::string data(size, '\0');
  for (std::size_t at = 0; at < size; ++at)
  {
    data[at] = static_cast<char>((at * 7) % 251);
  }
  return Entry{.Handle = handle, .Schema = std::string{Bodies}, .Data = data, .Ranges = {}};
}

/**
 *  A long written over four bytes of a store.
 */
void Patch(std::vector<std::byte> &store, std::size_t at, std::uint32_t value)
{
  for (std::size_t one = 0; one < 4; ++one)
  {
    store.at(at + one) = static_cast<std::byte>((value >> (8 * one)) & 0xFF);
  }
}
} // namespace

TEST_CASE("store/store: AutoCAD's stores read with bodies under the ACIS schema and the thumbnail under its own")
{
  for (const std::string name : {"kinds-r2013.dwg", "kinds-r2018.dwg", "kinds-r2000-as-r2013.dwg"})
  {
    CAPTURE(name);
    const std::expected<std::vector<Entry>, std::string> entries = Read(Kept(name));
    REQUIRE(entries);
    CHECK(std::ranges::count(*entries, std::string{Bodies}, &Entry::Schema) == 13);
    CHECK(std::ranges::count(*entries, std::string{"AcDb_Thumbnail_Schema"}, &Entry::Schema) == 1);
    for (const Entry &entry : *entries)
    {
      REQUIRE(entry.Ranges.size() == 1);
      CHECK(entry.Ranges.front().Size == entry.Data.size());
    }
  }
}

TEST_CASE("store/store: written bodies read back with a large one spread over blob segments")
{
  const std::vector<Entry> bodies{Body(0x2CE, 1000), Body(0x2A0, MostInline + 1), Body(0x300, (2 * MostPage) + 5)};
  const std::vector<std::byte> store = Write(bodies);
  CHECK(store.size() % Alignment == 0);
  const std::expected<std::vector<Entry>, std::string> entries = Read(store);
  REQUIRE(entries);
  REQUIRE(entries->size() == 3);
  for (std::size_t at = 0; at < bodies.size(); ++at)
  {
    CHECK(entries->at(at).Handle == bodies.at(at).Handle);
    CHECK(entries->at(at).Schema == Bodies);
    CHECK(entries->at(at).Data == bodies.at(at).Data);
  }
  CHECK(entries->at(1).Ranges.size() == 1);
  CHECK(entries->at(2).Ranges.size() == 3);
  // A store of no bodies still reads.
  const std::expected<std::vector<Entry>, std::string> none = Read(Write({}));
  REQUIRE(none);
  CHECK(none->empty());
}

TEST_CASE("store/store: a broken store is refused and a stub skipped and an unknown schema left unnamed")
{
  const std::vector<std::byte> store = Write(std::vector<Entry>{Body(0x10, 8), Body(0x11, 8)});
  CHECK_FALSE(Read(std::vector<std::byte>(store.begin(), store.begin() + 40)));
  CHECK_FALSE(Read(std::vector<std::byte>(16, std::byte{0})));
  // The data index is segment 2, after the head and the segment index of 0x100: its entries follow its header and
  // a count and a zero, each the segment, the offset of the record's header and the schema.
  constexpr std::size_t Index = HeadSize + 0x100 + HeaderSize + 8;
  std::vector<std::byte> stub = store;
  Patch(stub, Index, 0);
  Patch(stub, Index + 12 + 8, 99);
  const std::expected<std::vector<Entry>, std::string> entries = Read(stub);
  REQUIRE(entries);
  REQUIRE(entries->size() == 1);
  CHECK(entries->front().Handle == 0x11);
  CHECK(entries->front().Schema.empty());
  std::vector<std::byte> lost = store;
  Patch(lost, Index, 77);
  CHECK(Read(lost).error() == "data store not read past 0 records");
  // A schema index naming more schemas than it holds, and a blob of more pages than the store has.
  std::vector<std::byte> named = store;
  const std::string thumbnail = "AcDb_Thumbnail_Schema";
  const auto found = std::ranges::search(
      named, std::vector<std::byte>(reinterpret_cast<const std::byte *>(thumbnail.data()),
                                    reinterpret_cast<const std::byte *>(thumbnail.data() + thumbnail.size())));
  REQUIRE_FALSE(found.empty());
  Patch(named, static_cast<std::size_t>(found.begin() - named.begin()) - 4, 0x00FFFFFF);
  CHECK_FALSE(Read(named));
  std::vector<std::byte> paged = Write(std::vector<Entry>{Body(0x12, MostInline + 1)});
  const std::vector<std::byte> mark{std::byte{0xB1}, std::byte{0x6B}, std::byte{0x10}, std::byte{0xBB}};
  const auto blob = std::ranges::search(paged, mark);
  REQUIRE_FALSE(blob.empty());
  Patch(paged, static_cast<std::size_t>(blob.begin() - paged.begin()) + 4 + 8, 0x7FFFFFFF);
  CHECK_FALSE(Read(paged));
  std::vector<std::byte> bare = store;
  Patch(bare, 0, 0);
  CHECK(Read(bare).error() == "data store head not read");
}

TEST_CASE("store/store: a record whose pages overrun the store or repeat or overflow is refused")
{
  const std::vector<std::byte> store = Write(std::vector<Entry>{Body(0x12, MostPage + 1)});
  const std::expected<std::vector<Entry>, std::string> entries = Read(store);
  REQUIRE(entries);
  REQUIRE(entries->front().Ranges.size() == 2);
  // The page list follows the reference's mark, its size, its page count and four longs: each page its blob
  // segment's index and its size.
  const std::vector<std::byte> mark{std::byte{0xB1}, std::byte{0x6B}, std::byte{0x10}, std::byte{0xBB}};
  const auto reference = std::ranges::search(store, mark);
  REQUIRE_FALSE(reference.empty());
  const std::size_t list = static_cast<std::size_t>(reference.begin() - store.begin()) + 4 + 32;
  std::vector<std::byte> twice = store;
  Patch(twice, list + 8, 6); // The second page names the first's blob segment, whose page is nearly the store.
  CHECK_FALSE(Read(twice));
  // A page whose size, of eight bytes, is 2^61: its bits wrap to none.
  std::vector<std::byte> huge = store;
  const std::size_t size = entries->front().Ranges.front().Offset - 8;
  Patch(huge, size, 0);
  Patch(huge, size + 4, 0x20000000);
  CHECK_FALSE(Read(huge));
}
} // namespace DwgIO::Store
