/***********************************************************************************************************************
 *  @file      source/releases/dimstyle.cpp
 *  @brief     A dimension style, and the header's dimension variables, as another release holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/dimstyle.hpp"

#include "ascii.hpp"
#include "releases/common.hpp"
#include "releases/passage.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <format>
#include <string>
#include <string_view>

namespace DwgIO::Releases
{
namespace
{
constexpr int Stacked = 0;
constexpr int Unstacked = 2;
constexpr int Architectural = 4;
constexpr int Fractional = 5;
constexpr int Windows = 6;
constexpr int Decimal = 2;

constexpr int BestFit = 3;
constexpr int OnLeader = 1;
constexpr int Freely = 2;

/**
 *  A dimension style's arrows as the other release holds them.
 */
void ConvertArrows(Tables::DimStyle &style, const Passage &passage, Version to)
{
  const Handle self = style.Record.Base.Self;
  if (to >= Version::R2000)
  {
    style.Dimblk = ArrowRecord(style.Arrows.Dimblk, passage, self);
    style.Dimblk1 = ArrowRecord(style.Arrows.Dimblk1, passage, self);
    style.Dimblk2 = ArrowRecord(style.Arrows.Dimblk2, passage, self);
    return;
  }
  style.Arrows.Dimblk = ArrowName(*passage.Drawing, style.Dimblk);
  style.Arrows.Dimblk1 = ArrowName(*passage.Drawing, style.Dimblk1);
  style.Arrows.Dimblk2 = ArrowName(*passage.Drawing, style.Dimblk2);
  style.Dimldrblk = {};
}

/**
 *  A dimension style as R2000 holds it: the units and the fit from what R14 held.
 */
void RaiseDimStyle(Tables::DimStyle &style)
{
  const Units units = SplitUnits(style.Dimunit);
  const Fit fit = SplitFit(style.Dimfit);
  style.Dimlunit = units.Lunit;
  style.Dimfrac = units.Frac;
  style.Dimatfit = fit.Atfit;
  style.Dimtmove = fit.Tmove;
}

/**
 *  A dimension style as R14 holds it: the units and the fit joined, what came with R2000 at the kind's defaults.
 */
void LowerDimStyle(Tables::DimStyle &style)
{
  style.Dimunit = JoinUnits(Units{.Lunit = style.Dimlunit, .Frac = style.Dimfrac});
  style.Dimfit = JoinFit(Fit{.Atfit = style.Dimatfit, .Tmove = style.Dimtmove});
  const Tables::DimStyle fresh;
  style.Dimazin = fresh.Dimazin;
  style.Dimaltrnd = fresh.Dimaltrnd;
  style.Dimadec = fresh.Dimadec;
  style.Dimfrac = fresh.Dimfrac;
  style.Dimlunit = fresh.Dimlunit;
  style.Dimdsep = fresh.Dimdsep;
  style.Dimtmove = fresh.Dimtmove;
  style.Dimatfit = fresh.Dimatfit;
  style.Dimlwd = fresh.Dimlwd;
  style.Dimlwe = fresh.Dimlwe;
  style.Dimldrblk = fresh.Dimldrblk;
  style.Dimblk = fresh.Dimblk;
  style.Dimblk1 = fresh.Dimblk1;
  style.Dimblk2 = fresh.Dimblk2;
}
} // namespace

auto ArrowName(const Drawing &drawing, Handle block) -> std::string
{
  const Object *object = Find(drawing, block);
  const auto *record = object == nullptr ? nullptr : GetIf<Tables::BlockRecord>(object);
  if (record == nullptr)
  {
    return {};
  }
  const std::string &name = record->Record.Name;
  return name.starts_with('_') ? name.substr(1) : name;
}

auto ArrowRecord(std::string_view name, const Passage &passage, Handle self) -> Handle
{
  if (name.empty())
  {
    return {};
  }
  for (const Tables::BlockRecord *record : View::Records<Tables::BlockRecord>(*passage.Drawing))
  {
    const std::string_view named = record->Record.Name;
    const std::string_view bare = named.starts_with('_') ? named.substr(1) : named;
    if (Ascii::Same(named, name) || Ascii::Same(bare, name))
    {
      return record->Record.Base.Self;
    }
  }
  Note(passage, self, std::format("dimension block {} missing", name));
  return {};
}

auto SplitUnits(int dimunit) -> Units
{
  switch (dimunit)
  {
  case 1:
  case 2:
  case 3:
    return {.Lunit = dimunit, .Frac = Stacked};
  case 4:
    return {.Lunit = Architectural, .Frac = Stacked};
  case 5:
    return {.Lunit = Fractional, .Frac = Stacked};
  case 6:
    return {.Lunit = Architectural, .Frac = Unstacked};
  case 7:
    return {.Lunit = Fractional, .Frac = Unstacked};
  case 8:
    return {.Lunit = Windows, .Frac = Stacked};
  default:
    return {.Lunit = Decimal, .Frac = Stacked};
  }
}

auto JoinUnits(Units units) -> int
{
  switch (units.Lunit)
  {
  case 1:
  case 2:
  case 3:
    return units.Lunit;
  case Architectural:
    return units.Frac == Unstacked ? 6 : 4;
  case Fractional:
    return units.Frac == Unstacked ? 7 : 5;
  case Windows:
    return 8;
  default:
    return Decimal;
  }
}

auto SplitFit(int dimfit) -> Fit
{
  switch (dimfit)
  {
  case 0:
  case 1:
  case 2:
  case 3:
    return {.Atfit = dimfit, .Tmove = 0};
  case 4:
    return {.Atfit = BestFit, .Tmove = OnLeader};
  case 5:
    return {.Atfit = BestFit, .Tmove = Freely};
  default:
    return {.Atfit = BestFit, .Tmove = 0};
  }
}

auto JoinFit(Fit fit) -> int
{
  if (fit.Tmove == OnLeader)
  {
    return 4;
  }
  if (fit.Tmove == Freely)
  {
    return 5;
  }
  return fit.Atfit >= 0 && fit.Atfit <= BestFit ? fit.Atfit : BestFit;
}

void ConvertDimStyle(Tables::DimStyle &kind, const Passage &passage, const Crossings &crossing)
{
  if (crossing.R2000Up || crossing.R2000Down)
  {
    ConvertArrows(kind, passage, passage.To);
  }
  if (crossing.R2000Down)
  {
    LowerDimStyle(kind);
  }
  if (crossing.R2000Up)
  {
    RaiseDimStyle(kind);
  }
  if (crossing.R2004Down)
  {
    const Handle self = kind.Record.Base.Self;
    FadeColor(kind.Dimclrd, passage, self);
    FadeColor(kind.Dimclre, passage, self);
    FadeColor(kind.Dimclrt, passage, self);
  }
}
} // namespace DwgIO::Releases
