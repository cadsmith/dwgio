/***********************************************************************************************************************
 *  @file      source/releases/mtext.cpp
 *  @brief     What an MTEXT holds that an older release keeps in its extended data.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/mtext.hpp"

#include "releases/palette.hpp"
#include "releases/remake.hpp"

#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
constexpr std::string_view InfoBegin = "ACAD_MTEXT_COLUMN_INFO_BEGIN";
constexpr std::string_view InfoEnd = "ACAD_MTEXT_COLUMN_INFO_END";
constexpr std::string_view ListBegin = "ACAD_MTEXT_COLUMNS_BEGIN";
constexpr std::string_view ListEnd = "ACAD_MTEXT_COLUMNS_END";
constexpr std::string_view SpacingBegin = "MTEXTBEGIN";
constexpr std::string_view SpacingEnd = "MTEXTEND";
constexpr std::string_view BackgroundBegin = "ACAD_MTEXT_BBRT";
constexpr std::string_view BackgroundEnd = "ACAD_MTEXT_BERT";
constexpr std::string_view DepthBegin = "ACAD_MTEXT_DEFINED_HEIGHT_BEGIN";
constexpr std::string_view DepthEnd = "ACAD_MTEXT_DEFINED_HEIGHT_END";

constexpr int Text = 1000;
constexpr int Real = 1040;
constexpr int Short = 1070;
constexpr int Long = 1071;

/**
 *  The DXF codes the sections name each value by.
 */
constexpr std::int64_t Type = 75;
constexpr std::int64_t Auto = 79;
constexpr std::int64_t Count = 76;
constexpr std::int64_t Reversed = 78;
constexpr std::int64_t Width = 48;
constexpr std::int64_t Gutter = 49;
constexpr std::int64_t Heights = 50;
constexpr std::int64_t Spacing = 73;
constexpr std::int64_t Factor = 44;
constexpr std::int64_t Unnamed = 74;
constexpr std::int64_t Flags = 91;
constexpr std::int64_t Margin = 46;
constexpr std::int64_t Tint = 64;
constexpr std::int64_t Name = 65;
constexpr std::int64_t Book = 66;
constexpr std::int64_t Transparency = 442;
constexpr std::int64_t Depth = 46;

constexpr int Dynamic = 2;

using Groups = std::vector<Tag>;

auto IsText(const Tag &group, std::string_view text) -> bool
{
  const auto *value = std::get_if<std::string>(&group.Value);
  return value != nullptr && *value == text;
}

/**
 *  Where the groups from a marker to its end lie, both in, the end past them.
 */
struct Span
{
  std::ptrdiff_t Begin;
  std::ptrdiff_t End;
};

/**
 *  Empty when the groups lack either marker.
 */
auto FindSpan(const Groups &groups, std::string_view begin, std::string_view end) -> std::optional<Span>
{
  const auto first = std::ranges::find_if(groups, [begin](const Tag &group) -> bool { return IsText(group, begin); });
  const auto last = std::find_if(first, groups.end(), [end](const Tag &group) -> bool { return IsText(group, end); });
  if (last == groups.end())
  {
    return std::nullopt;
  }
  return Span{.Begin = first - groups.begin(), .End = last - groups.begin() + 1};
}

/**
 *  The groups between the markers.
 */
auto Inside(const Groups &groups, const Span &span) -> std::span<const Tag>
{
  return std::span<const Tag>{groups}.subspan(static_cast<std::size_t>(span.Begin + 1),
                                              static_cast<std::size_t>(span.End - span.Begin - 2));
}

void Erase(Groups &groups, const Span &span)
{
  groups.erase(groups.begin() + span.Begin, groups.begin() + span.End);
}

auto WholeOf(const Tag &group) -> std::int64_t
{
  const auto *value = std::get_if<std::int64_t>(&group.Value);
  return value == nullptr ? 0 : *value;
}

auto RealOf(const Tag &group) -> double
{
  const auto *value = std::get_if<double>(&group.Value);
  return value == nullptr ? 0 : *value;
}

auto TextOf(const Tag &group) -> std::string
{
  const auto *value = std::get_if<std::string>(&group.Value);
  return value == nullptr ? std::string{} : *value;
}

auto MakeCodeTag(std::int64_t code) -> Tag
{
  return Tag{.Code = Short, .Value = code};
}

auto MakeOpening(std::string_view text) -> Tag
{
  return Tag{.Code = Text, .Value = std::string{text}};
}

/**
 *  The heights of columns after their count, as many as there are.
 */
auto HeightsOf(std::span<const Tag> groups, std::int64_t count) -> std::vector<double>
{
  std::vector<double> heights;
  const auto many = static_cast<std::size_t>(std::max<std::int64_t>(count, 0));
  for (std::size_t at = 0; at < many && at < groups.size(); ++at)
  {
    heights.push_back(RealOf(groups[at]));
  }
  return heights;
}

/**
 *  The columns a section gives.
 */
auto ColumnsOf(std::span<const Tag> groups) -> Entities::Columns
{
  Entities::Columns columns;
  for (std::size_t at = 0; at + 1 < groups.size(); at += 2)
  {
    const Tag &value = groups[at + 1];
    switch (WholeOf(groups[at]))
    {
    case Type:
      columns.Kind = static_cast<int>(WholeOf(value));
      break;
    case Auto:
      columns.Automatic = WholeOf(value) != 0;
      break;
    case Count:
      columns.Count = static_cast<std::int32_t>(WholeOf(value));
      break;
    case Reversed:
      columns.Reversed = WholeOf(value) != 0;
      break;
    case Width:
      columns.Breadth = RealOf(value);
      break;
    case Gutter:
      columns.Gutter = RealOf(value);
      break;
    case Heights:
      columns.Heights = HeightsOf(groups.subspan(at + 2), WholeOf(value));
      at += columns.Heights.size();
      break;
    default:
      break;
    }
  }
  return columns;
}

/**
 *  The values of a section of codes and values in pairs, by their code.
 */
template <typename SetT>
void ReadPairs(std::span<const Tag> groups, SetT set)
{
  for (std::size_t at = 0; at + 1 < groups.size(); at += 2)
  {
    set(WholeOf(groups[at]), groups[at + 1]);
  }
}

/**
 *  What the sections of a record keep for the release the text comes from, read into it and taken away; a list of
 *  further columns kept aside for a release below R2018.
 */
class SectionReader
{
public:
  SectionReader(Entities::MText &text, const TextContext &texting) : m_Text{&text}, m_Texting{&texting} {}

  /**
   *  False when the record held none of the sections.
   */
  auto Read(Groups &groups) -> bool
  {
    const std::size_t size = groups.size();
    const Version from = m_Texting->From;
    const Version to = m_Texting->To;
    if (from < Version::R2018)
    {
      ReadColumns(groups);
    }
    if (from < Version::R2004)
    {
      ReadSpacing(groups);
      ReadBackground(groups);
    }
    if (const std::optional<Span> depth = FindSpan(groups, DepthBegin, DepthEnd))
    {
      if (from < Version::R2007)
      {
        ReadPairs(Inside(groups, *depth), [this](std::int64_t code, const Tag &value) -> void
                  { m_Text->Defined = code == Depth ? RealOf(value) : m_Text->Defined; });
      }
      if (to < Version::R2007)
      {
        Erase(groups, *depth); // Made again below; when upgrading, AutoCAD leaves it.
      }
    }
    return groups.size() != size;
  }

  [[nodiscard]] auto List() const -> const Groups & { return m_List; }

private:
  void ReadColumns(Groups &groups)
  {
    if (const std::optional<Span> info = FindSpan(groups, InfoBegin, InfoEnd))
    {
      Entities::Columns columns = ColumnsOf(Inside(groups, *info));
      Erase(groups, *info);
      for (double &height : columns.Heights)
      {
        // AutoCAD writes 0 for a height it lays out, and keeps minus the text's own height.
        height = height == 0 ? -m_Text->ExtentHeight : height;
      }
      if (columns.Count > 0)
      {
        m_Text->Width = (columns.Breadth * columns.Count) + (columns.Gutter * (columns.Count - 1));
      }
      m_Text->Columns = std::move(columns);
    }
    if (const std::optional<Span> list = FindSpan(groups, ListBegin, ListEnd))
    {
      if (m_Texting->To < Version::R2018)
      {
        m_List.assign(groups.begin() + list->Begin, groups.begin() + list->End);
      }
      Erase(groups, *list);
    }
  }

  void ReadSpacing(Groups &groups)
  {
    if (const std::optional<Span> spacing = FindSpan(groups, SpacingBegin, SpacingEnd))
    {
      ReadPairs(Inside(groups, *spacing),
                [this](std::int64_t code, const Tag &value) -> void
                {
                  m_Text->LineSpacingStyle =
                      code == Spacing ? static_cast<int>(WholeOf(value)) : m_Text->LineSpacingStyle;
                  m_Text->LineSpacingFactor = code == Factor ? RealOf(value) : m_Text->LineSpacingFactor;
                });
      Erase(groups, *spacing);
    }
  }

  void ReadBackground(Groups &groups)
  {
    const std::optional<Span> background = FindSpan(groups, BackgroundBegin, BackgroundEnd);
    if (!background)
    {
      return;
    }
    Entities::Background &backdrop = m_Text->Background;
    ColorNames names;
    ReadPairs(Inside(groups, *background),
              [&backdrop, &names](std::int64_t code, const Tag &value) -> void
              {
                switch (code)
                {
                case Flags:
                  backdrop.Fill = static_cast<int>(WholeOf(value));
                  break;
                case Margin:
                  backdrop.Margin = RealOf(value);
                  break;
                case Tint:
                  backdrop.Color = FromLong(WholeOf(value));
                  break;
                case Name:
                case Book:
                {
                  std::string text = TextOf(value);
                  (code == Name ? names.Name : names.Book) =
                      text.empty() ? std::nullopt : std::optional<std::string>{std::move(text)};
                  break;
                }
                case Transparency:
                  backdrop.Transparency = static_cast<int>(WholeOf(value));
                  break;
                default:
                  break;
                }
              });
    if (names.Name || names.Book)
    {
      backdrop.Color.Names = std::move(names);
    }
    Erase(groups, *background);
  }

  Entities::MText *m_Text;
  const TextContext *m_Texting;
  Groups m_List;
};

/**
 *  The sections the release a text goes to keeps of it, in the order AutoCAD appends them.
 */
auto MakeSections(const Entities::MText &text, const Groups &list, Version to) -> Groups
{
  Groups groups;
  const Entities::Columns &columns = text.Columns;
  if (to < Version::R2018 && columns.Kind != 0 && !text.Annotative)
  {
    groups = {MakeOpening(InfoBegin),
              MakeCodeTag(Type),
              MakeCodeTag(columns.Kind),
              MakeCodeTag(Auto),
              MakeCodeTag(columns.Automatic ? 1 : 0),
              MakeCodeTag(Count),
              MakeCodeTag(columns.Count),
              MakeCodeTag(Reversed),
              MakeCodeTag(columns.Reversed ? 1 : 0),
              MakeCodeTag(Width),
              Tag{.Code = Real, .Value = columns.Breadth},
              MakeCodeTag(Gutter),
              Tag{.Code = Real, .Value = columns.Gutter}};
    if (columns.Kind == Dynamic && !columns.Automatic)
    {
      groups.push_back(MakeCodeTag(Heights));
      groups.push_back(MakeCodeTag(static_cast<std::int64_t>(columns.Heights.size())));
      for (const double height : columns.Heights)
      {
        groups.push_back(Tag{.Code = Real, .Value = std::max(height, 0.0)});
      }
    }
    groups.push_back(MakeOpening(InfoEnd));
    groups.insert(groups.end(), list.begin(), list.end());
  }
  if (to < Version::R2004)
  {
    groups.insert(groups.end(), {MakeOpening(SpacingBegin), MakeCodeTag(Spacing), MakeCodeTag(text.LineSpacingStyle),
                                 MakeCodeTag(Factor), Tag{.Code = Real, .Value = text.LineSpacingFactor},
                                 MakeCodeTag(Unnamed), MakeCodeTag(0), MakeOpening(SpacingEnd)});
  }
  const Entities::Background &backdrop = text.Background;
  if (to < Version::R2004 && backdrop.Fill != 0)
  {
    const ColorNames none;
    const ColorNames &names = backdrop.Color.Names ? **backdrop.Color.Names : none;
    groups.insert(groups.end(),
                  {MakeOpening(BackgroundBegin), MakeCodeTag(Flags),
                   Tag{.Code = Long, .Value = std::int64_t{backdrop.Fill}}, MakeCodeTag(Margin),
                   Tag{.Code = Real, .Value = backdrop.Margin}, MakeCodeTag(Tint),
                   Tag{.Code = Long, .Value = LongOf(backdrop.Color)}, MakeCodeTag(Name),
                   Tag{.Code = Text, .Value = names.Name.value_or("")}, MakeCodeTag(Book),
                   Tag{.Code = Text, .Value = names.Book.value_or("")}, MakeCodeTag(Transparency),
                   Tag{.Code = Long, .Value = std::int64_t{backdrop.Transparency}}, MakeOpening(BackgroundEnd)});
  }
  if (to < Version::R2007 && text.Defined != 0)
  {
    groups.insert(groups.end(), {MakeOpening(DepthBegin), MakeCodeTag(Depth), Tag{.Code = Real, .Value = text.Defined},
                                 MakeOpening(DepthEnd)});
  }
  return groups;
}

/**
 *  The handles of the further columns' MTEXTs a record lists.
 */
auto CollectColumns(const Xdata &xdata) -> std::vector<Handle>
{
  const std::optional<Span> list = FindSpan(xdata.Tags, ListBegin, ListEnd);
  std::vector<Handle> handles;
  for (std::ptrdiff_t at = list ? list->Begin : 0; list && at < list->End; ++at)
  {
    const Tag &group = xdata.Tags[static_cast<std::size_t>(at)];
    if (const auto *handle = std::get_if<Handle>(&group.Value); handle != nullptr)
    {
      handles.push_back(*handle);
    }
  }
  return handles;
}

/**
 *  An MTEXT whose columns AutoCAD split, and the MTEXTs of its further columns.
 */
struct Join
{
  Handle First;
  std::vector<Handle> Rest;
};
} // namespace

auto ConvertMText(Entities::MText &text, const TextContext &texting) -> bool
{
  Entities::MText made = text;
  SectionReader reading{made, texting};
  std::vector<Xdata> &extended = made.Base.Extended;
  for (auto record = extended.begin(); record != extended.end();)
  {
    const bool read = reading.Read(record->Tags);
    record = read && record->Tags.empty() ? extended.erase(record) : record + 1;
  }
  const Groups sections = MakeSections(made, reading.List(), texting.To);
  const Version to = texting.To;
  if (to < Version::R2018)
  {
    made.Width = made.Columns.Kind != 0 && !made.Annotative ? made.Columns.Breadth : made.Width;
    made.Columns = {};
  }
  if (to < Version::R2004)
  {
    made.Background = {};
  }
  if (to < Version::R2000)
  {
    const Entities::MText fresh;
    made.LineSpacingStyle = fresh.LineSpacingStyle;
    made.LineSpacingFactor = fresh.LineSpacingFactor;
  }
  if (to < Version::R2007)
  {
    made.Defined = 0;
  }
  if (!sections.empty())
  {
    if (!texting.Acad)
    {
      return false;
    }
    const auto record = std::ranges::find(extended, texting.Acad, &Xdata::AppId);
    if (record == extended.end())
    {
      extended.push_back(Xdata{.Application = {}, .AppId = texting.Acad, .Tags = sections});
    }
    else
    {
      record->Tags.insert(record->Tags.end(), sections.begin(), sections.end());
    }
  }
  text = std::move(made);
  return true;
}

void JoinColumns(Remake &remade, Version to)
{
  const Drawing &drawing = remade.Now();
  if (drawing.Release >= Version::R2018 || to < Version::R2018)
  {
    return;
  }
  std::vector<Join> joins;
  for (const Object &object : drawing.Objects)
  {
    const auto *text = GetIf<Entities::MText>(&object);
    if (text == nullptr)
    {
      continue;
    }
    for (const Xdata &xdata : text->Base.Extended)
    {
      std::vector<Handle> rest = CollectColumns(xdata);
      if (!rest.empty())
      {
        joins.push_back(Join{.First = text->Base.Self, .Rest = std::move(rest)});
      }
    }
  }
  if (joins.empty())
  {
    return;
  }
  Drawing &made = remade.Change();
  std::vector<Handle> merged;
  for (const Join &join : joins)
  {
    std::string joined;
    for (const Handle further : join.Rest)
    {
      if (const auto *column = GetIf<Entities::MText>(Find(made, further)))
      {
        joined += column->Value;
        merged.push_back(further);
      }
    }
    auto &first = Get<Entities::MText>(*Find(made, join.First));
    first.Value += joined;
    for (Xdata &xdata : first.Base.Extended)
    {
      if (const std::optional<Span> list = FindSpan(xdata.Tags, ListBegin, ListEnd))
      {
        Erase(xdata.Tags, *list);
      }
    }
  }
  for (const Handle further : merged)
  {
    // An MTEXT in use elsewhere stays; the text is joined all the same.
    const std::expected<void, Failure> removed = Edit::Remove(made, further);
    static_cast<void>(removed);
  }
}

} // namespace DwgIO::Releases
