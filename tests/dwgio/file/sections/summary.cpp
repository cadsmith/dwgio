/***********************************************************************************************************************
 *  @file      tests/dwgio/file/sections/summary.cpp
 *  @brief     The summary section in each release's text form, and summaries that would not write back.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/sections/summary.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "support/sections.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/load.hpp"
#include "dwgio/summary.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace DwgIO::File::Sections
{
namespace
{
const Encoding::Scheme Latin = Encoding::MakeScheme(Encoding::Standard());

/**
 *  A summary of every field, text of the page and beyond it among them.
 */
auto MakeSummary() -> SummaryInfo
{
  return SummaryInfo{.Title = "Caf\xc3\xa9 \xe3\x82\xbf",
                     .Subject = "Subject",
                     .Author = "Author",
                     .Keywords = "one, two",
                     .Comments = "Line one",
                     .LastSavedBy = "someone",
                     .RevisionNumber = "7",
                     .HyperlinkBase = "http://example.com/",
                     .TotalEditingTimeDays = 1,
                     .TotalEditingTimeMilliseconds = 2,
                     .CreateDateTimeDays = 2461352,
                     .CreateDateTimeMilliseconds = 29248000,
                     .ModifiedDateTimeDays = 2461358,
                     .ModifiedDateTimeMilliseconds = 7395000,
                     .Properties = {{.Name = "Client", .Value = "ACME"}, {.Name = "Empty", .Value = ""}}};
}
} // namespace

TEST_CASE("file/sections/summary: the summary of every release from R2004 writes back byte for byte")
{
  for (const auto &[fixture, release] :
       {std::pair{"empty-r2004.dwg", Version::R2004}, std::pair{"empty-r2007.dwg", Version::R2007},
        std::pair{"empty-r2010.dwg", Version::R2010}, std::pair{"empty-r2013.dwg", Version::R2013},
        std::pair{"curves-r2018.dwg", Version::R2018}})
  {
    CAPTURE(fixture);
    const std::optional<std::vector<std::byte>> bytes = SectionOf(fixture, "AcDb:SummaryInfo");
    REQUIRE(bytes);
    const std::optional<SummaryInfo> summary = ReadSummary(*bytes, release, Latin);
    REQUIRE(summary);
    // AutoCAD's save names whoever saved it, and dates it as the header does.
    const Drawing drawing = Load(ReadFixture(fixture)).value().Drawing;
    CHECK_FALSE(summary->LastSavedBy.empty());
    CHECK(summary->CreateDateTimeDays == drawing.Header.TdcreateDays);
    CHECK(summary->CreateDateTimeMilliseconds == drawing.Header.TdcreateMilliseconds);
    CHECK(summary->Title.empty());
    CHECK(WriteSummary(*summary, release, Latin) == *bytes);
    CHECK(drawing.SummaryInfo == summary);
  }
}

TEST_CASE("file/sections/summary: code page text below R2007 and UTF-16 from it read back")
{
  const SummaryInfo summary = MakeSummary();
  const std::vector<std::byte> older = WriteSummary(summary, Version::R2004, Latin);
  const std::vector<std::byte> newer = WriteSummary(summary, Version::R2018, Latin);
  CHECK(ReadSummary(older, Version::R2004, Latin) == summary);
  CHECK(ReadSummary(newer, Version::R2018, Latin) == summary);
  // The title: a short of its characters and the NUL, then in the code page the escape of what it lacks, or UTF-16.
  CHECK(older[0] == std::byte{13});
  CHECK(older[5] == std::byte{0xE9});
  CHECK(older[7] == std::byte{'\\'});
  CHECK(newer[0] == std::byte{7});
  CHECK(newer[2] == std::byte{'C'});
  CHECK(newer[3] == std::byte{0});
  CHECK(newer.size() > older.size());
}

TEST_CASE("file/sections/summary: bytes that would not write back unchanged give no summary")
{
  const std::vector<std::byte> good = WriteSummary(MakeSummary(), Version::R2004, Latin);
  // Cut short, a byte more, a closing long not zero.
  std::vector<std::byte> cut = good;
  cut.pop_back();
  CHECK_FALSE(ReadSummary(cut, Version::R2004, Latin));
  std::vector<std::byte> longer = good;
  longer.push_back(std::byte{0});
  CHECK_FALSE(ReadSummary(longer, Version::R2004, Latin));
  std::vector<std::byte> closing = good;
  closing.back() = std::byte{1};
  CHECK_FALSE(ReadSummary(closing, Version::R2004, Latin));
  // Properties cut short: the reading stops with the bytes.
  std::vector<std::byte> properties = good;
  properties.resize(properties.size() - 20);
  CHECK_FALSE(ReadSummary(properties, Version::R2004, Latin));
  // A text counted none, or not closed by a NUL, in either form.
  for (const Version release : {Version::R2004, Version::R2018})
  {
    CAPTURE(NameOf(release));
    std::vector<std::byte> bytes = WriteSummary(SummaryInfo{}, release, Latin);
    std::vector<std::byte> none = bytes;
    none[0] = std::byte{0};
    CHECK_FALSE(ReadSummary(none, release, Latin));
    std::vector<std::byte> open = bytes;
    open[2] = std::byte{'x'};
    CHECK_FALSE(ReadSummary(open, release, Latin));
  }
}

TEST_CASE("file/sections/summary: text longer than a short counts and properties past the count are cut")
{
  SummaryInfo summary;
  summary.Title = std::string(70000, 'a');
  for (int at = 0; at < 0x10000; ++at)
  {
    summary.Properties.push_back(Property{.Name = "k", .Value = ""});
  }
  const std::optional<SummaryInfo> read =
      ReadSummary(WriteSummary(summary, Version::R2018, Latin), Version::R2018, Latin);
  REQUIRE(read);
  CHECK(read->Title == std::string(0xFFFE, 'a'));
  CHECK(read->Properties.size() == 0xFFFF);
}
} // namespace DwgIO::File::Sections
