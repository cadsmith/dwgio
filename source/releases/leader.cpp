/***********************************************************************************************************************
 *  @file      source/releases/leader.cpp
 *  @brief     A LEADER's R14 fields, kept by R2000 and derived again at a save to R14.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/leader.hpp"

#include "ascii.hpp"

#include "dwgio/base.hpp"
#include "dwgio/entities/leader.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
constexpr int Text = 1000;
constexpr int Brace = 1002;
constexpr int Short = 1070;
constexpr const char *Overrides = "DSTYLE";

/**
 *  The DXF codes of the dimension variables a LEADER's R14 fields derive from, as DSTYLE names them.
 */
constexpr std::int64_t Dimblk = 5;
constexpr std::int64_t Dimscale = 40;
constexpr std::int64_t Dimasz = 41;
constexpr std::int64_t Dimgap = 147;
constexpr std::int64_t Dimclrd = 176;
constexpr std::int64_t Dimldrblk = 341;

/**
 *  What R14's leader arrow kind says of an arrow drawn by a block, as AutoCAD writes it.
 */
constexpr int Blocked = 8;
/**
 *  The index a ByBlock leader of a ByBlock style draws in.
 */
constexpr int White = 7;

/**
 *  The dimension variables a leader draws by.
 */
struct Settings
{
  double Scale;
  double Size;
  double Gap;
  int Color;
  Handle Arrow;
};

auto IsText(const Tag &group, const char *text) -> bool
{
  const auto *value = std::get_if<std::string>(&group.Value);
  return value != nullptr && *value == text;
}

/**
 *  Where the DSTYLE overrides of extended data open: the group after its brace; the end when it has none.
 */
auto FindOverrides(const std::vector<Tag> &groups) -> std::vector<Tag>::const_iterator
{
  const auto named = std::ranges::find_if(groups, [](const Tag &group) -> bool { return IsText(group, Overrides); });
  if (named == groups.end() || std::next(named) == groups.end() || std::next(named)->Code != Brace)
  {
    return groups.end();
  }
  return std::next(named, 2);
}

/**
 *  The settings with one DSTYLE override, by its variable's DXF code, over them.
 */
void Override(Settings &settings, std::int64_t code, const Content &value)
{
  if (const auto *real = std::get_if<double>(&value))
  {
    settings.Scale = code == Dimscale ? *real : settings.Scale;
    settings.Size = code == Dimasz ? *real : settings.Size;
    settings.Gap = code == Dimgap ? *real : settings.Gap;
  }
  if (const auto *whole = std::get_if<std::int64_t>(&value); whole != nullptr && code == Dimclrd)
  {
    settings.Color = static_cast<int>(*whole);
  }
  if (const auto *handle = std::get_if<Handle>(&value); handle != nullptr && code == Dimldrblk)
  {
    settings.Arrow = *handle;
  }
}

/**
 *  The variables of the style, or of the header for a leader naming no style, with the record's overrides over them.
 */
auto SettingsOf(const Entities::Leader &leader, const Lookup &found, const Xdata *record) -> Settings
{
  const Header &header = found.Drawing().Header;
  const Object *object = found.Of(leader.Style);
  const auto *style = object == nullptr ? nullptr : GetIf<Tables::DimStyle>(object);
  Settings settings = style == nullptr ? Settings{.Scale = header.Dimscale,
                                                  .Size = header.Dimasz,
                                                  .Gap = header.Dimgap,
                                                  .Color = header.Dimclrd.Index,
                                                  .Arrow = header.Dimldrblk}
                                       : Settings{.Scale = style->Dimscale,
                                                  .Size = style->Dimasz,
                                                  .Gap = style->Dimgap,
                                                  .Color = style->Dimclrd.Index,
                                                  .Arrow = style->Dimldrblk};
  if (record == nullptr)
  {
    return settings;
  }
  const std::vector<Tag> &groups = record->Tags;
  for (auto at = FindOverrides(groups); at != groups.end() && std::next(at) != groups.end(); std::advance(at, 2))
  {
    const auto *code = at->Code == Short ? std::get_if<std::int64_t>(&at->Value) : nullptr;
    const Content &value = std::next(at)->Value;
    if (code == nullptr)
    {
      break; // The closing brace, or groups of no override.
    }
    Override(settings, *code, value);
  }
  return settings;
}

/**
 *  The name R14 gives an arrow block: its record's, upper case, without the underscore of AutoCAD's own arrows.
 */
auto ArrowName(const Lookup &found, Handle block) -> std::string
{
  const Object *object = found.Of(block);
  const auto *record = object == nullptr ? nullptr : GetIf<Tables::BlockRecord>(object);
  std::string name = record == nullptr ? std::string{} : record->Record.Name;
  if (name.starts_with('_'))
  {
    name.erase(0, 1);
  }
  std::ranges::transform(name, name.begin(), Ascii::Upper);
  return name;
}
} // namespace

void RaiseLeader(Entities::Leader &leader)
{
  const Entities::Leader fresh{};
  leader.ArrowheadType = leader.Arrowhead;
  leader.Gap = fresh.Gap;
  leader.Arrowhead = fresh.Arrowhead;
  leader.Dimasz = fresh.Dimasz;
  leader.Unknown2 = fresh.Unknown2;
  leader.Unknown3 = fresh.Unknown3;
  leader.RepeatedColor = fresh.RepeatedColor;
  leader.Color = fresh.Color;
  leader.Unknown5 = fresh.Unknown5;
  leader.Unknown6 = fresh.Unknown6;
}

auto LowerLeader(Entities::Leader &leader, const Lookup &found, Handle acad) -> bool
{
  std::vector<Xdata> &extended = leader.Base.Extended;
  const auto owned = std::ranges::find_if(extended, [acad](const Xdata &xdata) -> bool { return xdata.AppId == acad; });
  const Settings settings = SettingsOf(leader, found, owned == extended.end() ? nullptr : &*owned);
  const double scale = settings.Scale == 0 ? 1 : settings.Scale;
  int color = settings.Color;
  color = color == 0 ? leader.Common.Color.Index : color;
  color = color == 0 ? White : color;
  const Entities::Leader fresh{};
  leader.Gap = settings.Gap * scale;
  leader.Dimasz = settings.Size * scale;
  leader.Color = color;
  leader.RepeatedColor = color;
  leader.Unknown2 = fresh.Unknown2;
  leader.Unknown3 = true;
  leader.Unknown5 = fresh.Unknown5;
  leader.Unknown6 = fresh.Unknown6;
  leader.Arrowhead = settings.Arrow ? Blocked : 0;
  leader.ArrowheadType = fresh.ArrowheadType;
  leader.Unknown8 = fresh.Unknown8;
  leader.Unknown9 = fresh.Unknown9;
  if (!acad)
  {
    return false;
  }
  Xdata record = owned == extended.end() ? Xdata{.Application = {}, .AppId = acad, .Tags = {}} : std::move(*owned);
  if (owned != extended.end())
  {
    extended.erase(owned);
  }
  const std::vector<Tag> arrow{Tag{.Code = Short, .Value = Dimblk},
                               Tag{.Code = Text, .Value = ArrowName(found, settings.Arrow)}};
  std::vector<Tag> &groups = record.Tags;
  if (const auto opened = FindOverrides(groups); opened != groups.end())
  {
    groups.insert(opened, arrow.begin(), arrow.end());
  }
  else
  {
    groups.push_back(Tag{.Code = Text, .Value = std::string{Overrides}});
    groups.push_back(Tag{.Code = Brace, .Value = std::string{"{"}});
    groups.insert(groups.end(), arrow.begin(), arrow.end());
    groups.push_back(Tag{.Code = Brace, .Value = std::string{"}"}});
  }
  extended.push_back(std::move(record)); // AutoCAD's R14 save writes the application after any other.
  return true;
}

void BoxLeader(Entities::Leader &leader, const Lookup &found)
{
  const Object *object = leader.Annotation == 0 ? found.Of(leader.Note) : nullptr;
  const auto *text = object == nullptr ? nullptr : GetIf<Entities::MText>(object);
  leader.Height = text == nullptr ? 0 : text->ExtentHeight;
  leader.Width = text == nullptr ? 0 : text->ExtentWidth;
}
} // namespace DwgIO::Releases
