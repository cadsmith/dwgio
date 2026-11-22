/***********************************************************************************************************************
 *  @file      source/releases/summary.cpp
 *  @brief     The drawing's summary as R14 and R2000 keep it, in the DWGPROPS record.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/summary.hpp"

#include "ascii.hpp"

#include "dwgio/base.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/view.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The record's name in the named objects dictionary, and the text it opens with.
 */
constexpr std::string_view Name = "DWGPROPS";
constexpr std::string_view Cookie = "DWGPROPS COOKIE";

/**
 *  The group codes of the record.
 */
constexpr int Opening = 1;
constexpr int Hyperlink = 1;
constexpr int Editing = 40;
constexpr int Created = 41;
constexpr int Modified = 42;
constexpr int Counted = 90;
constexpr int FirstProperty = 300;
constexpr int LastProperty = 309;

/**
 *  The properties the record keeps as 300 to 309, the rest after the count.
 */
constexpr std::size_t Kept = 10;

/**
 *  The texts of the summary the record keeps as 2 to 9, by their codes.
 */
struct Coded
{
  int Code;
  std::string SummaryInfo::*Member;
};

constexpr std::array Texts{
    Coded{.Code = 2, .Member = &SummaryInfo::Title},         Coded{.Code = 3, .Member = &SummaryInfo::Subject},
    Coded{.Code = 4, .Member = &SummaryInfo::Author},        Coded{.Code = 6, .Member = &SummaryInfo::Comments},
    Coded{.Code = 7, .Member = &SummaryInfo::Keywords},      Coded{.Code = 8, .Member = &SummaryInfo::LastSavedBy},
    Coded{.Code = 9, .Member = &SummaryInfo::RevisionNumber}};

/**
 *  A date or a time of the summary, its days and milliseconds, and the step the record keeps them to: AutoCAD writes
 *  the editing time to the millisecond and the dates to the second.
 */
struct Dated
{
  int Code;
  std::int32_t SummaryInfo::*Days;
  std::int32_t SummaryInfo::*Milliseconds;
  std::int32_t Step;
};

constexpr std::int32_t Millisecond = 1;
constexpr std::int32_t Second = 1000;

constexpr std::array Dates{Dated{.Code = Editing,
                                 .Days = &SummaryInfo::TotalEditingTimeDays,
                                 .Milliseconds = &SummaryInfo::TotalEditingTimeMilliseconds,
                                 .Step = Millisecond},
                           Dated{.Code = Created,
                                 .Days = &SummaryInfo::CreateDateTimeDays,
                                 .Milliseconds = &SummaryInfo::CreateDateTimeMilliseconds,
                                 .Step = Second},
                           Dated{.Code = Modified,
                                 .Days = &SummaryInfo::ModifiedDateTimeDays,
                                 .Milliseconds = &SummaryInfo::ModifiedDateTimeMilliseconds,
                                 .Step = Second}};

constexpr double Day = 86400000.0;

/**
 *  The days and milliseconds of a Julian date, the milliseconds rounded.
 *
 *  A date past the days a long counts, or no number, as only a broken record holds, is kept within them, no number as
 *  day 0.
 */
void SplitDate(double date, std::int32_t &days, std::int32_t &milliseconds)
{
  constexpr double First = std::numeric_limits<std::int32_t>::min();
  constexpr double Last = std::numeric_limits<std::int32_t>::max() - 1;
  const double kept = std::isnan(date) ? 0.0 : std::clamp(date, First, Last);
  const double whole = std::floor(kept);
  const auto rounded = static_cast<std::int64_t>(std::llround((kept - whole) * Day));
  const std::int64_t carried = rounded >= static_cast<std::int64_t>(Day) ? 1 : 0;
  days = static_cast<std::int32_t>(static_cast<std::int64_t>(whole) + carried);
  milliseconds = static_cast<std::int32_t>(rounded - (carried * static_cast<std::int64_t>(Day)));
}

/**
 *  A property of a 300 to 309 text: the name before the first equals sign, the value after; empty for one of no name,
 *  as AutoCAD leaves it out.
 */
auto ParseProperty(std::string_view text) -> std::optional<Property>
{
  const std::size_t equals = text.find('=');
  if (equals == 0 || equals == std::string_view::npos)
  {
    return std::nullopt;
  }
  return Property{.Name = std::string{text.substr(0, equals)}, .Value = std::string{text.substr(equals + 1)}};
}

/**
 *  The summary read from one group of the record.
 */
void Read(SummaryInfo &summary, const Tag &group)
{
  if (const auto *text = std::get_if<std::string>(&group.Value))
  {
    const auto *const coded = std::ranges::find(Texts, group.Code, &Coded::Code);
    if (coded != Texts.end())
    {
      summary.*coded->Member = *text;
    }
    else if (group.Code == Hyperlink)
    {
      summary.HyperlinkBase = *text;
    }
    else if (group.Code >= FirstProperty && group.Code <= LastProperty)
    {
      if (std::optional<Property> property = ParseProperty(*text))
      {
        summary.Properties.push_back(std::move(*property));
      }
    }
  }
  else if (const auto *real = std::get_if<double>(&group.Value))
  {
    const auto *const dated = std::ranges::find(Dates, group.Code, &Dated::Code);
    if (dated != Dates.end())
    {
      SplitDate(*real, summary.*dated->Days, summary.*dated->Milliseconds);
    }
  }
}

/**
 *  The named objects dictionary's entry of the record, or none.
 */
auto FindEntry(const Objects::Dictionary &root) -> std::vector<Objects::Entry>::const_iterator
{
  return std::ranges::find(root.Entries, Name, &Objects::Entry::Name);
}

auto RootOf(Drawing &drawing) -> Objects::Dictionary *
{
  return GetIf<Objects::Dictionary>(Find(drawing, drawing.Header.DictionaryNamedObjects));
}

/**
 *  Whether one entry name comes before another as AutoCAD orders the named objects: by their ASCII lower case, a byte
 *  past ASCII after every letter.
 */
auto IsBefore(std::string_view one, std::string_view other) -> bool
{
  constexpr auto Folded = [](char letter) -> unsigned char
  {
    return static_cast<unsigned char>(Ascii::Lower(letter));
  };
  return std::ranges::lexicographical_compare(one, other, {}, Folded, Folded);
}
} // namespace

auto SummaryOf(const Objects::XRecord &record) -> std::optional<SummaryInfo>
{
  if (record.Data.empty() || record.Data.front() != Tag{.Code = Opening, .Value = std::string{Cookie}})
  {
    return std::nullopt;
  }
  SummaryInfo summary;
  for (const Tag &group : record.Data | std::views::drop(1))
  {
    Read(summary, group);
  }
  return summary;
}

auto MakePropertyGroups(const SummaryInfo &summary) -> std::vector<Tag>
{
  std::vector<Tag> groups{{.Code = Opening, .Value = std::string{Cookie}}};
  for (const Coded &coded : Texts)
  {
    groups.push_back({.Code = coded.Code, .Value = summary.*coded.Member});
  }
  const auto text = [&summary](std::size_t at) -> std::string
  {
    return summary.Properties[at].Name + "=" + summary.Properties[at].Value;
  };
  const std::size_t count = summary.Properties.size();
  for (std::size_t at = 0; at < Kept; ++at)
  {
    groups.push_back({.Code = FirstProperty + static_cast<int>(at), .Value = at < count ? text(at) : "="});
  }
  for (const Dated &dated : Dates)
  {
    const std::int32_t milliseconds = summary.*dated.Milliseconds - (summary.*dated.Milliseconds % dated.Step);
    groups.push_back({.Code = dated.Code, .Value = summary.*dated.Days + (milliseconds / Day)});
  }
  groups.push_back({.Code = Hyperlink, .Value = summary.HyperlinkBase});
  groups.push_back({.Code = Counted, .Value = static_cast<std::int64_t>(count)});
  // The properties past the tenth after the count, each a 300.
  for (std::size_t at = Kept; at < count; ++at)
  {
    groups.push_back({.Code = FirstProperty, .Value = text(at)});
  }
  return groups;
}

auto FindProperties(const Drawing &drawing) -> const Objects::XRecord *
{
  const Objects::Dictionary *root = View::Root(drawing);
  if (root == nullptr)
  {
    return nullptr;
  }
  return GetIf<Objects::XRecord>(View::Entry(drawing, *root, Name));
}

void MoveSummary(Remake &remade, Version to)
{
  const Drawing &drawing = remade.Now();
  const Objects::XRecord *record = FindProperties(drawing);
  if (to >= Version::R2004)
  {
    // AutoCAD reads the record into the summary on opening, and keeps none.
    if (drawing.Release >= Version::R2004 || record == nullptr || !SummaryOf(*record))
    {
      return;
    }
    const Handle self = record->Base.Self;
    Drawing &made = remade.Change();
    Objects::Dictionary &root = *RootOf(made);
    root.Entries.erase(FindEntry(root));
    std::erase_if(made.Objects, [self](const Object &object) -> bool { return BaseOf(object).Self == self; });
    return;
  }
  if (!drawing.SummaryInfo || View::Root(drawing) == nullptr ||
      (record != nullptr && SummaryOf(*record) == drawing.SummaryInfo))
  {
    return;
  }
  const Objects::Dictionary &held = *View::Root(drawing);
  const auto entered = FindEntry(held);
  if (entered != held.Entries.end() && GetIf<Objects::XRecord>(Find(drawing, entered->Object)) == nullptr)
  {
    return; // Another kind under its name: left as it is.
  }
  Drawing &made = remade.Change();
  Objects::Dictionary &root = *RootOf(made);
  const auto entry = FindEntry(root);
  if (entry != root.Entries.end())
  {
    Get<Objects::XRecord>(*Find(made, entry->Object)).Data = MakePropertyGroups(*drawing.SummaryInfo);
    return;
  }
  const Handle owner = made.Header.DictionaryNamedObjects;
  const Handle self = Edit::MakeHandle(made);
  const auto after =
      std::ranges::find_if(root.Entries, [](const Objects::Entry &one) -> bool { return IsBefore(Name, one.Name); });
  root.Entries.insert(after, Objects::Entry{.Name = std::string{Name}, .Object = self});
  made.Objects.emplace_back(Objects::XRecord{.Base = {.Self = self, .Owner = owner, .Reactors = {owner}},
                                             .Cloning = 1,
                                             .Data = MakePropertyGroups(*drawing.SummaryInfo),
                                             .Objects = {},
                                             .Rest = {}});
}

} // namespace DwgIO::Releases
