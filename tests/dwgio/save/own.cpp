/***********************************************************************************************************************
 *  @file      tests/dwgio/save/own.cpp
 *  @brief     Drawings saved to their own release, against AutoCAD's bytes.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/codepage.hpp"
#include "file/coded/layout.hpp"
#include "file/locator/layout.hpp"
#include "file/paged/essential.hpp"
#include "file/paged/layout.hpp"
#include "file/sections/information.hpp"
#include "file/sections/summary.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/section.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <string_view>
#include <vector>

namespace DwgIO
{
namespace
{
auto FirstDifference(std::span<const std::byte> one, std::span<const std::byte> two) -> std::optional<std::size_t>
{
  const std::size_t least = std::min(one.size(), two.size());
  for (std::size_t at = 0; at < least; ++at)
  {
    if (one[at] != two[at])
    {
      return at;
    }
  }
  return one.size() == two.size() ? std::nullopt : std::optional{least};
}
} // namespace

TEST_CASE("save/own: an AutoCAD R2000 drawing writes back byte for byte")
{
  const std::vector<std::byte> bytes = ReadFixture("foundation-r2000.dwg");
  const std::expected<Loaded, Failure> loaded = Load(bytes);
  REQUIRE(loaded);
  const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  const std::optional<std::size_t> differs = FirstDifference(saved->Bytes, bytes);
  CAPTURE(differs.value_or(0));
  CAPTURE(saved->Bytes.size());
  CHECK_FALSE(differs);
}

TEST_CASE("save/own: every R14 and R2000 fixture saves byte for byte to its own release")
{
  for (const std::string_view name : {"tables-r2000.dwg", "entities-r2000.dwg", "curves-r2000.dwg", "objects-r2000.dwg",
                                      "empty-r2000.dwg", "foundation-r14.dwg", "tables-r14.dwg", "entities-r14.dwg",
                                      "curves-r14.dwg", "objects-r14.dwg", "empty-r14.dwg"})
  {
    CAPTURE(name);
    const std::vector<std::byte> bytes = ReadFixture(name);
    const std::expected<Loaded, Failure> loaded = Load(bytes);
    REQUIRE(loaded);
    // A save to the other release first leaves the drawing as it was, and its own save still byte for byte.
    const Drawing before = loaded->Drawing;
    const Version other = loaded->Drawing.Release == Version::R2000 ? Version::R14 : Version::R2000;
    REQUIRE(Save(loaded->Drawing, SaveOptions{.Release = other}));
    CHECK(loaded->Drawing == before);
    const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::optional<std::size_t> differs = FirstDifference(saved->Bytes, bytes);
    CAPTURE(differs.value_or(0));
    CHECK_FALSE(differs);
  }
}

TEST_CASE("save/own: the entities fixture saves byte for byte")
{
  const std::vector<std::byte> bytes = ReadFixture("entities-r2000.dwg");
  const std::expected<Loaded, Failure> loaded = Load(bytes);
  REQUIRE(loaded);
  const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
  REQUIRE(saved);
  CHECK(saved->Issues.empty());
  CHECK(saved->Bytes == bytes);
}

TEST_CASE("save/own: an R2004 fixture saved as R2004 loads back equal section for section")
{
  for (const std::string_view name : {"empty", "entities", "tables", "curves", "objects", "foundation"})
  {
    CAPTURE(name);
    const std::vector<std::byte> bytes = ReadFixture(std::format("{}-r2004.dwg", name));
    const std::expected<Loaded, Failure> loaded = Load(bytes);
    REQUIRE(loaded);
    const Drawing &first = loaded->Drawing;
    const std::expected<Saved, Failure> saved = Save(first);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> again = Load(saved->Bytes);
    REQUIRE(again);
    CHECK(again->Issues.empty());
    CHECK(again->Drawing == first);
    std::vector<Issue> issues;
    const File::Paged::Layout theirs = *File::Paged::ReadLayout(bytes, issues);
    const File::Paged::Layout ours = *File::Paged::ReadLayout(saved->Bytes, issues);
    REQUIRE(ours.Sections.size() == theirs.Sections.size());
    for (std::size_t at = 0; at < theirs.Sections.size(); ++at)
    {
      CAPTURE(theirs.Sections[at].Name);
      CHECK(ours.Sections[at].Name == theirs.Sections[at].Name);
      CHECK(ours.Sections[at].Id == theirs.Sections[at].Id);
      CHECK(ours.Sections[at].Compressed == theirs.Sections[at].Compressed);
      CHECK(ours.Sections[at].PageSize == theirs.Sections[at].PageSize);
      const auto [mine, its] = std::ranges::mismatch(ours.Sections[at].Bytes, theirs.Sections[at].Bytes);
      CAPTURE(mine - ours.Sections[at].Bytes.begin());
      CHECK((RemakesSection(theirs.Sections[at].Name) || ours.Sections[at].Bytes == theirs.Sections[at].Bytes));
    }
    CHECK(ours.Head.Maintenance == theirs.Head.Maintenance);
    CHECK(ours.Head.Image == theirs.Head.Image);
    CHECK(ours.Head.Summary == theirs.Head.Summary);
    QueueAudit(saved->Bytes, std::format("dwgio-{}-r2004-again.dwg", name));
  }
}

TEST_CASE("save/own: every R2010 to R2018 fixture saved to its release loads back equal section for section")
{
  for (const std::string_view year : {"2010", "2013", "2018"})
  {
    for (const std::string_view name : {"empty", "entities", "tables", "curves", "objects", "foundation"})
    {
      CAPTURE(name);
      CAPTURE(year);
      const std::vector<std::byte> bytes = ReadFixture(std::format("{}-r{}.dwg", name, year));
      const std::expected<Loaded, Failure> loaded = Load(bytes);
      REQUIRE(loaded);
      const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
      REQUIRE(saved);
      CHECK(saved->Issues.empty());
      const std::expected<Loaded, Failure> again = Load(saved->Bytes);
      REQUIRE(again);
      CHECK(again->Issues.empty());
      CHECK(again->Drawing == loaded->Drawing);
      std::vector<Issue> issues;
      const File::Paged::Layout theirs = *File::Paged::ReadLayout(bytes, issues);
      const File::Paged::Layout ours = *File::Paged::ReadLayout(saved->Bytes, issues);
      REQUIRE(ours.Sections.size() == theirs.Sections.size());
      for (std::size_t at = 0; at < theirs.Sections.size(); ++at)
      {
        CAPTURE(theirs.Sections[at].Name);
        CHECK(ours.Sections[at].Name == theirs.Sections[at].Name);
        CHECK(ours.Sections[at].PageSize == theirs.Sections[at].PageSize);
        CHECK((RemakesSection(theirs.Sections[at].Name) || ours.Sections[at].Bytes == theirs.Sections[at].Bytes));
      }
      CHECK(ours.Head.Maintenance == theirs.Head.Maintenance);
      CHECK(ours.Head.MaintRelease == theirs.Head.MaintRelease);
      CHECK(ours.Head.Repeat == theirs.Head.Repeat);
      CHECK(ours.Head.Image == theirs.Head.Image);
      CHECK(ours.Head.Summary == theirs.Head.Summary);
      CHECK(issues.empty());
      if (name == "curves")
      {
        QueueAudit(saved->Bytes, std::format("dwgio-{}-r{}-again.dwg", name, year));
      }
    }
  }
}

TEST_CASE("save/own: every R2007 fixture saved as R2007 loads back equal section for section")
{
  for (const std::string_view name : {"empty", "entities", "tables", "curves", "objects", "foundation"})
  {
    CAPTURE(name);
    const std::vector<std::byte> bytes = ReadFixture(std::format("{}-r2007.dwg", name));
    const std::expected<Loaded, Failure> loaded = Load(bytes);
    REQUIRE(loaded);
    const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> again = Load(saved->Bytes);
    REQUIRE(again);
    CHECK(again->Issues.empty());
    CHECK(again->Drawing == loaded->Drawing);
    std::vector<Issue> issues;
    const File::Coded::Layout theirs = *File::Coded::ReadLayout(bytes, issues);
    const File::Coded::Layout ours = *File::Coded::ReadLayout(saved->Bytes, issues);
    CHECK(issues.empty());
    REQUIRE(ours.Sections.size() == theirs.Sections.size());
    for (std::size_t at = 0; at < theirs.Sections.size(); ++at)
    {
      CAPTURE(theirs.Sections[at].Name);
      CHECK(ours.Sections[at].Name == theirs.Sections[at].Name);
      if (theirs.Sections[at].Name != "preview") // Its images' starts are file addresses, and ours differ.
      {
        CHECK((RemakesSection(theirs.Sections[at].Name) || ours.Sections[at].Bytes == theirs.Sections[at].Bytes));
      }
      CHECK(ours.Descriptions[at].Hash == theirs.Descriptions[at].Hash);
    }
    CHECK(ours.Meta.Maintenance == theirs.Meta.Maintenance);
    CHECK(ours.Meta.Codepage == theirs.Meta.Codepage);
    CHECK(saved->Bytes.size() < bytes.size()); // AutoCAD stores the preview and the information uncompressed.
    if (name == "curves")
    {
      QueueAudit(saved->Bytes, "dwgio-curves-r2007-again.dwg");
    }
  }
}

TEST_CASE("save/own: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
