/***********************************************************************************************************************
 *  @file      source/file/sections/summary.cpp
 *  @brief     The summary info section, chapter 13.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/sections/summary.hpp"

#include "file/sections/text.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <string>
#include <utility>

namespace DwgIO::File::Sections
{
namespace
{
/**
 *  The summary's texts, in the section's order.
 */
constexpr std::array Texts{&SummaryInfo::Title,          &SummaryInfo::Subject,      &SummaryInfo::Author,
                           &SummaryInfo::Keywords,       &SummaryInfo::Comments,     &SummaryInfo::LastSavedBy,
                           &SummaryInfo::RevisionNumber, &SummaryInfo::HyperlinkBase};

/**
 *  The editing time and the dates, each its days and milliseconds, in the section's order.
 */
constexpr std::array Times{&SummaryInfo::TotalEditingTimeDays, &SummaryInfo::TotalEditingTimeMilliseconds,
                           &SummaryInfo::CreateDateTimeDays,   &SummaryInfo::CreateDateTimeMilliseconds,
                           &SummaryInfo::ModifiedDateTimeDays, &SummaryInfo::ModifiedDateTimeMilliseconds};

/**
 *  The two longs after the properties, which AutoCAD writes zero.
 */
constexpr int Closing = 2;

/**
 *  The most properties a short counts.
 */
constexpr std::size_t MostProperties = 0xFFFF;
} // namespace

auto ReadSummary(std::span<const std::byte> bytes, Version release, const Encoding::Scheme &text)
    -> std::optional<SummaryInfo>
{
  TextReader in{bytes, release, text};
  SummaryInfo summary;
  for (const auto member : Texts)
  {
    summary.*member = in.Text(TextCount::Ended);
  }
  for (const auto member : Times)
  {
    summary.*member = static_cast<std::int32_t>(in.Long());
  }
  const std::uint16_t count = in.Short();
  for (std::uint16_t at = 0; at < count && !in.HasFailed(); ++at)
  {
    Property property;
    property.Name = in.Text(TextCount::Ended);
    property.Value = in.Text(TextCount::Ended);
    summary.Properties.push_back(std::move(property));
  }
  // Typed only when it writes back as it was: the closing longs zero, nothing after them, each text as written.
  if (in.HasFailed() || !std::ranges::equal(WriteSummary(summary, release, text), bytes))
  {
    return std::nullopt;
  }
  return summary;
}

auto WriteSummary(const SummaryInfo &summary, Version release, const Encoding::Scheme &text) -> std::vector<std::byte>
{
  TextWriter out{release, text};
  for (const auto member : Texts)
  {
    out.Text(summary.*member, TextCount::Ended);
  }
  for (const auto member : Times)
  {
    out.Long(static_cast<std::uint32_t>(summary.*member));
  }
  // As many properties as the count holds.
  const std::size_t count = std::min(summary.Properties.size(), MostProperties);
  out.Short(static_cast<std::uint16_t>(count));
  for (const Property &property : summary.Properties | std::views::take(count))
  {
    out.Text(property.Name, TextCount::Ended);
    out.Text(property.Value, TextCount::Ended);
  }
  for (int at = 0; at < Closing; ++at)
  {
    out.Long(0);
  }
  return out.Take();
}
} // namespace DwgIO::File::Sections
