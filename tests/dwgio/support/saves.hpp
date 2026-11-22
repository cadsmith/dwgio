/***********************************************************************************************************************
 *  @file      tests/dwgio/support/saves.hpp
 *  @brief     A fixture saved to its own release or another, checked against AutoCAD's bytes or audit.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TESTS_SUPPORT_SAVES_HPP
#define DWGIO_TESTS_SUPPORT_SAVES_HPP

#include "file/coded/layout.hpp"
#include "file/paged/layout.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/save.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace DwgIO
{
/**
 *  Checks a fixture saved to its own release: it loads back equal, and its bytes are AutoCAD's, the whole file up to
 *  R2000, each section from R2004, each but R2007's preview, whose images' starts are file addresses.
 */
inline void CheckSameSave(std::string_view name)
{
  CAPTURE(name);
  const std::vector<std::byte> bytes = ReadFixture(name);
  const std::expected<Loaded, Failure> loaded = Load(bytes);
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::expected<Loaded, Failure> again = Load(saved->Bytes);
  REQUIRE(again);
  CHECK(again->Issues.empty());
  CHECK(again->Drawing == loaded->Drawing);
  const Version release = loaded->Drawing.Release;
  std::vector<Issue> issues;
  if (release <= Version::R2000)
  {
    CHECK(saved->Bytes == bytes);
  }
  else if (release == Version::R2007)
  {
    const File::Coded::Layout theirs = *File::Coded::ReadLayout(bytes, issues);
    const File::Coded::Layout ours = *File::Coded::ReadLayout(saved->Bytes, issues);
    REQUIRE(ours.Sections.size() == theirs.Sections.size());
    for (std::size_t at = 0; at < theirs.Sections.size(); ++at)
    {
      CAPTURE(theirs.Sections[at].Name);
      if (theirs.Sections[at].Name != "preview")
      {
        CHECK((RemakesSection(theirs.Sections[at].Name) || ours.Sections[at].Bytes == theirs.Sections[at].Bytes));
      }
    }
  }
  else
  {
    const File::Paged::Layout theirs = *File::Paged::ReadLayout(bytes, issues);
    const File::Paged::Layout ours = *File::Paged::ReadLayout(saved->Bytes, issues);
    REQUIRE(ours.Sections.size() == theirs.Sections.size());
    for (std::size_t at = 0; at < theirs.Sections.size(); ++at)
    {
      CAPTURE(theirs.Sections[at].Name);
      CHECK(ours.Sections[at].Name == theirs.Sections[at].Name);
      CHECK((RemakesSection(theirs.Sections[at].Name) || ours.Sections[at].Bytes == theirs.Sections[at].Bytes));
    }
  }
  CHECK(issues.empty());
}

/**
 *  Checks a fixture saved to another release: it loads back with no issue, and it is queued for AutoCAD's AUDIT.
 */
inline auto CheckSaveAs(std::string_view from, Version to) -> Drawing
{
  CAPTURE(from);
  CAPTURE(NameOf(to));
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture(from));
  REQUIRE(loaded);
  const std::expected<Saved, Failure> saved = Save(loaded->Drawing, SaveOptions{.Release = to});
  REQUIRE(saved);
  const std::expected<Loaded, Failure> back = Load(saved->Bytes);
  REQUIRE(back);
  CHECK(back->Issues.empty());
  CHECK(back->Drawing.Release == to);
  std::string stem{from};
  stem.resize(stem.size() - std::string_view{".dwg"}.size());
  QueueAudit(saved->Bytes, std::format("dwgio-{}-as-{}.dwg", stem, NameOf(to)));
  return back->Drawing;
}
} // namespace DwgIO

#endif /* DWGIO_TESTS_SUPPORT_SAVES_HPP */
