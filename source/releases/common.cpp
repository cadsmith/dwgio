/***********************************************************************************************************************
 *  @file      source/releases/common.cpp
 *  @brief     An entity's common data as another release holds it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/common.hpp"

#include "releases/chain.hpp"
#include "releases/lookup.hpp"
#include "releases/passage.hpp"

#include "dwgio/base.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/lineweight.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>
#include <utility>
#include <variant>

namespace DwgIO::Releases
{
namespace
{
/**
 *  R14's lineweight extended data: ACAD's "RTLwPs" text and a short of 128 plus the code.
 */
constexpr int TextCode = 1000;
constexpr int ShortCode = 1070;
constexpr std::int64_t Lifted = 128;
constexpr const char *Width = "RTLwPs";

auto IsWidth(const Xdata &xdata) -> bool
{
  return xdata.Tags.size() >= 2 && xdata.Tags[0].Code == TextCode && xdata.Tags[1].Code == ShortCode &&
         std::holds_alternative<std::string>(xdata.Tags[0].Value) &&
         std::get<std::string>(xdata.Tags[0].Value) == Width &&
         std::holds_alternative<std::int64_t>(xdata.Tags[1].Value);
}

/**
 *  An entity's common data as R2000 holds it, from R14's.
 */
void RaiseCommon(Common &common, Base &base, const Header &header)
{
  if (common.LinetypeOrigin == Origin::Record)
  {
    if (common.Linetype == header.LTypeByLayer)
    {
      common.LinetypeOrigin = Origin::ByLayer;
      common.Linetype = {};
    }
    else if (common.Linetype == header.LTypeByBlock)
    {
      common.LinetypeOrigin = Origin::ByBlock;
      common.Linetype = {};
    }
    else if (common.Linetype == header.LTypeContinuous)
    {
      common.LinetypeOrigin = Origin::Continuous;
      common.Linetype = {};
    }
  }
  common.Lineweight = Lineweight::ByLayer;
  std::erase_if(base.Extended,
                [&common](const Xdata &xdata) -> bool
                {
                  if (!IsWidth(xdata))
                  {
                    return false;
                  }
                  common.Lineweight = static_cast<Lineweight>(std::get<std::int64_t>(xdata.Tags[1].Value) - Lifted);
                  return true;
                });
}

/**
 *  An entity's common data as R14 holds it, from R2000's.
 */
void LowerCommon(Common &common, Base &base, const Passage &passage)
{
  const Header &header = passage.Drawing->Header;
  if (common.LinetypeOrigin == Origin::ByBlock || common.LinetypeOrigin == Origin::Continuous)
  {
    common.Linetype = common.LinetypeOrigin == Origin::ByBlock ? header.LTypeByBlock : header.LTypeContinuous;
    common.LinetypeOrigin = Origin::Record;
  }
  if (common.PlotstyleOrigin == Origin::Record)
  {
    Note(passage, base.Self, std::format("plot style of {} not kept by R14", passage.What));
  }
  common.PlotstyleOrigin = Origin::ByLayer;
  common.Plotstyle = {};
  if (common.Lineweight && common.Lineweight != Lineweight::ByLayer)
  {
    const auto *acad = passage.Lookup->Record<Tables::AppId>("ACAD");
    if (acad == nullptr)
    {
      Note(passage, base.Self, std::format("application ACAD missing for the lineweight of {}", passage.What));
    }
    else
    {
      base.Extended.push_back(
          Xdata{.Application = {},
                .AppId = acad->Record.Base.Self,
                .Tags = {Tag{.Code = TextCode, .Value = std::string{Width}},
                         Tag{.Code = ShortCode, .Value = Lifted + std::to_underlying(*common.Lineweight)}}});
    }
  }
  common.Lineweight.reset();
}
} // namespace

void ConvertCommon(Common &common, Base &base, const Passage &passage)
{
  const Crossings crossing = CrossingsOf(passage.Drawing->Release, passage.To);
  if (crossing.R2000Up)
  {
    RaiseCommon(common, base, passage.Drawing->Header);
  }
  if (crossing.R2000Down)
  {
    LowerCommon(common, base, passage);
  }
  if (crossing.R2004Up)
  {
    common.Previous = {};
    common.Next = {};
  }
  if (crossing.R2004Down)
  {
    LinkEntity(common, base, *passage.Lookup);
    FadeColor(common.Color, passage, base.Self);
  }
}

void FadeColor(Color &color, const Passage &passage, Handle self)
{
  if (color.Rgb || color.Names || color.Reference)
  {
    Note(passage, self, std::format("true color of {} not kept by R2000", passage.What));
    color.Rgb.reset();
    color.Names.reset();
    color.Reference.reset();
  }
  if (color.Transparency)
  {
    Note(passage, self, std::format("transparency of {} not kept by R2000", passage.What));
    color.Transparency.reset();
  }
}
} // namespace DwgIO::Releases
