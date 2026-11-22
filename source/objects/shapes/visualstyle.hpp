/***********************************************************************************************************************
 *  @file      source/objects/shapes/visualstyle.hpp
 *  @brief     How a VISUALSTYLE lies in the file, its properties release by release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_OBJECTS_SHAPES_VISUALSTYLE_HPP
#define DWGIO_OBJECTS_SHAPES_VISUALSTYLE_HPP

#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"
#include "objects/shape.hpp"

#include "dwgio/color.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace DwgIO::Objects::Shapes
{
enum class Stored : std::uint8_t
{
  Short,
  Long,
  Real,
  Color,
  Flag,
  Text,
  Byte,
};

struct Slot
{
  int Code;
  Stored As;
};

/**
 *  R14 to R2004, then R2007 with a real more.
 */
inline constexpr std::array<Slot, 30> Older{
    {{.Code = 71, .As = Stored::Short},  {.Code = 72, .As = Stored::Short},  {.Code = 73, .As = Stored::Short},
     {.Code = 40, .As = Stored::Real},   {.Code = 41, .As = Stored::Real},   {.Code = 63, .As = Stored::Color},
     {.Code = 90, .As = Stored::Long},   {.Code = 74, .As = Stored::Short},  {.Code = 91, .As = Stored::Long},
     {.Code = 64, .As = Stored::Color},  {.Code = 65, .As = Stored::Color},  {.Code = 75, .As = Stored::Short},
     {.Code = 42, .As = Stored::Real},   {.Code = 92, .As = Stored::Long},   {.Code = 66, .As = Stored::Color},
     {.Code = 43, .As = Stored::Real},   {.Code = 76, .As = Stored::Short},  {.Code = 77, .As = Stored::Short},
     {.Code = 78, .As = Stored::Short},  {.Code = 67, .As = Stored::Color},  {.Code = 79, .As = Stored::Short},
     {.Code = 170, .As = Stored::Byte},  {.Code = 171, .As = Stored::Short}, {.Code = 290, .As = Stored::Flag},
     {.Code = 174, .As = Stored::Short}, {.Code = 175, .As = Stored::Short}, {.Code = 93, .As = Stored::Long},
     {.Code = 44, .As = Stored::Long},   {.Code = 173, .As = Stored::Short}, {.Code = 45, .As = Stored::Real}}};

/**
 *  R2010, each property followed by its flags.
 */
inline constexpr std::array<Slot, 28> Flagged{
    {{.Code = 71, .As = Stored::Short},  {.Code = 72, .As = Stored::Short},  {.Code = 73, .As = Stored::Short},
     {.Code = 90, .As = Stored::Long},   {.Code = 40, .As = Stored::Real},   {.Code = 41, .As = Stored::Real},
     {.Code = 63, .As = Stored::Color},  {.Code = 74, .As = Stored::Short},  {.Code = 91, .As = Stored::Long},
     {.Code = 64, .As = Stored::Color},  {.Code = 65, .As = Stored::Color},  {.Code = 75, .As = Stored::Short},
     {.Code = 175, .As = Stored::Short}, {.Code = 42, .As = Stored::Real},   {.Code = 92, .As = Stored::Long},
     {.Code = 66, .As = Stored::Color},  {.Code = 43, .As = Stored::Real},   {.Code = 76, .As = Stored::Short},
     {.Code = 77, .As = Stored::Short},  {.Code = 78, .As = Stored::Short},  {.Code = 67, .As = Stored::Color},
     {.Code = 79, .As = Stored::Short},  {.Code = 170, .As = Stored::Short}, {.Code = 171, .As = Stored::Short},
     {.Code = 290, .As = Stored::Flag},  {.Code = 93, .As = Stored::Long},   {.Code = 44, .As = Stored::Real},
     {.Code = 173, .As = Stored::Short}}};

/**
 *  R2013 and R2018, each property followed by its flags: R2010's in its order, then thirty more.
 */
inline constexpr std::array<Slot, 58> Typed{
    {{.Code = 90, .As = Stored::Long},  {.Code = 90, .As = Stored::Long},  {.Code = 90, .As = Stored::Long},
     {.Code = 90, .As = Stored::Long},  {.Code = 40, .As = Stored::Real},  {.Code = 40, .As = Stored::Real},
     {.Code = 62, .As = Stored::Color}, {.Code = 90, .As = Stored::Long},  {.Code = 90, .As = Stored::Long},
     {.Code = 62, .As = Stored::Color}, {.Code = 62, .As = Stored::Color}, {.Code = 90, .As = Stored::Long},
     {.Code = 90, .As = Stored::Long},  {.Code = 40, .As = Stored::Real},  {.Code = 90, .As = Stored::Long},
     {.Code = 62, .As = Stored::Color}, {.Code = 40, .As = Stored::Real},  {.Code = 90, .As = Stored::Long},
     {.Code = 90, .As = Stored::Long},  {.Code = 90, .As = Stored::Long},  {.Code = 62, .As = Stored::Color},
     {.Code = 90, .As = Stored::Long},  {.Code = 90, .As = Stored::Long},  {.Code = 90, .As = Stored::Long},
     {.Code = 290, .As = Stored::Flag}, {.Code = 90, .As = Stored::Long},  {.Code = 40, .As = Stored::Real},
     {.Code = 90, .As = Stored::Long},  {.Code = 290, .As = Stored::Flag}, {.Code = 290, .As = Stored::Flag},
     {.Code = 290, .As = Stored::Flag}, {.Code = 290, .As = Stored::Flag}, {.Code = 290, .As = Stored::Flag},
     {.Code = 290, .As = Stored::Flag}, {.Code = 290, .As = Stored::Flag}, {.Code = 290, .As = Stored::Flag},
     {.Code = 290, .As = Stored::Flag}, {.Code = 90, .As = Stored::Long},  {.Code = 40, .As = Stored::Real},
     {.Code = 40, .As = Stored::Real},  {.Code = 90, .As = Stored::Long},  {.Code = 62, .As = Stored::Color},
     {.Code = 90, .As = Stored::Long},  {.Code = 90, .As = Stored::Long},  {.Code = 62, .As = Stored::Color},
     {.Code = 290, .As = Stored::Flag}, {.Code = 90, .As = Stored::Long},  {.Code = 90, .As = Stored::Long},
     {.Code = 90, .As = Stored::Long},  {.Code = 290, .As = Stored::Flag}, {.Code = 90, .As = Stored::Long},
     {.Code = 62, .As = Stored::Color}, {.Code = 40, .As = Stored::Real},  {.Code = 90, .As = Stored::Long},
     {.Code = 1, .As = Stored::Text},   {.Code = 290, .As = Stored::Flag}, {.Code = 40, .As = Stored::Real},
     {.Code = 40, .As = Stored::Real}}};

/**
 *  The R2007 real the older layout ends with.
 */
constexpr std::size_t Older2004 = 29;

inline auto LayoutOf(Version release) -> std::span<const Slot>
{
  if (release >= Version::R2013)
  {
    return Typed;
  }
  if (release >= Version::R2010)
  {
    return Flagged;
  }
  return std::span{Older}.first(release >= Version::R2007 ? Older.size() : Older2004);
}

inline auto ReadSetting(Stored stored, const Fields::Input &in) -> VisualStyles::Setting
{
  switch (stored)
  {
  case Stored::Short:
    return std::int32_t{in.In->BS()};
  case Stored::Long:
    return in.In->BL();
  case Stored::Real:
    return in.In->BD();
  case Stored::Color:
    return in.In->CMC(in.With.Release, Fields::TextIn(in));
  case Stored::Flag:
    return in.In->B();
  case Stored::Text:
    return Fields::TextCode::Read(in);
  default:
    return std::int32_t{in.In->RC()};
  }
}

/**
 *  Noted and zero when the property holds another kind.
 */
template <typename ValueT>
auto SettingAs(const VisualStyles::Setting &value, int code, const Fields::Output &to) -> const ValueT &
{
  static const ValueT zero{};
  if (const auto *held = std::get_if<ValueT>(&value))
  {
    return *held;
  }
  Fields::Note(to, IssueCode::Damaged, std::format("visual style property {} of another kind, written as zero", code));
  return zero;
}

/**
 *  Clamped and noted when out of range.
 */
inline void WriteShort(int value, const Fields::Output &to)
{
  Fields::Output writing = to;
  Fields::ShortCode::Write(value, writing);
}

inline void WriteSetting(Slot slot, const VisualStyles::Setting &value, const Fields::Output &to)
{
  switch (slot.As)
  {
  case Stored::Short:
    WriteShort(SettingAs<std::int32_t>(value, slot.Code, to), to);
    break;
  case Stored::Long:
    to.To->BL(SettingAs<std::int32_t>(value, slot.Code, to));
    break;
  case Stored::Real:
    to.To->BD(SettingAs<double>(value, slot.Code, to));
    break;
  case Stored::Color:
    to.To->CMC(SettingAs<DwgIO::Color>(value, slot.Code, to), to.With.Release, Fields::TextOut(to));
    break;
  case Stored::Flag:
    to.To->B(SettingAs<bool>(value, slot.Code, to));
    break;
  case Stored::Text:
    Fields::TextCode::Write(SettingAs<std::string>(value, slot.Code, to), to);
    break;
  default:
    to.To->RC(static_cast<std::uint8_t>(SettingAs<std::int32_t>(value, slot.Code, to)));
    break;
  }
}

inline auto MakeZero(Slot slot) -> VisualStyles::Setting
{
  switch (slot.As)
  {
  case Stored::Real:
    return 0.0;
  case Stored::Color:
    return VisualStyles::Setting{std::in_place_type<DwgIO::Color>};
  case Stored::Flag:
    return false;
  case Stored::Text:
    return std::string{};
  default:
    return std::int32_t{};
  }
}

/**
 *  From R2010 each property is followed by its flags.
 */
inline auto ReadProperties(VisualStyle &style, const Fields::Input &in) -> bool
{
  style.Properties.clear();
  for (const Slot slot : LayoutOf(in.With.Release))
  {
    VisualStyles::Property property{.Code = slot.Code, .Value = ReadSetting(slot.As, in), .Flags = 0};
    if (in.With.Release >= Version::R2010)
    {
      property.Flags = in.In->BS();
    }
    if (in.In->HasFailed())
    {
      return false;
    }
    style.Properties.push_back(std::move(property));
  }
  return true;
}

/**
 *  A property the style lacks is written as a zero with no flags, one beyond the layout left out, either noted.
 */
inline auto WriteProperties(const VisualStyle &style, const Fields::Output &to) -> bool
{
  const std::span<const Slot> layout = LayoutOf(to.With.Release);
  if (style.Properties.size() != layout.size())
  {
    Fields::Note(to, IssueCode::Damaged,
                 std::format("visual style of {} properties written with the {} of its layout", style.Properties.size(),
                             layout.size()));
  }
  for (std::size_t at = 0; at < layout.size(); ++at)
  {
    const bool held = at < style.Properties.size();
    if (held)
    {
      WriteSetting(layout[at], style.Properties[at].Value, to);
    }
    else
    {
      WriteSetting(layout[at], MakeZero(layout[at]), to);
    }
    if (to.With.Release >= Version::R2010)
    {
      WriteShort(held ? style.Properties[at].Flags : 0, to);
    }
  }
  return true;
}

} // namespace DwgIO::Objects::Shapes

namespace DwgIO::Objects
{
/**
 *  No chapter; stored under its class.
 *
 *  The internal bit comes before the properties from R2010, after them up to R2007.
 */
template <>
struct Shape<Objects::VisualStyle>
{
  static constexpr std::array<std::uint16_t, 0> Types{};
  static constexpr std::string_view Name = "VISUALSTYLE";

  static auto Layout()
  {
    using Objects::VisualStyle;
    return Fields::Layout(
        Fields::Text(&VisualStyle::Name), Fields::Short(&VisualStyle::Kind),
        Fields::Since(Version::R2010, Fields::Short(&VisualStyle::Version), Fields::Bit(&VisualStyle::Internal)),
        Fields::Custom(Shapes::ReadProperties, Shapes::WriteProperties),
        Fields::Until(Version::R2007, Fields::Bit(&VisualStyle::Internal)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_OBJECTS_SHAPES_VISUALSTYLE_HPP */
