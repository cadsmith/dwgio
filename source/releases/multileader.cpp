/***********************************************************************************************************************
 *  @file      source/releases/multileader.cpp
 *  @brief     What a MULTILEADER holds of R2010, kept in a record below it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/multileader.hpp"

#include "geometry/axes.hpp"
#include "releases/palette.hpp"

#include "dwgio/color.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/vector.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
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
constexpr std::string_view Line = "Leaderline";
constexpr std::string_view Context = "ROUNDTRIP_MLCONTEXT";
constexpr std::string_view Leader = "ACAD_ROUNDTRIP_2010_MLEADER";

constexpr int Marker = 102;
constexpr int Index = 91;
constexpr int Kind = 170;
constexpr int Tint = 92;
constexpr int Linetype = 340;
constexpr int Lineweight = 171;
constexpr int Size = 40;
constexpr int Head = 341;
constexpr int Overrides = 93;

/**
 *  The override bits of a leader line, in the order the record keeps their values.
 */
constexpr int OwnKind = 1;
constexpr int OwnColor = 2;
constexpr int OwnLinetype = 4;
constexpr int OwnLineweight = 8;
constexpr int OwnSize = 16;
constexpr int OwnHead = 32;

/**
 *  The groups of the context's and the leader's own R2010 values in the record.
 *
 *  The context keeps its top attachment in 273 and its bottom one in 274, the leader its bottom one in 273 and its top
 *  one in 274; each only when it is not 9.
 */
constexpr int Direction = 272;
constexpr int ContextLeft = 174;
constexpr int ContextRight = 175;
constexpr int LeaderLeft = 173;
constexpr int LeaderRight = 174;
constexpr int Upper = 273;
constexpr int Lower = 274;
constexpr int Branched = 91;
constexpr int Landed = 40;
constexpr int Dogleg = 41;

/**
 *  The attachments of text above or below its leaders: at its center, or over its line and at its center.
 */
constexpr int Centered = 9;
constexpr int Overlined = 10;

/**
 *  The attachments of text beside its leaders AutoCAD's save before R2010 gives a vertically attached text: on the left
 *  the bottom line's bottom, underlined when the bottom attachment is overlined; on the right the top line's top,
 *  underlined when the top attachment is and every leader comes from above.
 */
constexpr int Bottom = 4;
constexpr int BottomUnderlined = 5;
constexpr int Top = 0;
constexpr int TopUnderlined = 6;

/**
 *  The override bits AutoCAD's save before R2010 sets on a vertically attached text: the dogleg's length, the left and
 *  the right attachments.
 */
constexpr unsigned Lowered = 0x80U | 0x1000U | 0x2000000U;

/**
 *  The columns of an MTEXT's attachment, as group 71 gives it from 1 to 9 row by row: left, center and right.
 */
constexpr std::size_t Columns = 3;

/**
 *  How far AutoCAD's save before R2010 moves the content's base point of a text no leader comes to from below, up the
 *  text, in text heights; its save to R2010 moves it back as far.
 */
constexpr double Lift = 1.2;

auto Shift(Vector point, Vector along, double by) -> Vector
{
  return Vector{.X = point.X + (along.X * by), .Y = point.Y + (along.Y * by), .Z = point.Z + (along.Z * by)};
}

/**
 *  How far along the text its location lies from the content's base point.
 */
auto OffsetOf(const Entities::MultiLeaders::ContextData &note) -> double
{
  const Vector apart{.X = note.TextLocation.X - note.ContentBasePoint.X,
                     .Y = note.TextLocation.Y - note.ContentBasePoint.Y,
                     .Z = note.TextLocation.Z - note.ContentBasePoint.Z};
  return Geometry::Dot(apart, Geometry::Unit(note.TextDirection));
}

auto ColumnOf(const Entities::MultiLeaders::ContextData &note) -> std::size_t
{
  return static_cast<std::size_t>(std::max(note.TextAttachment - 1, 0)) % Columns;
}

/**
 *  Whether the leaders of a vertically attached text all come to its top: each one's dogleg points against the text's
 *  direction.
 */
auto IsLifted(const Entities::MultiLeaders::ContextData &note) -> bool
{
  return !note.Leaders.empty() &&
         std::ranges::none_of(note.Leaders, [&note](const Entities::MultiLeaders::LeaderRoot &branch) -> bool
                              { return Geometry::Dot(branch.DoglegVector, note.TextDirection) > 0; });
}

/**
 *  The content's base point moved by the landing gap and half the text's width along the text, and by Lift text heights
 *  up it when the text is lifted, in the sense `sign` gives: -1 to where AutoCAD's save before R2010 puts it, 1 back.
 */
void MoveBase(Entities::MultiLeaders::ContextData &note, double half, double sign)
{
  const Vector along = Geometry::Unit(note.TextDirection);
  const Vector upward = Geometry::Unit(Geometry::Cross(note.TextNormal, along));
  const double lift = IsLifted(note) ? Lift * note.TextHeight : 0;
  note.ContentBasePoint =
      Shift(Shift(note.ContentBasePoint, along, sign * (note.LandingGap + half)), upward, -sign * lift);
}

/**
 *  A vertically attached text's base point as AutoCAD's save before R2010 puts it. R2010 keeps it at the middle of the
 *  text's width, the release before at the text's left edge less the landing gap, so the save moves it back by the gap
 *  and half the width the font gives, and up by Lift heights when the text is lifted. A text aligned left or right has
 *  its location at that edge, half its width from the base point; a centered text has it at the middle, which leaves
 *  the width unknown, taken as 0.
 */
void LowerBase(Entities::MultiLeaders::ContextData &note)
{
  const double offset = OffsetOf(note);
  const std::array<double, Columns> halves{-offset, 0, offset};
  MoveBase(note, halves.at(ColumnOf(note)), -1);
}

/**
 *  A vertically attached text's base point as AutoCAD's save to R2010 puts it back, LowerBase reversed. The text's left
 *  edge lies the landing gap past the base point; a centered text has its location half its width past that edge, a
 *  text aligned right its whole width, and one aligned left at the edge, which leaves the width unknown, taken as 0.
 */
void RaiseBase(Entities::MultiLeaders::ContextData &note)
{
  const double edge = OffsetOf(note) - note.LandingGap;
  const std::array<double, Columns> halves{0, edge, edge / 2};
  MoveBase(note, halves.at(ColumnOf(note)), 1);
}

auto IsLine(const Tag &group) -> bool
{
  const auto *text = std::get_if<std::string>(&group.Value);
  return group.Code == Marker && text != nullptr && text->starts_with(Line);
}

/**
 *  A whole number of a group, or zero.
 */
auto WholeOf(const Tag &group) -> std::int64_t
{
  const auto *value = std::get_if<std::int64_t>(&group.Value);
  return value == nullptr ? 0 : *value;
}

/**
 *  A line's value from a group of the record.
 */
void SetLineValue(Entities::MultiLeaders::LeaderLine &stem, const Tag &group)
{
  switch (group.Code)
  {
  case Kind:
    stem.LeaderType = static_cast<int>(WholeOf(group));
    break;
  case Tint:
    stem.Color = FromLong(WholeOf(group));
    break;
  case Linetype:
  case Head:
  {
    const auto *handle = std::get_if<Handle>(&group.Value);
    (group.Code == Head ? stem.Arrowhead : stem.Linetype) = handle == nullptr ? Handle{} : *handle;
    break;
  }
  case Lineweight:
    stem.Lineweight = static_cast<int>(WholeOf(group));
    break;
  case Size:
  {
    const auto *real = std::get_if<double>(&group.Value);
    stem.ArrowheadSize = real == nullptr ? 0 : *real;
    break;
  }
  case Overrides:
    stem.Overrides = static_cast<int>(WholeOf(group));
    break;
  default:
    break;
  }
}

auto IsMarker(const Tag &group, std::string_view name) -> bool
{
  const auto *text = std::get_if<std::string>(&group.Value);
  return group.Code == Marker && text != nullptr && *text == name;
}

auto RealOf(const Tag &group) -> double
{
  const auto *value = std::get_if<double>(&group.Value);
  return value == nullptr ? 0 : *value;
}

/**
 *  An attachment above or below the text, in the record only when it is not at the center.
 */
void AddAttachment(std::vector<Tag> &groups, int code, int attachment)
{
  if (attachment != Centered)
  {
    groups.push_back(Tag{.Code = code, .Value = std::int64_t{attachment}});
  }
}

/**
 *  The context's R2010 values in the record, and the context as AutoCAD's save before R2010 leaves it, when its text
 *  attaches vertically: the text's base point moved, the attachments beside the text derived from those above and
 *  below, the leaders' doglegs of no length.
 */
auto ContextOf(Entities::MultiLeaders::MLeader &callout) -> std::vector<Tag>
{
  Entities::MultiLeaders::ContextData &note = callout.Context;
  std::vector<Tag> groups;
  if (callout.TextAttachmentDirection == 1)
  {
    groups = {Tag{.Code = Marker, .Value = std::string{Context}},
              Tag{.Code = Direction, .Value = std::int64_t{callout.TextAttachmentDirection}},
              Tag{.Code = ContextLeft, .Value = std::int64_t{note.TextLeftAttachment}},
              Tag{.Code = ContextRight, .Value = std::int64_t{note.TextRightAttachment}}};
    AddAttachment(groups, Upper, note.TextTopAttachment);
    AddAttachment(groups, Lower, note.TextBottomAttachment);
    if (note.HasMText)
    {
      LowerBase(note);
    }
    note.TextLeftAttachment = note.TextBottomAttachment == Overlined ? BottomUnderlined : Bottom;
    note.TextRightAttachment = IsLifted(note) && note.TextTopAttachment == Overlined ? TopUnderlined : Top;
    for (Entities::MultiLeaders::LeaderRoot &branch : note.Leaders)
    {
      branch.AttachmentDirection = 0;
      branch.DoglegLength = 0;
      groups.push_back(Tag{.Code = Branched, .Value = std::int64_t{branch.Index}});
      groups.push_back(Tag{.Code = Landed, .Value = branch.DoglegLength});
    }
  }
  note.TextTopAttachment = Centered;
  note.TextBottomAttachment = Centered;
  return groups;
}

/**
 *  The leader's own R2010 values in the record, and the leader as AutoCAD's save before R2010 leaves it: when its text
 *  attaches vertically, the attachments beside it derived as the context's, the dogleg of no length and those three
 *  marked overridden; else only the attachments above and below kept, where they are not at the center.
 */
auto OwnOf(Entities::MultiLeaders::MLeader &callout) -> std::vector<Tag>
{
  std::vector<Tag> groups;
  if (callout.TextAttachmentDirection == 1)
  {
    groups = {Tag{.Code = Marker, .Value = std::string{Leader}},
              Tag{.Code = Direction, .Value = std::int64_t{callout.TextAttachmentDirection}},
              Tag{.Code = LeaderLeft, .Value = std::int64_t{callout.TextLeftAttachment}},
              Tag{.Code = LeaderRight, .Value = std::int64_t{callout.TextRightAttachment}},
              Tag{.Code = Dogleg, .Value = callout.DoglegLength}};
    callout.TextLeftAttachment = callout.TextBottomAttachment == Overlined ? BottomUnderlined : Bottom;
    callout.TextRightAttachment = Top;
    callout.DoglegLength = 0;
    callout.Overrides = static_cast<int>(static_cast<unsigned>(callout.Overrides) | Lowered);
  }
  else if (callout.TextTopAttachment != Centered || callout.TextBottomAttachment != Centered)
  {
    groups = {Tag{.Code = Marker, .Value = std::string{Leader}}};
  }
  AddAttachment(groups, Upper, callout.TextBottomAttachment);
  AddAttachment(groups, Lower, callout.TextTopAttachment);
  callout.TextAttachmentDirection = 0;
  callout.TextTopAttachment = Centered;
  callout.TextBottomAttachment = Centered;
  return groups;
}

/**
 *  The context given back its R2010 values from the record, a vertically attached text's base point moved back.
 */
void RestoreContext(Entities::MultiLeaders::ContextData &note, std::span<const Tag> part)
{
  for (const Tag &group : part)
  {
    switch (group.Code)
    {
    case Direction:
      for (Entities::MultiLeaders::LeaderRoot &branch : note.Leaders)
      {
        branch.AttachmentDirection = static_cast<int>(WholeOf(group));
      }
      if (note.HasMText) // The record holds the context only for a vertically attached text.
      {
        RaiseBase(note);
      }
      break;
    case ContextLeft:
      note.TextLeftAttachment = static_cast<int>(WholeOf(group));
      break;
    case ContextRight:
      note.TextRightAttachment = static_cast<int>(WholeOf(group));
      break;
    case Upper:
      note.TextTopAttachment = static_cast<int>(WholeOf(group));
      break;
    case Lower:
      note.TextBottomAttachment = static_cast<int>(WholeOf(group));
      break;
    default:
      break;
    }
  }
  // Each leader's dogleg by its index: a 91, then its length's 40.
  for (std::size_t at = 0; at + 1 < part.size(); ++at)
  {
    if (part[at].Code == Branched && part[at + 1].Code == Landed)
    {
      const auto branch = std::ranges::find(note.Leaders, WholeOf(part[at]),
                                            [](const Entities::MultiLeaders::LeaderRoot &one) -> std::int64_t
                                            { return std::int64_t{one.Index}; });
      if (branch != note.Leaders.end())
      {
        branch->DoglegLength = RealOf(part[at + 1]);
      }
    }
  }
}

/**
 *  The leader given back its own R2010 values from the record.
 */
void RestoreLeader(Entities::MultiLeaders::MLeader &callout, std::span<const Tag> part)
{
  for (const Tag &group : part)
  {
    switch (group.Code)
    {
    case Direction:
      callout.TextAttachmentDirection = static_cast<int>(WholeOf(group));
      break;
    case LeaderLeft:
      callout.TextLeftAttachment = static_cast<int>(WholeOf(group));
      break;
    case LeaderRight:
      callout.TextRightAttachment = static_cast<int>(WholeOf(group));
      break;
    case Dogleg:
      callout.DoglegLength = RealOf(group);
      break;
    case Upper:
      callout.TextBottomAttachment = static_cast<int>(WholeOf(group));
      break;
    case Lower:
      callout.TextTopAttachment = static_cast<int>(WholeOf(group));
      break;
    default:
      break;
    }
  }
}

/**
 *  The leader's line of that index, or null.
 */
auto LineOf(Entities::MultiLeader &leader, std::int64_t index) -> Entities::MultiLeaders::LeaderLine *
{
  for (Entities::MultiLeaders::LeaderRoot &branch : leader.MLeader.Context.Leaders)
  {
    for (Entities::MultiLeaders::LeaderLine &stem : branch.Lines)
    {
      if (stem.Index == index)
      {
        return &stem;
      }
    }
  }
  return nullptr;
}
} // namespace

auto MakeLinesRecord(Entities::MultiLeader &leader) -> std::vector<Tag>
{
  std::vector<Tag> groups = ContextOf(leader.MLeader);
  for (Entities::MultiLeaders::LeaderRoot &branch : leader.MLeader.Context.Leaders)
  {
    for (Entities::MultiLeaders::LeaderLine &stem : branch.Lines)
    {
      const int own = stem.Overrides;
      if (own != 0)
      {
        groups.push_back(Tag{.Code = Marker, .Value = std::format("{}{}", Line, stem.Index)});
        groups.push_back(Tag{.Code = Index, .Value = std::int64_t{stem.Index}});
        const std::vector<std::pair<int, Tag>> values{
            {OwnKind, Tag{.Code = Kind, .Value = std::int64_t{stem.LeaderType}}},
            {OwnColor, Tag{.Code = Tint, .Value = LongOf(stem.Color)}},
            {OwnLinetype, Tag{.Code = Linetype, .Value = stem.Linetype}},
            {OwnLineweight, Tag{.Code = Lineweight, .Value = std::int64_t{stem.Lineweight}}},
            {OwnSize, Tag{.Code = Size, .Value = stem.ArrowheadSize}},
            {OwnHead, Tag{.Code = Head, .Value = stem.Arrowhead}}};
        for (const auto &[bit, group] : values)
        {
          if ((static_cast<unsigned>(own) & static_cast<unsigned>(bit)) != 0)
          {
            groups.push_back(group);
          }
        }
        groups.push_back(Tag{.Code = Overrides, .Value = std::int64_t{own}});
      }
      stem = Entities::MultiLeaders::LeaderLine{
          .Vertices = std::move(stem.Vertices), .Breaks = std::move(stem.Breaks), .Index = stem.Index};
    }
  }
  std::vector<Tag> own = OwnOf(leader.MLeader);
  groups.insert(groups.end(), own.begin(), own.end());
  return groups;
}

void RestoreLines(Entities::MultiLeader &leader, std::vector<Tag> &record)
{
  std::vector<Tag> rest;
  for (std::size_t at = 0; at < record.size();)
  {
    std::size_t end = at + 1;
    while (end < record.size() && record[end].Code != Marker)
    {
      ++end;
    }
    const std::span<const Tag> part{record.begin() + static_cast<std::ptrdiff_t>(at), end - at};
    if (IsLine(record[at]))
    {
      const bool indexed = part.size() > 1 && part[1].Code == Index;
      Entities::MultiLeaders::LeaderLine *stem = indexed ? LineOf(leader, WholeOf(part[1])) : nullptr;
      for (std::size_t value = 1; stem != nullptr && value < part.size(); ++value)
      {
        SetLineValue(*stem, part[value]);
      }
    }
    else if (IsMarker(record[at], Context))
    {
      RestoreContext(leader.MLeader.Context, part);
    }
    else if (IsMarker(record[at], Leader))
    {
      RestoreLeader(leader.MLeader, part);
    }
    else
    {
      rest.insert(rest.end(), std::make_move_iterator(record.begin() + static_cast<std::ptrdiff_t>(at)),
                  std::make_move_iterator(record.begin() + static_cast<std::ptrdiff_t>(end)));
    }
    at = end;
  }
  record = std::move(rest);
}
} // namespace DwgIO::Releases
