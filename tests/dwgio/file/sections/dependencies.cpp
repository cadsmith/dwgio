/***********************************************************************************************************************
 *  @file      tests/dwgio/file/sections/dependencies.cpp
 *  @brief     The file dependencies section in each release's text form, and lists that would not write back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/sections/dependencies.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "support/sections.hpp"

#include "dwgio/dependencies.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace DwgIO::File::Sections
{
namespace
{
const Encoding::Scheme Latin = Encoding::MakeScheme(Encoding::Standard());

/**
 *  The list of AutoCAD 2007's break line symbol: the font a text style needs, found where AutoCAD 2007 keeps it.
 */
auto MakeList() -> FileDependencies
{
  return FileDependencies{.Features = {"Acad:Text", "Acad:PlotConfig"},
                          .Files = {FileDependency{.FullFilename = "txt.shx",
                                                   .FoundPath = "C:\\Program Files\\AutoCAD 2007\\fonts\\",
                                                   .FingerprintGuid = "",
                                                   .VersionGuid = "",
                                                   .FeatureIndex = 0,
                                                   .Timestamp = 652373610,
                                                   .Filesize = 8489,
                                                   .AffectsGraphics = true,
                                                   .ReferenceCount = 1}}};
}
} // namespace

TEST_CASE("file/sections/dependencies: from R2007 each text is UTF-16 with a long byte count")
{
  const std::vector<std::byte> bytes = WriteDependencies(MakeList(), Version::R2007, Latin);
  // The first 46 bytes of the list of AutoCAD 2007's R2007 break line symbol, and its size.
  const std::vector<std::byte> head = Octets(
      {0x02, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 'A', 0, 'c', 0, 'a', 0, 'd', 0, ':', 0, 'T', 0, 'e', 0, 'x', 0,
       't',  0,    0x1e, 0x00, 0x00, 0x00, 'A',  0,    'c', 0, 'a', 0, 'd', 0, ':', 0, 'P', 0, 'l', 0, 'o', 0});
  CHECK(std::vector<std::byte>(bytes.begin(), bytes.begin() + 46) == head);
  CHECK(bytes.size() == 184);
  CHECK(ReadDependencies(bytes, Version::R2007, Latin) == MakeList());
}

TEST_CASE("file/sections/dependencies: below R2007 the code page's bytes read back")
{
  FileDependencies list = MakeList();
  list.Files.front().FoundPath = "C:\\Caf\xc3\xa9\\";
  // An image that does not affect the graphics.
  list.Features.emplace_back("Acad:Image");
  list.Files.push_back(FileDependency{.FullFilename = "dot.png",
                                      .FoundPath = "",
                                      .FingerprintGuid = "",
                                      .VersionGuid = "",
                                      .FeatureIndex = 2,
                                      .Timestamp = 3,
                                      .Filesize = 73,
                                      .AffectsGraphics = false,
                                      .ReferenceCount = 2});
  const std::vector<std::byte> bytes = WriteDependencies(list, Version::R2004, Latin);
  CHECK(bytes[4] == std::byte{9});
  CHECK(bytes[8] == std::byte{'A'});
  CHECK(bytes[9] == std::byte{'c'});
  CHECK(ReadDependencies(bytes, Version::R2004, Latin) == list);
  CHECK(ReadDependencies(WriteDependencies(FileDependencies{}, Version::R2004, Latin), Version::R2004, Latin) ==
        FileDependencies{});
}

TEST_CASE("file/sections/dependencies: bytes that would not write back unchanged give no list")
{
  const std::vector<std::byte> good = WriteDependencies(MakeList(), Version::R2007, Latin);
  std::vector<std::byte> cut = good;
  cut.pop_back();
  CHECK_FALSE(ReadDependencies(cut, Version::R2007, Latin));
  std::vector<std::byte> longer = good;
  longer.push_back(std::byte{0});
  CHECK_FALSE(ReadDependencies(longer, Version::R2007, Latin));
  // The graphics neither 0 nor 1.
  std::vector<std::byte> graphics = good;
  graphics[graphics.size() - 6] = std::byte{2};
  CHECK_FALSE(ReadDependencies(graphics, Version::R2007, Latin));
  // A text of an odd count of bytes in UTF-16.
  std::vector<std::byte> odd = good;
  odd[4] = std::byte{0x13};
  CHECK_FALSE(ReadDependencies(odd, Version::R2007, Latin));
  // Counts past the bytes stop with them.
  CHECK_FALSE(ReadDependencies(Octets({0xff, 0xff, 0xff, 0xff}), Version::R2007, Latin));
  CHECK_FALSE(ReadDependencies(Octets({0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff}), Version::R2004, Latin));
}
} // namespace DwgIO::File::Sections
