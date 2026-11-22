/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/samples.cpp
 *  @brief     AutoCAD's own sample drawings loaded, saved to every release and audited.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/fixtures.hpp"
#include "support/styles.hpp"
#include "support/summary.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/save.hpp"
#include "dwgio/summary.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
/**
 *  AutoCAD's English sample drawings: the dynamic block libraries, the title blocks and the break line symbol.
 */
auto Samples() -> std::vector<std::filesystem::path>
{
  std::vector<std::filesystem::path> samples;
  const std::filesystem::path english = FindResources() / "UserDataCache" / "en-us";
  for (const std::filesystem::path &folder :
       {english / "Support" / "ContentLibrary" / "DynamicBlocks", english / "Template"})
  {
    if (std::filesystem::is_directory(folder))
    {
      for (const auto &entry : std::filesystem::directory_iterator{folder})
      {
        if (entry.path().extension() == ".dwg")
        {
          samples.push_back(entry.path());
        }
      }
    }
  }
  if (std::filesystem::exists(FindResources() / "Support" / "brkline.dwg"))
  {
    samples.push_back(FindResources() / "Support" / "brkline.dwg");
  }
  std::ranges::sort(samples);
  return samples;
}

/**
 *  Every drawing among AutoCAD's resources, in every language: the dynamic block libraries, the title blocks, the
 *  templates and the symbols.
 */
auto Everything() -> std::vector<std::filesystem::path>
{
  std::vector<std::filesystem::path> drawings;
  if (std::filesystem::is_directory(FindResources()))
  {
    for (const auto &entry : std::filesystem::recursive_directory_iterator{FindResources()})
    {
      if (entry.path().extension() == ".dwg")
      {
        drawings.push_back(entry.path());
      }
    }
  }
  std::ranges::sort(drawings);
  return drawings;
}

/**
 *  A file's name without spaces, for the temporary directory.
 */
auto Plain(const std::filesystem::path &file) -> std::string
{
  std::string name = file.stem().string();
  std::ranges::replace(name, ' ', '_');
  return name;
}

/**
 *  AutoCAD's own save of a sample to a release, made once through its console under the temporary directory.
 */
auto Oracle(const std::filesystem::path &sample, Version release) -> std::vector<std::byte>
{
  const std::filesystem::path place = std::filesystem::temp_directory_path() / "dwgio-oracle";
  std::filesystem::create_directories(place);
  const std::string year = release == Version::R14 ? "R14" : std::string{NameOf(release).substr(1)};
  const std::filesystem::path saved = place / std::format("{}-{}.dwg", Plain(sample), year);
  if (!std::filesystem::exists(saved))
  {
    const std::filesystem::path script = place / std::format("{}-{}.scr", Plain(sample), year);
    {
      std::ofstream lines{script};
      lines << "FILEDIA 0\nCMDECHO 0\n_.SAVEAS _" << year << " \"" << saved.string() << "\"\n_.QUIT _Y\n";
    }
    RunScript(script, sample);
  }
  return ReadBytes(saved.string());
}
} // namespace

TEST_CASE("releases/samples: every drawing in AutoCAD's resources loads clean and saves back equal" *
          doctest::skip(!HasSamples()))
{
  for (const std::filesystem::path &sample : Everything())
  {
    CAPTURE(sample.string());
    const std::expected<Loaded, Failure> loaded = Load(ReadBytes(sample.string()));
    REQUIRE(loaded);
    CHECK(loaded->Issues.empty());
    const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
    REQUIRE(saved);
    CHECK(saved->Issues.empty());
    const std::expected<Loaded, Failure> back = Load(saved->Bytes);
    REQUIRE(back);
    CHECK(back->Issues.empty());
    CHECK(back->Drawing == loaded->Drawing);
  }
}

TEST_CASE("releases/samples: the English samples saved to every other release load back clean" *
          doctest::skip(!HasSamples()))
{
  for (const std::filesystem::path &sample : Samples())
  {
    const std::expected<Loaded, Failure> loaded = Load(ReadBytes(sample.string()));
    REQUIRE(loaded);
    const Drawing &drawing = loaded->Drawing;
    for (const Version release : FixtureReleases)
    {
      if (release == drawing.Release)
      {
        continue;
      }
      CAPTURE(std::format("{} as {}", sample.filename().string(), NameOf(release)));
      const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = release});
      REQUIRE(saved);
      CHECK(KeepsStyles(drawing, saved->Issues));
      const std::expected<Loaded, Failure> back = Load(saved->Bytes);
      REQUIRE(back);
      CHECK(back->Issues.empty());
      // The summary in every release, the file dependencies in every release that has a place for them.
      REQUIRE(drawing.SummaryInfo);
      // Below R2004 the record keeps the dates to the second, as AutoCAD writes them.
      SummaryInfo summary = *drawing.SummaryInfo;
      if (release < Version::R2004)
      {
        summary.CreateDateTimeMilliseconds -= summary.CreateDateTimeMilliseconds % 1000;
        summary.ModifiedDateTimeMilliseconds -= summary.ModifiedDateTimeMilliseconds % 1000;
      }
      CHECK(back->Drawing.SummaryInfo == summary);
      CHECK(back->Drawing.FileDependencies == (release >= Version::R2004 ? drawing.FileDependencies : std::nullopt));
      for (const Object &object : drawing.Objects)
      {
        const auto *unknown = GetIf<Unknown>(&object);
        if (unknown == nullptr || release < drawing.Release)
        {
          continue; // Downgrading, what is kept follows AutoCAD's own saves.
        }
        CAPTURE(std::format("{} {}", TypeOf(object), HexOf(unknown->Base.Self)));
        CHECK(Find(back->Drawing, unknown->Base.Self) != nullptr);
      }
    }
  }
}

TEST_CASE("releases/samples: each sample saved to each release keeps the untyped objects AutoCAD keeps" *
          doctest::skip(!IsVerifying() || !HasSamples()))
{
  for (const std::filesystem::path &sample : Samples())
  {
    const Drawing drawing = Load(ReadBytes(sample.string())).value().Drawing;
    for (const Version release : FixtureReleases)
    {
      if (release == drawing.Release)
      {
        continue;
      }
      CAPTURE(std::format("{} as {}", sample.filename().string(), NameOf(release)));
      const std::expected<Loaded, Failure> theirs = Load(Oracle(sample, release));
      REQUIRE(theirs);
      const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = release});
      REQUIRE(saved);
      const Drawing ours = Load(saved->Bytes).value().Drawing;
      for (const Object &object : theirs->Drawing.Objects)
      {
        const auto *their = GetIf<Unknown>(&object);
        const Object *original = their != nullptr ? Find(drawing, their->Base.Self) : nullptr;
        if (original == nullptr || TypeOf(*original) != TypeOf(object))
        {
          continue;
        }
        CAPTURE(std::format("{} {}", TypeOf(object), HexOf(their->Base.Self)));
        const Object *mine = Find(ours, their->Base.Self);
        REQUIRE(mine != nullptr);
        CHECK(TypeOf(*mine) == TypeOf(object));
      }
      CheckStyles(ours, theirs->Drawing, drawing);
      // AutoCAD's summary is the drawing's, but for what it sets at each save.
      REQUIRE(theirs->Drawing.SummaryInfo);
      CHECK(ClearSaveStamps(*theirs->Drawing.SummaryInfo) == ClearSaveStamps(*ours.SummaryInfo));
      QueueAudit(saved->Bytes, std::format("dwgio-{}-as-{}.dwg", Plain(sample), NameOf(release)));
    }
  }
}

TEST_CASE("releases/samples: the Spanish samples keep their constraints' associative network across R2018" *
          doctest::skip(!HasSamples()))
{
  const std::filesystem::path spanish =
      FindResources() / "UserDataCache" / "es" / "Support" / "ContentLibrary" / "DynamicBlocks";
  for (const std::string_view name : {"Architectural - Imperial.dwg", "Mechanical - Metric.dwg"})
  {
    const std::filesystem::path sample = spanish / name;
    if (!std::filesystem::exists(sample))
    {
      continue;
    }
    CAPTURE(sample.filename().string());
    const Drawing drawing = Load(ReadBytes(sample.string())).value().Drawing;
    REQUIRE((drawing.Release == Version::R2013 || drawing.Release == Version::R2018));
    const Version other = drawing.Release == Version::R2013 ? Version::R2018 : Version::R2013;
    const std::expected<Saved, Failure> there = Save(drawing, SaveOptions{.Release = other});
    REQUIRE(there);
    CHECK(there->Issues.empty());
    const std::expected<Saved, Failure> again =
        Save(Load(there->Bytes).value().Drawing, SaveOptions{.Release = drawing.Release});
    REQUIRE(again);
    CHECK(again->Issues.empty());
    const Drawing back = Load(again->Bytes).value().Drawing;
    std::size_t groups = 0;
    for (const Object &object : drawing.Objects)
    {
      const auto *unknown = GetIf<Unknown>(&object);
      const std::string_view kind = unknown != nullptr ? unknown->Class.View() : std::string_view{};
      if (!kind.starts_with("ACDBASSOC") && !kind.ends_with("DEPENDENCYBODY"))
      {
        continue;
      }
      groups += kind == "ACDBASSOC2DCONSTRAINTGROUP" ? 1U : 0U;
      CAPTURE(std::format("{} {}", TypeOf(object), HexOf(unknown->Base.Self)));
      const Object *ours = Find(back, unknown->Base.Self);
      REQUIRE(ours != nullptr);
      CHECK(GetIf<Unknown>(ours)->Body == unknown->Body);
    }
    CHECK(groups > 0);
    QueueAudit(there->Bytes, std::format("dwgio-es-{}-as-{}.dwg", Plain(sample), NameOf(other)));
    QueueAudit(again->Bytes, std::format("dwgio-es-{}-and-back.dwg", Plain(sample)));
  }
}

TEST_CASE("releases/samples: an AutoCAD downgrade upgraded again keeps the untyped objects both saves keep" *
          doctest::skip(!IsVerifying() || !HasSamples()))
{
  for (const std::filesystem::path &sample : Samples())
  {
    for (std::size_t low = 0; low < FixtureReleases.size(); ++low)
    {
      const std::expected<Loaded, Failure> older = Load(Oracle(sample, FixtureReleases.at(low)));
      REQUIRE(older);
      for (std::size_t high = low + 1; high < FixtureReleases.size(); ++high)
      {
        CAPTURE(std::format("{} from {} as {}", sample.filename().string(), NameOf(FixtureReleases.at(low)),
                            NameOf(FixtureReleases.at(high))));
        const std::expected<Loaded, Failure> theirs = Load(Oracle(sample, FixtureReleases.at(high)));
        REQUIRE(theirs);
        const std::expected<Saved, Failure> saved =
            Save(older->Drawing, SaveOptions{.Release = FixtureReleases.at(high)});
        REQUIRE(saved);
        const std::expected<Loaded, Failure> ours = Load(saved->Bytes);
        REQUIRE(ours);
        CHECK(ours->Issues.empty());
        for (const Object &object : theirs->Drawing.Objects)
        {
          const auto *their = GetIf<Unknown>(&object);
          const Object *was = their != nullptr ? Find(older->Drawing, their->Base.Self) : nullptr;
          if (was == nullptr || TypeOf(*was) != TypeOf(object))
          {
            continue;
          }
          CAPTURE(std::format("{} {}", TypeOf(object), HexOf(their->Base.Self)));
          CHECK(Find(ours->Drawing, their->Base.Self) != nullptr);
        }
      }
    }
  }
}

TEST_CASE("releases/samples: the drawings the cases saved pass AutoCAD's AUDIT" *
          doctest::skip(!IsVerifying() || !HasSamples()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO
