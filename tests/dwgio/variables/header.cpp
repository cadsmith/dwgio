/***********************************************************************************************************************
 *  @file      tests/dwgio/variables/header.cpp
 *  @brief     The header variables against the DXF twin, written back bit for bit, and cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/header.hpp"

#include "bits/writer.hpp"
#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "fields/context.hpp"
#include "file/locator/layout.hpp"
#include "file/locator/section.hpp"
#include "file/locator/sentinels.hpp"
#include "file/paged/head.hpp"
#include "file/paged/layout.hpp"
#include "support/fixtures.hpp"
#include "support/twin.hpp"
#include "variables/layout.hpp"

#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/save.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO::Variables
{
namespace
{
const Encoding::Scheme Latin = Encoding::MakeScheme(Encoding::Standard());

constexpr double MillisecondsADay = 86400000.0;

auto Body(std::string_view fixture) -> std::vector<std::byte>
{
  std::vector<Issue> issues;
  const auto layout = File::Locator::ReadLayout(ReadFixture(fixture), issues);
  REQUIRE(layout);
  for (const File::Locator::Located &block : layout->Blocks)
  {
    if (block.Name == "header")
    {
      return block.Bytes;
    }
  }
  FAIL("no header block");
  return {};
}

auto Point(const std::vector<Pair> &twin, std::string_view name) -> Vector
{
  for (std::size_t at = 0; at + 3 < twin.size(); ++at)
  {
    if (twin[at].Code == 9 && twin[at].Value == name)
    {
      return Vector{
          .X = std::stod(twin[at + 1].Value), .Y = std::stod(twin[at + 2].Value), .Z = std::stod(twin[at + 3].Value)};
    }
  }
  FAIL(name);
  return {};
}

auto FirstDifference(const std::vector<std::byte> &one, const std::vector<std::byte> &two) -> std::optional<std::size_t>
{
  const std::size_t bits = std::min(one.size(), two.size()) * 8;
  for (std::size_t bit = 0; bit < bits; ++bit)
  {
    const auto mask = static_cast<std::byte>(0x80 >> (bit % 8));
    if ((one[bit / 8] & mask) != (two[bit / 8] & mask))
    {
      return bit;
    }
  }
  return one.size() == two.size() ? std::nullopt : std::optional{bits};
}
} // namespace

TEST_CASE("variables/header: the R2000 fixture's variables match its DXF twin")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const DwgIO::Header header = Read(Body("foundation-r2000.dwg"), with);
  const std::vector<Pair> twin = ReadTwin("foundation-r2000.dxf");
  CHECK(issues.empty());
  CHECK(header.Handseed == Handle{0x417});
  CHECK(header.Handseed < ParseHandle(Variable(twin, "$HANDSEED")));
  const std::optional<std::size_t> layer = FindOpening(twin, "LAYER", Variable(twin, "$CLAYER"));
  REQUIRE(layer);
  CHECK(header.Clayer == Self(twin, *layer));
  CHECK(header.Ltscale == std::stod(Variable(twin, "$LTSCALE")));
  CHECK(header.Textsize == std::stod(Variable(twin, "$TEXTSIZE")));
  CHECK(header.Dimscale == std::stod(Variable(twin, "$DIMSCALE")));
  CHECK(header.Insbase == Point(twin, "$INSBASE"));
  CHECK(header.Extmin == Point(twin, "$EXTMIN"));
  CHECK(header.Extmax == Point(twin, "$EXTMAX"));
  const double created = header.TdcreateDays + (header.TdcreateMilliseconds / MillisecondsADay);
  CHECK(std::abs(created - std::stod(Variable(twin, "$TDCREATE"))) < 1e-6);
  CHECK(header.Cecolor.Index == std::stoi(Variable(twin, "$CECOLOR")));
  CHECK(header.Unknown1 == 412148564080.0);
  CHECK(header.Dimaso);
  CHECK(header.BlockRecordModelSpace == Handle{0x1F});
  CHECK(header.Padding.size() <= 1);
}

TEST_CASE("variables/header: the R2000 fixture's variables write back bit for bit")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const std::vector<std::byte> body = Body("foundation-r2000.dwg");
  const std::vector<std::byte> written = Write(Read(body, with), with);
  const std::optional<std::size_t> differs = FirstDifference(body, written);
  CAPTURE(differs.value_or(0));
  CAPTURE(body.size());
  CAPTURE(written.size());
  CHECK_FALSE(differs);
  CHECK(issues.empty());
}

TEST_CASE("variables/header: the R14 fixture's variables read and write back")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R14, .Text = &Latin, .Issues = &issues};
  const std::vector<std::byte> body = Body("foundation-r14.dwg");
  const DwgIO::Header header = Read(body, with);
  CHECK(issues.empty());
  CHECK(header.Ltscale == 1.0);
  CHECK(header.Insbase == Vector{});
  CHECK(header.Handseed);
  CHECK(header.BlockRecordModelSpace == Handle{0x1F});
  const std::vector<std::byte> written = Write(header, with);
  const std::optional<std::size_t> differs = FirstDifference(body, written);
  CAPTURE(differs.value_or(0));
  CHECK_FALSE(differs);
}

TEST_CASE("variables/header: a truncated body reads what fits and notes it")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Offset = 20207};
  std::vector<std::byte> body = Body("foundation-r2000.dwg");
  body.resize(100);
  const DwgIO::Header header = Read(body, with);
  CHECK(header.Unknown1 == 412148564080.0);
  CHECK(header.Dimaso);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front() ==
        Issue{.Code = IssueCode::Truncated, .Offset = 20207 + 100, .What = "header variables cut short"});
}

TEST_CASE("variables/header: a release after R2000 skips the items it lacks")
{
  std::vector<Issue> issues;
  const Fields::Context later{.Release = Version::R2004, .Text = &Latin, .Issues = &issues};
  const std::vector<std::byte> body = Body("foundation-r2000.dwg");
  const DwgIO::Header skipped = Read(body, later);
  CHECK(skipped.Unknown1 == 412148564080.0);
  CHECK(Read(Write(skipped, later), later).Unknown1 == 412148564080.0);
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  DwgIO::Header header = Read(body, with);
  header.Cepsntype = 3;
  header.Cpsnid = Handle{0x99};
  header.Padding.clear();
  const DwgIO::Header back = Read(Write(header, with), with);
  CHECK(back.Cepsntype == 3);
  CHECK(back.Cpsnid == Handle{0x99});
  CHECK(back.Padding.size() <= 1);
}

TEST_CASE("variables/header: a handle whose code differs from the specification's is noted")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  std::vector<std::byte> body = Body("foundation-r2000.dwg");
  body[13] ^= std::byte{0x04};
  const DwgIO::Header header = Read(body, with);
  CHECK(header.Dimaso);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What.ends_with("with code 1, 5 expected"));
}

TEST_CASE("variables/header: an empty body fails at its first item in either release")
{
  std::vector<Issue> issues;
  for (const Version release : {Version::R2000, Version::R14})
  {
    const Fields::Context with{.Release = release, .Text = &Latin, .Issues = &issues};
    const DwgIO::Header header = Read({}, with);
    CHECK(header.Unknown2 == 1.0);
  }
  CHECK(issues.size() == 2);
}

TEST_CASE("variables/header: an R14 header without its padding bits is padded to the byte")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R14, .Text = &Latin, .Issues = &issues};
  DwgIO::Header header = Read(Body("foundation-r14.dwg"), with);
  header.Padding.clear();
  const std::vector<std::byte> written = Write(header, with);
  CHECK(written.size() == Body("foundation-r14.dwg").size());
  CHECK(Read(written, with).Ltscale == 1.0);
}

TEST_CASE("variables/header: the R2004 fixtures' variables read and write back byte for byte")
{
  for (const std::string_view name : {"empty", "tables", "entities", "curves", "objects", "foundation"})
  {
    CAPTURE(name);
    std::vector<Issue> issues;
    const File::Paged::Layout layout = *File::Paged::ReadLayout(ReadFixture(std::format("{}-r2004.dwg", name)), issues);
    const File::Paged::Section &section = layout.Sections.back();
    REQUIRE(section.Name == "header");
    const File::Locator::Record whole{.Number = 0, .Seeker = 0, .Size = 0};
    const std::vector<std::byte> body =
        File::Locator::Cut(section.Bytes, whole, File::Locator::Sentinels::Header)->Body;
    const Fields::Context with{.Release = Version::R2004, .Text = &Latin, .Issues = &issues};
    const Header header = Read(body, with);
    CHECK(issues.empty());
    CHECK((header.Dimassoc == 1 || header.Dimassoc == 2));
    CHECK(header.Projectname.empty());
    CHECK(header.DictionaryColors != Handle{});
    CHECK(header.DictionaryMaterials != Handle{});
    CHECK(header.Menuname == ".");
    CHECK(std::ranges::equal(Write(header, with), body));
  }
}

TEST_CASE("variables/header: the R2010 to R2018 fixtures' variables write back byte for byte with their frame")
{
  for (const std::string_view year : {"2010", "2013", "2018"})
  {
    for (const std::string_view name : {"empty", "tables", "entities", "curves", "objects", "foundation"})
    {
      CAPTURE(name);
      CAPTURE(year);
      std::vector<Issue> issues;
      const File::Paged::Layout layout =
          *File::Paged::ReadLayout(ReadFixture(std::format("{}-r{}.dwg", name, year)), issues);
      const File::Paged::Section &section = layout.Sections.back();
      REQUIRE(section.Name == "header");
      REQUIRE(File::Paged::IsWide(layout.Head.Release, layout.Head.Maintenance));
      const File::Locator::Record whole{.Number = 0, .Seeker = 0, .Size = 0};
      const File::Locator::Framed framed =
          *File::Locator::Cut(section.Bytes, whole, File::Locator::Widen(File::Locator::Sentinels::Header));
      CHECK(framed.Issues.empty());
      const Fields::Context with{.Release = layout.Head.Release, .Text = &Latin, .Issues = &issues};
      const Header header = Read(framed.Body, with);
      CHECK(issues.empty());
      CHECK(header.Menuname == ".");
      CHECK(header.Clayer == Handle{0x10});
      CHECK(header.Cmaterial != Handle{});
      CHECK(header.DictionaryVisualStyles != Handle{});
      CHECK(header.Latitude == doctest::Approx(37.795));
      CHECK(header.Fingerprintguid.size() == 38);
      const std::vector<std::byte> written = Write(header, with);
      CHECK(std::ranges::equal(written, framed.Body));
      CHECK(File::Locator::Wrap(written, File::Locator::Widen(File::Locator::Sentinels::Header)) == section.Bytes);
    }
  }
}

TEST_CASE("variables/header: an R2010 header whose string stream size runs past its data is noted and read")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2010, .Text = &Latin, .Issues = &issues};
  // The bit count 7: the bit before it, inside the count itself, is set, and no size fits before it.
  Bits::Writer bad;
  bad.RL(7);
  const Header header = Read(bad.Take(), with);
  REQUIRE(issues.size() == 2);
  CHECK(issues.front().What == "header string stream size before the data");
  CHECK(issues.back().What == "header variables cut short");
  CHECK(header.Menuname.empty());
}

TEST_CASE("variables/header: REALWORLDSCALE and SHADOWPLANELOCATION keep AutoCAD's values through a save")
{
  for (const std::string_view name : {"variables-r2007.dwg", "variables-r2018.dwg"})
  {
    CAPTURE(name);
    const std::vector<std::byte> bytes = ReadFixture(name);
    const std::expected<Loaded, Failure> loaded = Load(bytes);
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    CHECK_FALSE(loaded->Drawing.Header.Realworldscale);
    CHECK(loaded->Drawing.Header.Shadowplanelocation == -5.0);
    const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
    REQUIRE(saved);
    const std::expected<Loaded, Failure> again = Load(saved->Bytes);
    REQUIRE(again);
    CHECK_FALSE(again->Drawing.Header.Realworldscale);
    CHECK(again->Drawing.Header.Shadowplanelocation == -5.0);
  }
}
} // namespace DwgIO::Variables
