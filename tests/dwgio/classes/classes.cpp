/***********************************************************************************************************************
 *  @file      tests/dwgio/classes/classes.cpp
 *  @brief     The classes section: read, written back, R14's, and cut short.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "bits/writer.hpp"
#include "classes/layout.hpp"
#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "fields/context.hpp"
#include "file/locator/layout.hpp"
#include "file/locator/section.hpp"
#include "file/locator/sentinels.hpp"
#include "file/paged/layout.hpp"
#include "support/fixtures.hpp"

#include "dwgio/class.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <span>
#include <string_view>
#include <vector>

namespace DwgIO::Classes
{
namespace
{
const Encoding::Scheme Latin = Encoding::MakeScheme(Encoding::Standard());

auto Body(std::string_view fixture) -> std::vector<std::byte>
{
  std::vector<Issue> issues;
  const auto layout = File::Locator::ReadLayout(ReadFixture(fixture), issues);
  REQUIRE(layout);
  for (const File::Locator::Located &block : layout->Blocks)
  {
    if (block.Name == "classes")
    {
      return block.Bytes;
    }
  }
  FAIL("no classes block");
  return {};
}
} // namespace

TEST_CASE("classes/classes: the fixture's seventeen classes are read")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const std::vector<Class> classes = Read(Body("foundation-r2000.dwg"), with);
  REQUIRE(classes.size() == 17);
  CHECK(classes.front() == Class{.Number = 500,
                                 .ProxyFlags = 0,
                                 .Application = "ObjectDBX Classes",
                                 .CppName = "AcDbDictionaryWithDefault",
                                 .DxfName = "ACDBDICTIONARYWDFLT",
                                 .WasZombie = false,
                                 .ItemClass = 0x1F3});
  CHECK(classes[6].Number == 506);
  CHECK(classes[6].DxfName == "VISUALSTYLE");
  CHECK(classes[6].ItemClass == 0x1F3);
  CHECK(classes.back().Number == 516);
  CHECK(issues.empty());
}

TEST_CASE("classes/classes: the classes write back byte for byte")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues};
  const std::vector<std::byte> body = Body("foundation-r2000.dwg");
  CHECK(Write(Read(body, with), with) == body);
}

TEST_CASE("classes/classes: an R14 file's classes read and write back")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R14, .Text = &Latin, .Issues = &issues};
  const std::vector<std::byte> body = Body("foundation-r14.dwg");
  const std::vector<Class> classes = Read(body, with);
  CHECK(classes.size() == 19);
  CHECK(classes.back().DxfName == "XRECORD");
  CHECK(classes.front().DxfName == "ACDBDICTIONARYWDFLT");
  CHECK(Write(classes, with) == body);
  CHECK(issues.empty());
}

TEST_CASE("classes/classes: a truncated body gives what fit and notes it")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2000, .Text = &Latin, .Issues = &issues, .Offset = 20695};
  std::vector<std::byte> body = Body("foundation-r2000.dwg");
  body.resize(100);
  const std::vector<Class> classes = Read(body, with);
  CHECK(classes.size() == 1);
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "classes cut short");
  CHECK(issues.front().Offset == 20695 + 100);
}

TEST_CASE("classes/classes: an R2007 body that ends at bit zero holds no classes")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2007, .Text = &Latin, .Issues = &issues};
  std::vector<std::byte> body(36, std::byte{0xFF});
  std::fill_n(body.begin(), 4, std::byte{0}); // The leading RL, the bit the data ends at.
  CHECK(Read(body, with).empty());
  CHECK(issues.empty());
}

TEST_CASE("classes/classes: the R2004 items write and read back and a class cut inside them is dropped")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2004, .Text = &Latin, .Issues = &issues};
  const Class one{.Number = 500,
                  .ProxyFlags = 1,
                  .Application = "A",
                  .CppName = "B",
                  .DxfName = "C",
                  .WasZombie = true,
                  .ItemClass = 0x1F2,
                  .Instances = 3,
                  .DwgVersion = 4,
                  .Maintenance = 5,
                  .Unknown1 = 6,
                  .Unknown2 = 7};
  const std::vector<std::byte> body = Write(std::vector{one, one}, with);
  CHECK(Read(body, with) == std::vector{one, one});
  std::vector<std::byte> cut = body;
  cut.resize(cut.size() - 3);
  CHECK(Read(cut, with) == std::vector{one});
  REQUIRE(issues.size() == 1);
  CHECK(issues.front().What == "classes cut short");
}

TEST_CASE("classes/classes: the R2004 fixtures' classes read and write back byte for byte")
{
  for (const std::string_view name : {"empty", "curves"})
  {
    CAPTURE(name);
    std::vector<Issue> issues;
    const File::Paged::Layout layout = *File::Paged::ReadLayout(ReadFixture(std::format("{}-r2004.dwg", name)), issues);
    const File::Paged::Section &section = layout.Sections[9];
    REQUIRE(section.Name == "classes");
    const std::span<const std::byte> framed = std::span{section.Bytes}.first(section.Bytes.size() - Trailer);
    const File::Locator::Record whole{.Number = 1, .Seeker = 0, .Size = 0};
    const std::vector<std::byte> body = File::Locator::Cut(framed, whole, File::Locator::Sentinels::Classes)->Body;
    const Fields::Context with{.Release = Version::R2004, .Text = &Latin, .Issues = &issues};
    const std::vector<Class> classes = Read(body, with);
    // MLEADERSTYLE's maintenance version, 377, is the one value past a byte: it shows the field is a BL.
    CHECK(classes[8].DxfName == "MLEADERSTYLE");
    CHECK(classes[8].Maintenance == 377);
    CHECK(classes[8].DwgVersion == 33);
    CHECK(issues.empty());
    CHECK(classes.size() >= 17);
    CHECK(classes.front().Number == 500);
    CHECK(classes.front().Instances >= 0);
    CHECK(Write(classes, with) == body);
  }
}

TEST_CASE("classes/classes: the R2010 to R2018 fixtures' classes and strings write back byte for byte")
{
  for (const std::string_view name : {"empty-r2010.dwg", "curves-r2013.dwg", "objects-r2018.dwg"})
  {
    CAPTURE(name);
    std::vector<Issue> issues;
    const File::Paged::Layout layout = *File::Paged::ReadLayout(ReadFixture(name), issues);
    const auto section = std::ranges::find(layout.Sections, "classes", &File::Paged::Section::Name);
    REQUIRE(section != layout.Sections.end());
    const std::span<const std::byte> framed = std::span{section->Bytes}.first(section->Bytes.size() - Trailer);
    const File::Locator::Record whole{.Number = 1, .Seeker = 0, .Size = 0};
    const std::vector<std::byte> body =
        File::Locator::Cut(framed, whole, File::Locator::Widen(File::Locator::Sentinels::Classes))->Body;
    const Fields::Context with{.Release = layout.Head.Release, .Text = &Latin, .Issues = &issues};
    const std::vector<Class> classes = Read(body, with);
    CHECK(issues.empty());
    REQUIRE(classes.size() >= 17);
    CAPTURE(classes.size());
    CHECK(classes.front().DxfName == "ACDBDICTIONARYWDFLT");
    CHECK(classes.front().Application == "ObjectDBX Classes");
    CHECK(Write(classes, with) == body);
  }
}

TEST_CASE("classes/classes: an R2010 body whose string stream reaches before its data is noted")
{
  std::vector<Issue> issues;
  const Fields::Context with{.Release = Version::R2010, .Text = &Latin, .Issues = &issues};
  // The bit count 7: the bit before it, inside the count itself, is set, and no size fits before it.
  Bits::Writer bad;
  bad.RL(7);
  CHECK(Read(bad.Take(), with).empty());
  REQUIRE_FALSE(issues.empty());
  CHECK(issues.front().What == "classes string stream size before the data");
}
} // namespace DwgIO::Classes
