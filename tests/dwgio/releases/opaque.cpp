/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/opaque.cpp
 *  @brief     Own bits recast from one release's layout to another's, and layouts that do not fit.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/opaque.hpp"

#include "bits/reader.hpp"
#include "bits/reference.hpp"
#include "bits/writer.hpp"
#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"

#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
auto RunOf(const Bits::Writer &writer) -> BitString
{
  const std::span<const std::byte> bytes = writer.Bytes();
  return BitString{.Bytes = {bytes.begin(), bytes.end()}, .Size = writer.Position()};
}

auto Standard() -> const Encoding::Scheme &
{
  static const Encoding::Scheme scheme = Encoding::MakeScheme(Encoding::Standard());
  return scheme;
}

/**
 *  The own bits of a release from its data and strings.
 */
auto Own(Version release, const Bits::Writer &data, const Bits::Writer &strings) -> Opaque
{
  Bits::Writer handles;
  handles.Bits(0x5, 3);
  return Opaque{.Release = release, .Data = RunOf(data), .Handles = RunOf(handles), .Strings = RunOf(strings)};
}

auto Recast(const Opaque &own, std::string_view layout, Version to) -> std::optional<Opaque>
{
  return DwgIO::Releases::Recast(own, layout, RecastContext{.To = to, .Text = &Standard()});
}

constexpr std::string_view Element = "BL BL BL X BL T E BL";

/**
 *  A block element as AutoCAD writes it up to R2004 or from R2007.
 */
auto ElementIn(Version release) -> Opaque
{
  const bool wide = release >= Version::R2007;
  Bits::Writer data;
  Bits::Writer strings;
  data.BL(-1);
  data.BL(33);
  data.BL(377);
  data.BS(-9999);
  data.BL(5);
  static_cast<void>(wide ? strings.TU("Grip") : data.T("Grip", Standard()));
  data.BL(wide ? 33 : 25);
  data.BL(wide ? 377 : 104);
  data.BL(0);
  return Own(release, data, strings);
}
} // namespace

TEST_CASE("releases/opaque: a block element converts across R2007 and back")
{
  const Opaque older = ElementIn(Version::R2004);
  const Opaque newer = ElementIn(Version::R2007);
  const std::optional<Opaque> upgraded = Recast(older, Element, Version::R2007);
  REQUIRE(upgraded);
  CHECK(*upgraded == newer);
  const std::optional<Opaque> downgraded = Recast(newer, Element, Version::R2004);
  REQUIRE(downgraded);
  CHECK(*downgraded == older);
  // Within the releases that write it alike, the bits stay, but for the release they are of.
  Opaque later = newer;
  later.Release = Version::R2018;
  CHECK(Recast(newer, Element, Version::R2018) == later);
  Opaque earliest = older;
  earliest.Release = Version::R14;
  CHECK(Recast(older, Element, Version::R14) == earliest);
}

TEST_CASE("releases/opaque: every field kind is copied with counts and masks applied")
{
  Bits::Writer data;
  data.B(true);
  data.RC(7);
  data.BS(300);
  data.BD(2.5);
  data.RD(1.0);
  data.RD(2.0);
  data.Point3({.X = 1, .Y = 2, .Z = 3});
  data.BL(2); // N: two of what follows,
  data.BL(1); // each a count of its own,
  data.BS(4);
  data.BL(0);
  data.BL(0b1011); // MASK: three of the four low bits.
  for (int text = 0; text < 3; ++text)
  {
    static_cast<void>(data.T("x", Standard()));
  }
  const Opaque own = Own(Version::R2000, data, Bits::Writer{});
  const std::optional<Opaque> same = Recast(own, "B RC BS BD 2RD 3BD N { N { BS } } MASK { T }", Version::R14);
  REQUIRE(same);
  CHECK(same->Data == own.Data);
  CHECK(same->Release == Version::R14);
  const std::optional<Opaque> wide = Recast(own, "B RC BS BD 2RD 3BD N { N { BS } } MASK { T }", Version::R2010);
  REQUIRE(wide);
  CHECK(wide->Strings.Size == 3 * (10 + 16));
}

TEST_CASE("releases/opaque: an evaluation expression holds no value or a real or a short or a long")
{
  Bits::Writer real;
  real.BS(40);
  real.BD(1.7976931348623157e308);
  real.BS(70);
  real.BS(3);
  real.BS(-9999);
  real.BS(90);
  real.BL(-2);
  const Opaque own = Own(Version::R2018, real, Bits::Writer{});
  const std::optional<Opaque> downgraded = Recast(own, "X X X X", Version::R14);
  REQUIRE(downgraded);
  CHECK(downgraded->Data == own.Data);
}

TEST_CASE("releases/opaque: a lookup table's cells are texts and its columns a count")
{
  Bits::Writer data;
  Bits::Writer strings;
  data.BL(2);
  data.BL(1);
  static_cast<void>(strings.TU("one"));
  static_cast<void>(strings.TU("two"));
  data.BL(9);
  const Opaque own = Own(Version::R2010, data, strings);
  const std::optional<Opaque> downgraded = Recast(own, "LOOKUP { BL }", Version::R2000);
  REQUIRE(downgraded);
  Bits::Reader back{downgraded->Data.Bytes};
  CHECK(back.BL() == 2);
  CHECK(back.BL() == 1);
  CHECK(back.T(Standard()) == "one");
  CHECK(back.T(Standard()) == "two");
  CHECK(back.BL() == 9);
  CHECK(downgraded->Strings.Size == 0);
  Bits::Writer negative;
  negative.BL(-1);
  negative.BL(1);
  CHECK_FALSE(Recast(Own(Version::R2010, negative, Bits::Writer{}), "LOOKUP", Version::R2000));
  Bits::Writer wide;
  wide.BL(1);
  wide.BL(-1);
  CHECK_FALSE(Recast(Own(Version::R2010, wide, Bits::Writer{}), "LOOKUP", Version::R2000));
  Bits::Writer huge;
  huge.BL(1000);
  huge.BL(1000);
  CHECK_FALSE(Recast(Own(Version::R2010, huge, Bits::Writer{}), "LOOKUP", Version::R2000));
}

TEST_CASE("releases/opaque: a context's version is 3 up to R2007 and 4 from R2010 with the rest kept")
{
  Bits::Writer older;
  older.BS(3);
  older.Bits(0x2A, 6);
  Bits::Writer newer;
  newer.BS(4);
  newer.Bits(0x2A, 6);
  const Opaque before = Own(Version::R2007, older, Bits::Writer{});
  Opaque after = Own(Version::R2010, newer, Bits::Writer{});
  CHECK(Recast(before, "V REST", Version::R2010) == after);
  after.Release = Version::R2018;
  CHECK(Recast(Own(Version::R2010, newer, Bits::Writer{}), "V REST", Version::R2018) == after);
  CHECK(Recast(Own(Version::R2010, newer, Bits::Writer{}), "V REST", Version::R2007)->Data == before.Data);
}

TEST_CASE("releases/opaque: strings are kept within releases that hold them and refused across R2007")
{
  Bits::Writer data;
  data.BL(1);
  Bits::Writer strings;
  static_cast<void>(strings.TU("kept"));
  const Opaque own = Own(Version::R2013, data, strings);
  const std::optional<Opaque> kept = Recast(own, "REST", Version::R2010);
  REQUIRE(kept);
  CHECK(kept->Strings == own.Strings);
  CHECK_FALSE(Recast(own, "REST", Version::R2004));
}

TEST_CASE("releases/opaque: a color is copied between releases that write it alike and rewritten otherwise")
{
  const Color red{.Index = 1, .Rgb = {}, .Names = ColorNames{.Name = "red", .Book = "book"}};
  Bits::Writer older;
  older.CMC(Color{.Index = 1}, Version::R2000);
  const std::optional<Opaque> same = Recast(Own(Version::R2000, older, Bits::Writer{}), "CMC", Version::R14);
  REQUIRE(same);
  Bits::Writer wide;
  Bits::Writer names;
  wide.CMC(red, Version::R2010, names);
  const Opaque newer = Own(Version::R2010, wide, names);
  const std::optional<Opaque> kept = Recast(newer, "CMC", Version::R2018);
  REQUIRE(kept);
  CHECK(kept->Data == newer.Data);
  CHECK(kept->Strings == newer.Strings);
  const std::optional<Opaque> downgraded = Recast(newer, "CMC", Version::R2004);
  REQUIRE(downgraded);
  Bits::Reader back{downgraded->Data.Bytes};
  const Color read = back.CMC(Version::R2004);
  REQUIRE(read.Names);
  CHECK((*read.Names)->Name == "red");
  CHECK((*read.Names)->Book == "book");
  const std::optional<Opaque> upgraded = Recast(*downgraded, "CMC", Version::R2007);
  REQUIRE(upgraded);
  CHECK(upgraded->Strings.Size > 0);
  const std::optional<Opaque> lowest = Recast(*downgraded, "CMC", Version::R2000);
  REQUIRE(lowest);
  Bits::Reader index{lowest->Data.Bytes};
  CHECK(index.CMC(Version::R2000).Index == 1);
}

TEST_CASE("releases/opaque: below R2004 a color takes AutoCAD's nearest index or 257 for none and comes back")
{
  Bits::Writer wide;
  wide.CMC(Color{.Index = 0, .Rgb = static_cast<int>(0xC2123456U)}, Version::R2004);
  wide.CMC(Color{.Index = 0, .Rgb = -939524096}, Version::R2004);
  wide.CMC(Color{.Index = 256}, Version::R2004);
  const std::optional<Opaque> downgraded =
      Recast(Own(Version::R2004, wide, Bits::Writer{}), "CMC CMC CMC", Version::R2000);
  REQUIRE(downgraded);
  Bits::Reader older{downgraded->Data.Bytes};
  CHECK(older.CMC(Version::R2000).Index == 148);
  CHECK(older.CMC(Version::R2000).Index == 257);
  CHECK(older.CMC(Version::R2000).Index == 256);
  const std::optional<Opaque> upgraded = Recast(*downgraded, "CMC CMC CMC", Version::R2004);
  REQUIRE(upgraded);
  Bits::Reader newer{upgraded->Data.Bytes};
  CHECK(newer.CMC(Version::R2004) == Color{.Index = 148});
  CHECK(newer.CMC(Version::R2004) == Color{.Index = 0, .Rgb = -939524096});
  CHECK(newer.CMC(Version::R2004) == Color{.Index = 256});
}

TEST_CASE("releases/opaque: R2018-only data is dropped below R2018 and given on upgrading with its bit 0 below")
{
  constexpr std::string_view Layout = "BS T B18 T18 BL18";
  Bits::Writer data;
  Bits::Writer strings;
  data.BS(0);
  static_cast<void>(strings.TU("Metric50"));
  data.B(true);
  static_cast<void>(strings.TU("Shown"));
  data.BL(5);
  const Opaque latest = Own(Version::R2018, data, strings);
  std::vector<Content> taken;
  const std::optional<Opaque> downgraded = DwgIO::Releases::Recast(
      latest, Layout, RecastContext{.To = Version::R2013, .Text = &Standard(), .Given = {}, .Taken = &taken});
  REQUIRE(downgraded);
  CHECK(taken == std::vector<Content>{std::string{"Shown"}, std::int64_t{5}});
  Bits::Reader below{downgraded->Data.Bytes};
  CHECK(below.BS() == 0);
  CHECK_FALSE(below.B());
  CHECK(below.Position() == downgraded->Data.Size);
  // Taken by no one, the values go; the bits are the same.
  const std::optional<Opaque> dropped = Recast(latest, Layout, Version::R2004);
  REQUIRE(dropped);
  CHECK(dropped->Data.Size > 0);
  // Within the releases that lack them, nothing is read or written.
  const std::optional<Opaque> lower = Recast(*downgraded, Layout, Version::R2010);
  REQUIRE(lower);
  CHECK(lower->Data == downgraded->Data);
  // Upgrading, the values given are written; where none or another kind is given, an empty text and 0.
  const std::vector<Content> given{std::string{"Shown"}, std::int64_t{5}};
  const std::optional<Opaque> upgraded = DwgIO::Releases::Recast(
      *downgraded, Layout, RecastContext{.To = Version::R2018, .Text = &Standard(), .Given = given, .Taken = nullptr});
  REQUIRE(upgraded);
  CHECK(upgraded->Strings == latest.Strings);
  Bits::Reader again{upgraded->Data.Bytes};
  CHECK(again.BS() == 0);
  CHECK_FALSE(again.B()); // Lost below R2018.
  CHECK(again.BL() == 5);
  const std::vector<Content> wrong{std::int64_t{1}, std::string{"x"}};
  const std::optional<Opaque> blank = DwgIO::Releases::Recast(
      *downgraded, Layout, RecastContext{.To = Version::R2018, .Text = &Standard(), .Given = wrong, .Taken = nullptr});
  REQUIRE(blank);
  Bits::Reader empty{blank->Strings.Bytes};
  CHECK(empty.TU() == "Metric50");
  CHECK(empty.TU().empty());
  const std::optional<Opaque> none = Recast(*downgraded, Layout, Version::R2018);
  REQUIRE(none);
  CHECK(none->Strings == blank->Strings);
  // A text given longer than a count holds does not fit.
  const std::vector<Content> longer{std::string(70000, 'x'), std::int64_t{0}};
  CHECK_FALSE(DwgIO::Releases::Recast(
      *downgraded, Layout,
      RecastContext{.To = Version::R2018, .Text = &Standard(), .Given = longer, .Taken = nullptr}));
  // Within R2018 they are copied.
  const std::optional<Opaque> same = Recast(latest, Layout, Version::R2018);
  REQUIRE(same);
  CHECK(*same == latest);
}

TEST_CASE("releases/opaque: bits that do not read by the layout are not converted")
{
  Bits::Writer data;
  data.BL(1);
  const Opaque own = Own(Version::R2004, data, Bits::Writer{});
  CHECK_FALSE(Recast(own, "BL BL", Version::R2007));  // Past the data's end.
  CHECK_FALSE(Recast(own, "", Version::R2007));       // Data left over.
  CHECK_FALSE(Recast(own, "QQ", Version::R2007));     // No such field.
  CHECK_FALSE(Recast(own, "{ BL }", Version::R2007)); // No count to repeat by.
  CHECK_FALSE(Recast(own, "N { BL", Version::R2007)); // No closing brace.
  CHECK_FALSE(Recast(own, "X", Version::R2007));      // A value of a type the files never held.
  CHECK(Recast(own, "N { }", Version::R2007));        // One empty repeat.
  Bits::Writer many;
  many.BL(-1);
  CHECK_FALSE(Recast(Own(Version::R2004, many, Bits::Writer{}), "N { }", Version::R2007));
  Bits::Writer most;
  most.BL(70000);
  CHECK_FALSE(Recast(Own(Version::R2004, most, Bits::Writer{}), "N { }", Version::R2007));
  Bits::Writer twice;
  twice.BL(2);
  CHECK_FALSE(Recast(Own(Version::R2004, twice, Bits::Writer{}), "N { QQ }", Version::R2007));
  // A count read into the padding of the strings' last byte.
  const Opaque padded{.Release = Version::R2007,
                      .Data = {},
                      .Handles = {},
                      .Strings = BitString{.Bytes = {std::byte{0x40}, std::byte{0x00}}, .Size = 4}};
  CHECK_FALSE(Recast(padded, "T", Version::R2010));
  Bits::Writer one;
  one.B(true);
  CHECK_FALSE(Recast(Own(Version::R2004, one, Bits::Writer{}), "B B", Version::R2007)); // Into the padding.
  CHECK_FALSE(Recast(Own(Version::R2004, one, Bits::Writer{}), "BL", Version::R2007));  // Past the bytes.
  Bits::Writer strings;
  static_cast<void>(strings.TU("left"));
  CHECK_FALSE(Recast(Own(Version::R2007, Bits::Writer{}, strings), "", Version::R2010));         // Strings left over.
  CHECK_FALSE(Recast(Own(Version::R2007, Bits::Writer{}, Bits::Writer{}), "T", Version::R2004)); // Past them.
  Bits::Writer whole;
  static_cast<void>(whole.TU(std::string(40000, 'a')));
  CHECK_FALSE(Recast(Own(Version::R2007, Bits::Writer{}, whole), "T", Version::R2004)); // Too long for a T.
}
} // namespace DwgIO::Releases

namespace DwgIO::Releases
{
namespace
{
/**
 *  A break point's handles and bits as a release holds them: a pointer, the null hard owner up to R2000, and the bits
 *  from R2004 the given values, the second from R2013.
 */
auto Broken(Version release, bool first, bool second) -> Opaque
{
  Bits::Writer data;
  Bits::Writer handles;
  handles.H(Bits::Reference{.Code = 4, .Value = Handle{0x2DC}});
  if (release < Version::R2004)
  {
    handles.H(Bits::Reference{.Code = 3, .Value = {}});
  }
  else
  {
    data.B(first);
  }
  if (release >= Version::R2013)
  {
    data.B(second);
  }
  data.BL(7);
  return Opaque{.Release = release, .Data = RunOf(data), .Handles = RunOf(handles), .Strings = {}};
}
} // namespace

TEST_CASE("releases/opaque: a break point's R2000 null handle and R2004 and R2013 bits are made and dropped")
{
  constexpr std::string_view Layout = "H H14 B04 BL";
  for (const Version from : {Version::R2000, Version::R2004, Version::R2013})
  {
    for (const Version to : {Version::R2000, Version::R2007, Version::R2018})
    {
      CAPTURE(from);
      CAPTURE(to);
      const std::optional<Opaque> cast = Recast(Broken(from, true, false), Layout, to);
      REQUIRE(cast);
      CHECK(*cast == Broken(to, true, false));
    }
  }
  // AutoCAD's files hold no other values: a handle of R2000 that is not null, a bit of R2004 of 0, a bit of R2013
  // of 1.
  Opaque pointing = Broken(Version::R2000, true, false);
  Bits::Writer handles;
  handles.H(Bits::Reference{.Code = 4, .Value = Handle{0x2DC}});
  handles.H(Bits::Reference{.Code = 3, .Value = Handle{0x2DD}});
  pointing.Handles = RunOf(handles);
  CHECK_FALSE(Recast(pointing, Layout, Version::R2004));
  CHECK_FALSE(Recast(Broken(Version::R2004, false, false), Layout, Version::R2000));
  CHECK_FALSE(Recast(Broken(Version::R2013, true, true), Layout, Version::R2010));
  // A handle read past the bits the stream holds.
  Opaque cut = Broken(Version::R2004, true, false);
  cut.Handles.Size = 3;
  CHECK_FALSE(Recast(cut, Layout, Version::R2007));
}
} // namespace DwgIO::Releases
