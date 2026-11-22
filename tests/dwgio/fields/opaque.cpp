/***********************************************************************************************************************
 *  @file      tests/dwgio/fields/opaque.cpp
 *  @brief     The own bits of an object kept from each stream and written back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "fields/opaque.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "fields/context.hpp"

#include "dwgio/issue.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

namespace DwgIO::Fields
{
TEST_CASE("fields/opaque: the rest of each stream is kept and written back with the data up to its end")
{
  Bits::Writer data;
  data.BL(7);
  data.Bits(0x5A, 7); // The bits kept: seven of them,
  data.Bits(0x3, 2);  // then two past the data's end.
  Bits::Writer handles;
  handles.Bits(0x1234, 13);
  Bits::Writer strings;
  strings.Bits(0xABCDE, 20);
  strings.Bits(0x1, 3); // Past the strings' end.
  const std::vector<std::byte> dataBytes = data.Take();
  const std::vector<std::byte> handleBytes = handles.Take();
  const std::vector<std::byte> stringBytes = strings.Take();
  Bits::Reader in{dataBytes};
  Bits::Reader handlesIn{handleBytes};
  Bits::Reader stringsIn{stringBytes};
  CHECK(in.BL() == 7);
  std::vector<Issue> issues;
  const Input reading{.In = &in,
                      .Handles = &handlesIn,
                      .Strings = &stringsIn,
                      .With = Context{.Release = Version::R2010, .Issues = &issues},
                      .DataEnd = in.Position() + 7,
                      .StringsEnd = 20};
  Opaque opaque;
  REQUIRE(ReadOpaque(opaque, reading));
  CHECK(opaque.Release == Version::R2010);
  CHECK(opaque.Data == BitString{.Bytes = {std::byte{0xB4}}, .Size = 7});
  CHECK(opaque.Handles.Size == 16); // To the object's end, its padding with it.
  CHECK(opaque.Strings.Size == 20);

  Bits::Writer out;
  Bits::Writer outHandles;
  Bits::Writer outStrings;
  const Output writing{.To = &out,
                       .Handles = &outHandles,
                       .Strings = &outStrings,
                       .With = Context{.Release = Version::R2010, .Issues = &issues}};
  REQUIRE(WriteOpaque(opaque, writing));
  CHECK(out.Position() == 7);
  CHECK(outHandles.Position() == 16);
  CHECK(outStrings.Position() == 20);
  Bits::Reader again{out.Bytes()};
  CHECK(again.Bits(7) == 0x5A);
  Bits::Reader handlesAgain{outHandles.Bytes()};
  CHECK(handlesAgain.Bits(13) == 0x1234);
  Bits::Reader stringsAgain{outStrings.Bytes()};
  CHECK(stringsAgain.Bits(20) == 0xABCDE);
  CHECK(issues.empty());
}

TEST_CASE("fields/opaque: without streams the data runs to its end and another release's bits are not written")
{
  Bits::Writer data;
  data.Bits(0xF0F0F, 20);
  const std::vector<std::byte> bytes = data.Take();
  Bits::Reader in{bytes};
  std::vector<Issue> issues;
  Opaque opaque;
  REQUIRE(ReadOpaque(opaque, Input{.In = &in, .With = Context{.Release = Version::R2000, .Issues = &issues}}));
  CHECK(opaque.Data.Size == 24); // To the reader's end: the whole bytes.
  CHECK(opaque.Handles.Size == 0);
  CHECK(opaque.Strings.Size == 0);
  Bits::Writer out;
  CHECK(WriteOpaque(opaque, Output{.To = &out, .With = Context{.Release = Version::R2000, .Issues = &issues}}));
  CHECK(out.Position() == 24);
  CHECK_FALSE(WriteOpaque(opaque, Output{.To = &out, .With = Context{.Release = Version::R2004, .Issues = &issues}}));
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "bits kept from another release not written");
  // A reader already past the end it is given keeps nothing.
  Bits::Reader past{bytes};
  past.Seek(16);
  CHECK(ReadRest(past, 8).Size == 0);
}
} // namespace DwgIO::Fields
