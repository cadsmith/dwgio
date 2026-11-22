/***********************************************************************************************************************
 *  @file      tests/dwgio/support/sections.hpp
 *  @brief     A section of a paged or coded fixture, by name.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_SECTIONS_HPP
#define DWGIO_TESTS_SUPPORT_SECTIONS_HPP

#include "file/coded/layout.hpp"
#include "file/paged/layout.hpp"
#include "support/fixtures.hpp"

#include "dwgio/issue.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace DwgIO::File::Sections
{
/**
 *  The bytes of a section of a fixture AutoCAD wrote, from R2004 on, or none when it has no such section.
 */
inline auto SectionOf(std::string_view fixture, std::string_view name) -> std::optional<std::vector<std::byte>>
{
  const std::vector<std::byte> file = ReadFixture(fixture);
  std::vector<Issue> issues;
  std::vector<Paged::Section> sections;
  if (fixture.contains("r2007"))
  {
    sections = Coded::ReadLayout(file, issues)->Sections;
  }
  else
  {
    sections = Paged::ReadLayout(file, issues)->Sections;
  }
  const auto found = std::ranges::find(sections, name, &Paged::Section::Name);
  return found == sections.end() ? std::nullopt : std::optional{found->Bytes};
}

inline auto Octets(std::initializer_list<int> values) -> std::vector<std::byte>
{
  std::vector<std::byte> bytes;
  for (const int one : values)
  {
    bytes.push_back(static_cast<std::byte>(one));
  }
  return bytes;
}
} // namespace DwgIO::File::Sections

#endif /* DWGIO_TESTS_SUPPORT_SECTIONS_HPP */
