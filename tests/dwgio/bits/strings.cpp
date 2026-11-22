/***********************************************************************************************************************
 *  @file      tests/dwgio/bits/strings.cpp
 *  @brief     Finding the string stream of an R2007 object back from its end, and refusing bad sizes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/strings.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>

namespace DwgIO::Bits
{
TEST_CASE("bits/strings: the tail of no strings is one clear bit and finds nothing")
{
  Writer data;
  data.BS(7);
  data.Append(WriteStringTail(Writer{}));
  CHECK(data.Position() == 11);
  const std::expected<std::optional<Strung>, std::string> found = FindStrings(data.Bytes(), data.Position());
  REQUIRE(found);
  CHECK_FALSE(*found);
  CHECK_FALSE(MakeStringReader(data.Bytes(), *found));
  const std::expected<std::optional<Strung>, std::string> none = FindStrings(data.Bytes(), 0);
  REQUIRE(none);
  CHECK_FALSE(*none);
}

TEST_CASE("bits/strings: a stream is found from the bit before the handles with its size in one short")
{
  Writer strings;
  static_cast<void>(strings.TU("Hi"));
  static_cast<void>(strings.TU("there"));
  Writer data;
  data.BS(300);
  data.Append(WriteStringTail(strings));
  const std::size_t handles = data.Position();
  CHECK(handles == 18 + strings.Position() + 16 + 1);
  const std::expected<std::optional<Strung>, std::string> found = FindStrings(data.Bytes(), handles);
  REQUIRE(found);
  REQUIRE(*found);
  CHECK(**found == Strung{.Start = 18, .End = 18 + strings.Position()});
  std::optional<Reader> reader = MakeStringReader(data.Bytes(), *found);
  REQUIRE(reader);
  CHECK(reader->TU() == "Hi");
  CHECK(reader->TU() == "there");
}

TEST_CASE("bits/strings: a stream of more than 0x7FFF bits takes a second short for its high bits")
{
  Writer strings;
  static_cast<void>(strings.TU(std::string(3000, 'x')));
  REQUIRE(strings.Position() > 0x7FFF);
  Writer data;
  data.B(true);
  data.Append(WriteStringTail(strings));
  const std::size_t handles = data.Position();
  CHECK(handles == 1 + strings.Position() + 32 + 1);
  const std::expected<std::optional<Strung>, std::string> found = FindStrings(data.Bytes(), handles);
  REQUIRE(found);
  REQUIRE(*found);
  CHECK(**found == Strung{.Start = 1, .End = 1 + strings.Position()});
  CHECK(MakeStringReader(data.Bytes(), *found)->TU() == std::string(3000, 'x'));
}

TEST_CASE("bits/strings: sizes reaching back past the first bit are refused")
{
  // The flag with too few bits before it for the size.
  Writer flag;
  flag.Bits(0, 5);
  flag.B(true);
  CHECK(FindStrings(flag.Bytes(), flag.Position()).error() == "string stream size before the data");
  // A size larger than the bits before it.
  Writer big;
  big.RS(500);
  big.B(true);
  CHECK(FindStrings(big.Bytes(), big.Position()).error() == "string stream before the data");
  // A size with the high bit and no room for its second short.
  Writer high;
  high.Bits(0, 4);
  high.RS(0x8001);
  high.B(true);
  CHECK(FindStrings(high.Bytes(), high.Position()).error() == "string stream size before the data");
}
} // namespace DwgIO::Bits
