/***********************************************************************************************************************
 *  @file      tests/dwgio/support/reader.hpp
 *  @brief     A fixture opened for the object reader: its bytes, classes and map.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_READER_HPP
#define DWGIO_TESTS_SUPPORT_READER_HPP

#include "bits/crc.hpp"
#include "bits/writer.hpp"
#include "classes/layout.hpp"
#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "fields/context.hpp"
#include "file/locator/layout.hpp"
#include "file/locator/section.hpp"
#include "file/locator/sentinels.hpp"
#include "file/paged/head.hpp"
#include "file/paged/layout.hpp"
#include "objects/context.hpp"
#include "objects/map.hpp"
#include "support/fixtures.hpp"

#include "dwgio/class.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace DwgIO::Objects
{
inline const Encoding::Scheme Latin = Encoding::MakeScheme(Encoding::Standard());

/**
 *  A fixture cut open: its bytes, its classes and its map, as the object reader needs them.
 */
struct Fixture
{
  std::vector<std::byte> File{};
  std::vector<Class> Classes{};
  std::vector<Mapped> Entries{};
  std::vector<Issue> Issues{};
  Version Release{};

  auto Reading() -> Objects::FileContext
  {
    return Objects::FileContext{.Release = Release, .Text = &Latin, .Classes = Classes, .Issues = &Issues};
  }

  auto Context() -> Fields::Context
  {
    return Fields::Context{.Release = Release, .Text = &Latin, .Issues = &Issues, .Classes = &Classes};
  }
};

inline auto Open(std::string_view name) -> Fixture
{
  Fixture fixture{.File = ReadFixture(name), .Classes = {}, .Entries = {}, .Issues = {}, .Release = {}};
  const auto layout = File::Locator::ReadLayout(fixture.File, fixture.Issues);
  REQUIRE(layout);
  fixture.Release = layout->Head.Release;
  const Fields::Context with{.Release = fixture.Release, .Text = &Latin, .Issues = &fixture.Issues};
  for (const File::Locator::Located &block : layout->Blocks)
  {
    if (block.Name == "classes")
    {
      fixture.Classes = Classes::Read(block.Bytes, with);
    }
    if (block.Name == "map")
    {
      fixture.Entries = ReadMap(block.Bytes, block.Offset, fixture.Issues);
    }
  }
  REQUIRE(fixture.Issues.empty());
  return fixture;
}

/**
 *  A paged fixture cut open as the object reader needs it: the objects section as the bytes, its classes and its map.
 */
inline auto OpenPaged(std::string_view name) -> Fixture
{
  Fixture fixture{.File = {}, .Classes = {}, .Entries = {}, .Issues = {}, .Release = {}};
  const std::vector<std::byte> file = ReadFixture(name);
  const auto layout = File::Paged::ReadLayout(file, fixture.Issues);
  REQUIRE(layout);
  fixture.Release = layout->Head.Release;
  const bool wide = File::Paged::IsWide(fixture.Release, layout->Head.Maintenance);
  const Fields::Context with{.Release = fixture.Release, .Text = &Latin, .Issues = &fixture.Issues};
  const File::Locator::Record whole{.Number = 1, .Seeker = 0, .Size = 0};
  for (const File::Paged::Section &section : layout->Sections)
  {
    if (section.Name == "classes")
    {
      const std::span<const std::byte> framed = std::span{section.Bytes}.first(section.Bytes.size() - Classes::Trailer);
      const auto body = File::Locator::Cut(framed, whole,
                                           wide ? File::Locator::Widen(File::Locator::Sentinels::Classes)
                                                : File::Locator::Sentinels::Classes);
      REQUIRE(body);
      fixture.Classes = Classes::Read(body->Body, with);
    }
    if (section.Name == "map")
    {
      fixture.Entries = ReadMap(section.Bytes, 0, fixture.Issues);
    }
    if (section.Name == "objects")
    {
      fixture.File = section.Bytes;
    }
  }
  REQUIRE(fixture.Issues.empty());
  return fixture;
}

/**
 *  An R2000 object made of its data bits: the size, the data, the CRC.
 */
inline auto FrameObject(const Bits::Writer &data) -> std::vector<std::byte>
{
  Bits::Writer whole;
  whole.MS(data.Bytes().size());
  whole.Raw(data.Bytes());
  const std::uint16_t crc = Bits::Crc(whole.Bytes(), Bits::Seed);
  whole.RS(crc);
  return whole.Take();
}
} // namespace DwgIO::Objects

#endif /* DWGIO_TESTS_SUPPORT_READER_HPP */
