/***********************************************************************************************************************
 *  @file      tests/dwgio/objects/tablecontent.cpp
 *  @brief     Every object of the R14 and R2000 table fixtures written back byte for byte.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/objects/tablecontent.hpp"

#include "objects/read.hpp"
#include "objects/write.hpp"
#include "support/fixtures.hpp"
#include "support/reader.hpp"

#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/unknown.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Objects
{
TEST_CASE("objects/tablecontent: every object of the R14 and R2000 table fixtures writes back the file's bytes")
{
  for (const std::string_view name : {"cells-r14.dwg", "cells-r2000.dwg", "table-r14.dwg"})
  {
    CAPTURE(name);
    Fixture fixture = Open(name);
    std::string differing;
    for (const Mapped &entry : fixture.Entries)
    {
      const std::optional<Object> object = ReadObject(fixture.File, entry.Offset, fixture.Reading());
      REQUIRE_MESSAGE(object, HexOf(entry.Self));
      const std::vector<std::byte> written = WriteObject(*object, fixture.Context());
      REQUIRE(written.size() <= fixture.File.size() - entry.Offset);
      const std::span<const std::byte> kept = std::span{fixture.File}.subspan(entry.Offset, written.size());
      if (const auto [ours, theirs] = std::ranges::mismatch(written, kept); ours != written.end())
      {
        differing += std::format("{} {} at {} size {} ours {:02x} {:02x} {:02x} theirs {:02x} {:02x} {:02x}; ",
                                 HexOf(entry.Self), TypeOf(*object), ours - written.begin(), written.size(),
                                 std::to_integer<int>(ours[0]), std::to_integer<int>(ours[1]),
                                 std::to_integer<int>(ours[2]), std::to_integer<int>(theirs[0]),
                                 std::to_integer<int>(theirs[1]), std::to_integer<int>(theirs[2]));
      }
    }
    MESSAGE(differing);
    CHECK(differing.empty());
    CHECK(fixture.Issues.empty());
  }
}
} // namespace DwgIO::Objects
