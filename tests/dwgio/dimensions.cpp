/***********************************************************************************************************************
 *  @file      tests/dwgio/dimensions.cpp
 *  @brief     The arc length and jogged radius dimensions and the page setup in every release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/coded/layout.hpp"
#include "file/paged/layout.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/dimension.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/plotsettings.hpp"
#include "dwgio/save.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  The arc length dimension, the jogged radius dimension and the page setup, under the same handles in every release.
 */
constexpr std::array<Handle, 3> Drawn{Handle{.Value = 0x2CD}, Handle{.Value = 0x2ED}, Handle{.Value = 0x303}};

/**
 *  How many objects of the dimensions fixture are of the three kinds it draws.
 */
auto CountKinds(const Drawing &drawing) -> std::size_t
{
  return static_cast<std::size_t>(std::ranges::count_if(drawing.Objects,
                                                        [](const Object &object) -> bool
                                                        {
                                                          return Holds<Entities::ArcDimension>(object) ||
                                                                 Holds<Entities::LargeRadialDimension>(object) ||
                                                                 Holds<Objects::PlotSettings>(object);
                                                        }));
}

/**
 *  The findings of the library's own audit, joined.
 */
auto Findings(const Drawing &drawing) -> std::string
{
  std::string findings;
  for (const Audit::Finding &finding : Audit::Check(drawing).Findings)
  {
    findings += finding.What + "; ";
  }
  return findings;
}
} // namespace

TEST_CASE("dimensions: every release of the fixture types its two dimensions and its page setup")
{
  for (const std::string_view release : FixtureSuffixes)
  {
    CAPTURE(release);
    const std::expected<Loaded, Failure> loaded = Load(ReadFixture(std::format("dimensions-{}.dwg", release)));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    CHECK(CountKinds(loaded->Drawing) == 3);
    const std::string findings = Findings(loaded->Drawing);
    CHECK_MESSAGE(findings.empty(), findings);
  }
}

TEST_CASE("dimensions: the R14 and R2000 fixtures save byte for byte to their own release")
{
  for (const std::string_view release : {"r14", "r2000"})
  {
    CAPTURE(release);
    const std::vector<std::byte> bytes = ReadFixture(std::format("dimensions-{}.dwg", release));
    const std::expected<Loaded, Failure> loaded = Load(bytes);
    REQUIRE(loaded);
    const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const auto [ours, theirs] = std::ranges::mismatch(saved->Bytes, bytes);
    CAPTURE(ours - saved->Bytes.begin());
    CHECK(saved->Bytes == bytes);
  }
}

TEST_CASE("dimensions: the R2004 R2010 R2013 and R2018 fixtures save to their own release section for section")
{
  for (const std::string_view release : {"r2004", "r2010", "r2013", "r2018"})
  {
    CAPTURE(release);
    const std::vector<std::byte> bytes = ReadFixture(std::format("dimensions-{}.dwg", release));
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
    CHECK(issues.empty());
    QueueAudit(saved->Bytes, std::format("dwgio-dimensions-{}-again.dwg", release));
  }
}

TEST_CASE("dimensions: the R2007 fixture saves to R2007 section for section")
{
  const std::vector<std::byte> bytes = ReadFixture("dimensions-r2007.dwg");
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
  }
  QueueAudit(saved->Bytes, "dwgio-dimensions-r2007-again.dwg");
}

TEST_CASE("dimensions: the fixture saved to every other release keeps its three kinds and loads back clean")
{
  struct Crossing
  {
    std::string_view From;
    Version To;
  };

  for (const Crossing crossing :
       {Crossing{.From = "r2018", .To = Version::R2000}, Crossing{.From = "r2000", .To = Version::R2018},
        Crossing{.From = "r2007", .To = Version::R2013}, Crossing{.From = "r2004", .To = Version::R14},
        Crossing{.From = "r14", .To = Version::R2010}, Crossing{.From = "r2013", .To = Version::R2007},
        Crossing{.From = "r2010", .To = Version::R2004}, Crossing{.From = "r2000", .To = Version::R14}})
  {
    CAPTURE(crossing.From);
    CAPTURE(NameOf(crossing.To));
    const Drawing drawing = Load(ReadFixture(std::format("dimensions-{}.dwg", crossing.From))).value().Drawing;
    const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = crossing.To});
    REQUIRE(saved);
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(back->Drawing.Release == crossing.To);
    CHECK(CountKinds(back->Drawing) == 3);
    // What is noted is of the kinds the library leaves out of another release, never of the three.
    for (const Issue &issue : saved->Issues)
    {
      CAPTURE(issue.What);
      CHECK_FALSE(std::ranges::contains(Drawn, issue.Subject));
    }
    QueueAudit(saved->Bytes, std::format("dwgio-dimensions-{}-as-{}.dwg", crossing.From, NameOf(crossing.To)));
  }
}

TEST_CASE("dimensions: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
