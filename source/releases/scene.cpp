/***********************************************************************************************************************
 *  @file      source/releases/scene.cpp
 *  @brief     The helix, light and visual style as the release a save writes holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/scene.hpp"

#include "releases/palette.hpp"

#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/helix.hpp"
#include "dwgio/entities/light.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
struct Notes
{
  std::vector<Issue> *Issues{};
  Handle Self{};
  std::string What{};
};

/**
 *  A color as R2000 holds it: no method as 257, a true color dropped and noted with its names and reference.
 */
void LowerColor(Color &color, const Notes &noting)
{
  if (color.Rgb == Unmethodical)
  {
    color = Color{.Index = UnmethodicalIndex};
    return;
  }
  if (color.Rgb)
  {
    noting.Issues->push_back(Issue{.Code = IssueCode::NotKept,
                                   .Subject = noting.Self,
                                   .What = std::format("true color of {} not kept by R2000", noting.What)});
    color.Rgb.reset();
    color.Names.reset();
    color.Reference.reset();
  }
}

/**
 *  The extended data AutoCAD marks a light and a visual style with below R2018: ACAD's "AcDbSavedByObjectVersion" and a
 *  short 0.
 */
constexpr std::string_view Marker = "AcDbSavedByObjectVersion";

/**
 *  The ACAD application's record, or null.
 */
auto FindAcad(const Drawing &drawing) -> Handle
{
  for (const Object &object : drawing.Objects)
  {
    if (const auto *appid = GetIf<Tables::AppId>(&object); appid != nullptr && appid->Record.Name == "ACAD")
    {
      return appid->Record.Base.Self;
    }
  }
  return {};
}

auto IsMarked(const Xdata &record, Handle acad) -> bool
{
  const auto *text = record.Tags.empty() ? nullptr : std::get_if<std::string>(&record.Tags.front().Value);
  return record.AppId == acad && text != nullptr && *text == Marker;
}

/**
 *  AutoCAD's saves below R2018 add the marker to an object that lacks it, and drop it when upgrading to R2018.
 *
 *  An older AutoCAD's R2007 file has none, and AutoCAD refuses its visual styles saved below R2007 without one.
 */
void ConvertMarker(Base &base, Way way)
{
  const Handle acad = FindAcad(*way.Drawing);
  const auto marked =
      std::ranges::find_if(base.Extended, [acad](const Xdata &one) -> bool { return IsMarked(one, acad); });
  if (way.To != Version::R2018)
  {
    if (!acad || marked != base.Extended.end())
    {
      return;
    }
    const std::vector<Tag> groups{Tag{.Code = 1000, .Value = std::string{Marker}},
                                  Tag{.Code = 1070, .Value = std::int64_t{0}}};
    const auto own = std::ranges::find(base.Extended, acad, &Xdata::AppId);
    if (own == base.Extended.end())
    {
      base.Extended.push_back(Xdata{.Application = {}, .AppId = acad, .Tags = groups});
    }
    else
    {
      own->Tags.insert(own->Tags.begin(), groups.begin(), groups.end());
    }
  }
  else if (marked != base.Extended.end())
  {
    const std::size_t taken = std::min<std::size_t>(2, marked->Tags.size());
    marked->Tags.erase(marked->Tags.begin(), marked->Tags.begin() + static_cast<std::ptrdiff_t>(taken));
    if (marked->Tags.empty())
    {
      base.Extended.erase(marked);
    }
  }
}

/**
 *  A color across R2004, either way.
 */
void ConvertColor(Color &color, Way way, const Notes &noting)
{
  if (way.From >= Version::R2004 && way.To < Version::R2004)
  {
    LowerColor(color, noting);
  }
  if (way.From < Version::R2004 && way.To >= Version::R2004)
  {
    color = RaiseColor(color);
  }
}

/**
 *  A helix upgraded to R2013 takes AutoCAD's flags and knot parameter of a spline by fit points, or by control points.
 */
void ConvertHelix(Entities::Helix &helix, Way way)
{
  constexpr std::int32_t ByFits = 9;
  constexpr std::int32_t Custom = 15;
  if (way.From < Version::R2013 && way.To >= Version::R2013)
  {
    helix.Curve.Options = helix.Curve.FitTolerance ? ByFits : 0;
    helix.Curve.Parameter = helix.Curve.FitTolerance ? 0 : Custom;
  }
}

} // namespace

void ConvertScene(Object &object, Way way, std::vector<Issue> &issues)
{
  if (way.From == way.To)
  {
    return;
  }
  const Notes noting{.Issues = &issues,
                     .Self = BaseOf(object).Self,
                     .What = std::format("{} {}", TypeOf(object), HexOf(BaseOf(object).Self))};
  if (auto *helix = GetIf<Entities::Helix>(&object))
  {
    ConvertHelix(*helix, way);
  }
  else if (auto *light = GetIf<Entities::Light>(&object))
  {
    ConvertColor(light->Color, way, noting);
    ConvertMarker(light->Base, way);
  }
  else if (auto *style = GetIf<Objects::VisualStyle>(&object))
  {
    ConvertMarker(style->Base, way); // Its properties are of the release already: see releases/roundtrip.hpp.
  }
}
} // namespace DwgIO::Releases
