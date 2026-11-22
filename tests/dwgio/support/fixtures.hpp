/***********************************************************************************************************************
 *  @file      tests/dwgio/support/fixtures.hpp
 *  @brief     The fixtures AutoCAD wrote: their bytes, their drawings, their names by release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_FIXTURES_HPP
#define DWGIO_TESTS_SUPPORT_FIXTURES_HPP

#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/load.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <fstream>
#include <ios>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO
{
/**
 *  The whole of a file, which must exist.
 */
inline auto ReadBytes(const std::string &path) -> std::vector<std::byte>
{
  std::ifstream file{path, std::ios::binary | std::ios::ate};
  REQUIRE_MESSAGE(file, path);
  std::vector<std::byte> bytes(static_cast<std::size_t>(file.tellg()));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return bytes;
}

/**
 *  The whole of a fixture AutoCAD wrote, by its file name under tests/fixtures/acad.
 */
inline auto ReadFixture(std::string_view name) -> std::vector<std::byte>
{
  return ReadBytes(std::string{DWGIO_FIXTURES_DIR "/acad/"} + std::string{name});
}

/**
 *  The releases of the fixtures, oldest first: each script saves one fixture per release.
 */
constexpr std::array<Version, 7> FixtureReleases{Version::R14,   Version::R2000, Version::R2004, Version::R2007,
                                                 Version::R2010, Version::R2013, Version::R2018};

/**
 *  The suffix a fixture's file name takes for each release of FixtureReleases, in the same order.
 */
constexpr std::array<std::string_view, 7> FixtureSuffixes{"r14", "r2000", "r2004", "r2007", "r2010", "r2013", "r2018"};

inline auto SuffixOf(Version release) -> std::string
{
  const auto at = std::ranges::find(FixtureReleases, release) - FixtureReleases.begin();
  return std::string{FixtureSuffixes.at(static_cast<std::size_t>(at))};
}

/**
 *  The file name of a fixture by its script's stem and its release: "kinds-r2000.dwg".
 */
inline auto FileOf(std::string_view stem, Version release) -> std::string
{
  return std::string{stem} + "-" + SuffixOf(release) + ".dwg";
}

/**
 *  A fixture loaded, which must succeed; its issues are left to the caller.
 */
inline auto LoadFixture(std::string_view name) -> Loaded
{
  CAPTURE(name);
  std::expected<Loaded, Failure> loaded = Load(ReadFixture(name));
  REQUIRE(loaded);
  return std::move(*loaded);
}

/**
 *  The drawing of a fixture that must load with no issue.
 */
inline auto LoadDrawing(std::string_view name) -> Drawing
{
  Loaded loaded = LoadFixture(name);
  REQUIRE(loaded.Issues.empty());
  return std::move(loaded.Drawing);
}

inline auto StringOf(const std::vector<std::byte> &bytes) -> std::string
{
  return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
}

/**
 *  Whether a save remakes the section: the application information names the program that saved the file.
 */
inline auto RemakesSection(std::string_view section) -> bool
{
  return section == "AcDb:AppInfo";
}
} // namespace DwgIO

#endif /* DWGIO_TESTS_SUPPORT_FIXTURES_HPP */
