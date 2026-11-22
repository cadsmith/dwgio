/***********************************************************************************************************************
 *  @file      source/releases/visualstyle.cpp
 *  @brief     A visual style's properties as each release lays them out, and their record.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/visualstyle.hpp"

#include "objects/shapes/visualstyle.hpp"
#include "releases/palette.hpp"

#include "dwgio/color.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
using Objects::VisualStyles::Property;
using Objects::VisualStyles::Setting;
namespace Shapes = Objects::Shapes;

/**
 *  R2010's properties, the first of R2013's.
 */
constexpr std::size_t Shared = 28;

/**
 *  The flags the record lists below R2010: R2013's 58 and five more, which AutoCAD sets to 1 for a style it read.
 */
constexpr std::size_t Listed = 63;

/**
 *  The DXF codes of the properties that change between the older layout and R2010's: the face opacity and specular
 *  amount, the face modifiers whose bits 1 and 2 turn them on, the lighting quality, the line width, the short R2010
 *  dropped and R2007's last real.
 */
constexpr int Opacity = 40;
constexpr int Specular = 41;
constexpr int Modifiers = 90;
constexpr int Quality = 72;
constexpr int Width = 44;
constexpr int Dropped = 174;
constexpr int Last = 45;

/**
 *  The highest lighting quality the older layout knows.
 */
constexpr std::int32_t OlderQuality = 2;

/**
 *  The kind of the 2D wireframe style, whose defaults differ.
 */
constexpr int Wireframe = 4;

/**
 *  The long of the record's color for a color that is no true color.
 */
constexpr std::int64_t Untrue = -16777216;
constexpr std::uint32_t Method = 0xFF000000U;
constexpr std::uint32_t Rgb = 0xFFFFFFU;
constexpr std::uint32_t TrueColor = 0xC2000000U;

/**
 *  The group codes the record keeps a value of each kind under.
 */
constexpr int Marker = 102;
constexpr int Flag = 280;
constexpr int Whole = 90;
constexpr int Real = 140;
constexpr int Text = 1;
constexpr int Operation = 70;

/**
 *  A true color of R2004's long, as a color.
 */
auto MakeTrueColor(std::uint32_t rgb) -> Color
{
  return Color{.Index = 0, .Rgb = static_cast<std::int32_t>(TrueColor | (rgb & Rgb))};
}

/**
 *  What AutoCAD fills the 30 properties beyond R2010's with for a style whose record lacks them, and their flags: those
 *  of every style but the 2D wireframe's, whose first and third are the other way.
 */
auto MakeDefaults(int kind) -> std::vector<Property>
{
  std::vector<Property> full;
  full.reserve(Shapes::Typed.size());
  for (const Shapes::Slot slot : Shapes::Typed)
  {
    full.push_back(Property{.Code = slot.Code, .Value = Shapes::MakeZero(slot), .Flags = 1});
  }
  const bool wire = kind == Wireframe;
  const std::vector<Setting> beyond{wire,
                                    true,
                                    !wire,
                                    false,
                                    false,
                                    false,
                                    false,
                                    false,
                                    false,
                                    50,
                                    0.0,
                                    1.0,
                                    0,
                                    MakeTrueColor(0),
                                    50,
                                    3,
                                    MakeTrueColor(0xFF),
                                    false,
                                    50,
                                    50,
                                    50,
                                    false,
                                    50,
                                    Color{.Index = 256},
                                    1.0,
                                    2,
                                    std::string{"strokes_ogs.tif"},
                                    false,
                                    1.0,
                                    1.0};
  for (std::size_t at = 0; at < beyond.size(); ++at)
  {
    full.at(Shared + at).Value = beyond.at(at);
  }
  constexpr std::size_t Unset = 51; // The last edge color and its width, which AutoCAD flags 0.
  full.at(Unset).Flags = 0;
  full.at(Unset + 1).Flags = 0;
  return full;
}

/**
 *  Where a code lies in a layout; its size when it is not there.
 */
auto Place(std::span<const Shapes::Slot> layout, int code) -> std::size_t
{
  const auto found = std::ranges::find(layout, code, &Shapes::Slot::Code);
  return static_cast<std::size_t>(found - layout.begin());
}

/**
 *  A whole number the record holds under the marker, or the default.
 */
auto NumberOf(std::span<const Tag> kept, std::string_view name, std::int64_t otherwise) -> std::int64_t
{
  const Content *value = FindMarked(kept, name);
  const auto *number = value != nullptr ? std::get_if<std::int64_t>(value) : nullptr;
  return number != nullptr ? *number : otherwise;
}

/**
 *  The record's color of a property: its index where its red, green and blue say it is no true color, else those.
 */
auto ReadColorSetting(std::span<const Tag> kept, std::size_t at) -> std::optional<Setting>
{
  const Content *index = FindMarked(kept, std::format("RTVSPost2010Prop{}ColorIndex", at));
  const Content *rgb = FindMarked(kept, std::format("RTVSPost2010Prop{}ColorRGB", at));
  const auto *number = index != nullptr ? std::get_if<std::int64_t>(index) : nullptr;
  const auto *value = rgb != nullptr ? std::get_if<std::int64_t>(rgb) : nullptr;
  if (number == nullptr || value == nullptr)
  {
    return std::nullopt;
  }
  if ((static_cast<std::uint32_t>(*value) & Method) == Method)
  {
    return RaiseColor(Color{.Index = static_cast<int>(*number)});
  }
  return MakeTrueColor(static_cast<std::uint32_t>(*value));
}

/**
 *  The record's value of a property, as its slot stores it; empty when the record holds none of that kind.
 */
auto ReadSetting(std::span<const Tag> kept, std::size_t at, Shapes::Slot slot) -> std::optional<Setting>
{
  if (slot.As == Shapes::Stored::Color)
  {
    return ReadColorSetting(kept, at);
  }
  const Content *value = FindMarked(kept, std::format("RTVSPost2010Prop{}", at));
  const auto *number = value != nullptr ? std::get_if<std::int64_t>(value) : nullptr;
  const auto *real = value != nullptr ? std::get_if<double>(value) : nullptr;
  const auto *text = value != nullptr ? std::get_if<std::string>(value) : nullptr;
  switch (slot.As)
  {
  case Shapes::Stored::Flag:
    return number != nullptr ? std::optional<Setting>{*number != 0} : std::nullopt;
  case Shapes::Stored::Real:
    return real != nullptr ? std::optional<Setting>{*real} : std::nullopt;
  case Shapes::Stored::Text:
    return text != nullptr ? std::optional<Setting>{*text} : std::nullopt;
  default:
    return number != nullptr ? std::optional<Setting>{static_cast<std::int32_t>(*number)} : std::nullopt;
  }
}

/**
 *  The groups a property beyond R2010's keeps in the record: its value under its marker, a color as its nearest index
 *  and its red, green and blue, then its flags.
 */
void Record(const Property &property, std::size_t at, std::vector<Tag> &kept)
{
  const auto mark = [&kept](std::string name) -> void
  {
    kept.push_back(Tag{.Code = Marker, .Value = std::move(name)});
  };
  if (const auto *color = std::get_if<Color>(&property.Value))
  {
    const Color older = LowerColor(*color);
    const bool truly = color->Rgb && *color->Rgb != Unmethodical;
    mark(std::format("RTVSPost2010Prop{}ColorIndex", at));
    kept.push_back(Tag{.Code = Whole, .Value = std::int64_t{older.Index}});
    mark(std::format("RTVSPost2010Prop{}ColorRGB", at));
    kept.push_back(
        Tag{.Code = Whole,
            .Value = truly ? std::int64_t{static_cast<std::int32_t>(static_cast<std::uint32_t>(*color->Rgb) & Rgb)}
                           : Untrue});
  }
  else
  {
    mark(std::format("RTVSPost2010Prop{}", at));
    if (const auto *flag = std::get_if<bool>(&property.Value))
    {
      kept.push_back(Tag{.Code = Flag, .Value = std::int64_t{*flag ? 1 : 0}});
    }
    else if (const auto *number = std::get_if<std::int32_t>(&property.Value))
    {
      kept.push_back(Tag{.Code = Whole, .Value = std::int64_t{*number}});
    }
    else if (const auto *real = std::get_if<double>(&property.Value))
    {
      kept.push_back(Tag{.Code = Real, .Value = *real});
    }
    else
    {
      kept.push_back(Tag{.Code = Text, .Value = std::get<std::string>(property.Value)});
    }
  }
  mark(std::format("RTVSPost2010PropOp{}", at));
  kept.push_back(Tag{.Code = Operation, .Value = std::int64_t{property.Flags}});
}

/**
 *  The face modifiers of R2010's 28 properties.
 */
auto ModifiersOf(std::span<const Property> shared) -> std::int32_t
{
  const auto *value = std::get_if<std::int32_t>(&shared[Place(Shapes::Flagged, Modifiers)].Value);
  return value != nullptr ? *value : 0;
}

/**
 *  A real the older layout makes negative where the face modifiers leave its bit off, either way.
 */
void SetSign(Setting &value, bool shown)
{
  if (auto *real = std::get_if<double>(&value); real != nullptr && !shown)
  {
    *real = -*real;
  }
}

/**
 *  The 28 shared properties of the older layout, as R2010 holds them.
 */
void ClearSigns(std::span<Property> shared)
{
  const auto modifiers = static_cast<std::uint32_t>(ModifiersOf(shared));
  SetSign(shared[Place(Shapes::Flagged, Opacity)].Value, (modifiers & 1U) != 0);
  SetSign(shared[Place(Shapes::Flagged, Specular)].Value, (modifiers & 2U) != 0);
  Setting &width = shared[Place(Shapes::Flagged, Width)].Value;
  if (const auto *whole = std::get_if<std::int32_t>(&width))
  {
    width = static_cast<double>(*whole);
  }
}

/**
 *  A property of the shared 28 as the older layout holds it; the short R2010 dropped and R2007's last real 0.
 */
auto NarrowProperty(std::span<const Property> shared, int code) -> Setting
{
  if (code == Dropped)
  {
    return std::int32_t{0};
  }
  if (code == Last)
  {
    return 0.0;
  }
  Setting value = shared[Place(Shapes::Flagged, code)].Value;
  const auto modifiers = static_cast<std::uint32_t>(ModifiersOf(shared));
  if (code == Opacity || code == Specular)
  {
    SetSign(value, (modifiers & (code == Opacity ? 1U : 2U)) != 0);
  }
  if (const auto *real = std::get_if<double>(&value); real != nullptr && code == Width)
  {
    constexpr double Most = std::numeric_limits<std::int32_t>::max();
    value = static_cast<std::int32_t>(std::trunc(std::clamp(*real, -Most, Most)));
  }
  if (const auto *quality = std::get_if<std::int32_t>(&value); quality != nullptr && code == Quality)
  {
    value = std::min(*quality, OlderQuality);
  }
  return value;
}

void ConvertColors(std::span<Property> properties, Version release, bool upgrading)
{
  for (Property &property : properties)
  {
    if (auto *color = std::get_if<Color>(&property.Value); color != nullptr && release < Version::R2004)
    {
      *color = upgrading ? RaiseColor(*color) : LowerColor(*color);
    }
  }
}
} // namespace

auto RaiseProperties(const Objects::VisualStyle &style, Version from, std::span<const Tag> kept)
    -> std::vector<Property>
{
  std::vector<Property> full = MakeDefaults(style.Kind);
  const std::span<const Property> held = style.Properties;
  if (from >= Version::R2013)
  {
    for (std::size_t at = 0; at < std::min(held.size(), full.size()); ++at)
    {
      full[at].Value = held[at].Value;
      full[at].Flags = held[at].Flags;
    }
    return full;
  }
  const std::span<const Shapes::Slot> layout = Shapes::LayoutOf(from);
  for (std::size_t at = 0; at < Shared; ++at)
  {
    const std::size_t place = from >= Version::R2010 ? at : Place(layout, Shapes::Flagged.at(at).Code);
    if (place < held.size())
    {
      full[at].Value = held[place].Value;
      full[at].Flags = held[place].Flags;
    }
    if (from < Version::R2010)
    {
      full[at].Flags = static_cast<int>(NumberOf(kept, std::format("RTVSPropertyOp{}", at), 1));
    }
  }
  if (from < Version::R2010)
  {
    ClearSigns(std::span{full}.first(Shared));
  }
  ConvertColors(std::span{full}.first(Shared), from, true);
  for (std::size_t at = Shared; at < full.size(); ++at)
  {
    if (std::optional<Setting> value = ReadSetting(kept, at, Shapes::Typed.at(at)))
    {
      full[at].Value = std::move(*value);
      full[at].Flags = static_cast<int>(NumberOf(kept, std::format("RTVSPost2010PropOp{}", at), full[at].Flags));
    }
  }
  return full;
}

auto FitProperties(std::span<const Property> full, std::span<const Tag> kept, Version to) -> StyleLayout
{
  StyleLayout styling;
  if (to >= Version::R2013)
  {
    styling.Properties.assign(full.begin(), full.end());
    return styling;
  }
  const std::span<const Property> shared = full.first(Shared);
  for (std::size_t at = Shared; at < full.size(); ++at)
  {
    Record(full[at], at, styling.Kept);
  }
  if (to >= Version::R2010)
  {
    for (std::size_t at = 0; at < Shared; ++at)
    {
      styling.Properties.push_back(
          Property{.Code = Shapes::Flagged.at(at).Code, .Value = shared[at].Value, .Flags = shared[at].Flags});
    }
    return styling;
  }
  for (const Shapes::Slot slot : Shapes::LayoutOf(to))
  {
    styling.Properties.push_back(Property{.Code = slot.Code, .Value = NarrowProperty(shared, slot.Code), .Flags = 0});
  }
  ConvertColors(styling.Properties, to, false);
  for (std::size_t at = 0; at < Listed; ++at)
  {
    styling.Kept.push_back(Tag{.Code = Marker, .Value = std::format("RTVSPropertyOp{}", at)});
    const std::int64_t flags =
        at < full.size() ? full[at].Flags : NumberOf(kept, std::format("RTVSPropertyOp{}", at), 1);
    styling.Kept.push_back(Tag{.Code = Operation, .Value = flags});
  }
  return styling;
}

auto FindMarked(std::span<const Tag> kept, std::string_view name) -> const Content *
{
  for (std::size_t at = 0; at + 1 < kept.size(); ++at)
  {
    const auto *text = std::get_if<std::string>(&kept[at].Value);
    if (kept[at].Code == Marker && text != nullptr && *text == name)
    {
      return &kept[at + 1].Value;
    }
  }
  return nullptr;
}
} // namespace DwgIO::Releases
