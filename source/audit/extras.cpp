/***********************************************************************************************************************
 *  @file      source/audit/extras.cpp
 *  @brief     The audit of classes, extended data, lineweights and colors.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "audit/checks.hpp"
#include "audit/common.hpp"
#include "classes/catalog.hpp"
#include "making/records.hpp"
#include "objects/storage.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/base.hpp"
#include "dwgio/class.hpp"
#include "dwgio/color.hpp"
#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/lineweight.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>

namespace DwgIO::Audit
{
namespace
{
/**
 *  The first type code a class takes.
 */
constexpr int FirstClass = 500;
constexpr int ByLayerColor = 256;
constexpr int White = 7;

auto Widthly(Lineweight lineweight) -> bool
{
  return MillimetersOf(lineweight) || (lineweight >= Lineweight::ByLayer && lineweight <= Lineweight::Default);
}

/**
 *  Every object of a kind stored under a class has the class declared, since a save leaves out every object whose class
 *  is not; a class AutoCAD declares is added as it declares it, one no table knows is left undeclared.
 */
void CheckClasses(Drawing &drawing, Report &report)
{
  std::unordered_set<std::string> asked;
  for (const Object &object : drawing.Objects)
  {
    const std::string_view name = Objects::ClassOf(object, drawing.Release);
    const bool declared =
        std::ranges::any_of(drawing.Classes, [name](const Class &one) -> bool { return one.DxfName == name; });
    if (name.empty() || declared || !asked.insert(std::string{name}).second)
    {
      continue;
    }
    std::optional<Class> made = Classes::FindDeclared(name);
    if (!made)
    {
      NoteUnfixed(report, Finding{.Level = Severity::Error,
                                  .What = std::format("class {} not declared", name),
                                  .Subject = BaseOf(object).Self,
                                  .Fixed = false});
      continue;
    }
    NoteFailure(report, std::format("class {} not declared", name), BaseOf(object).Self);
    made->Number = FirstClass + static_cast<int>(drawing.Classes.size());
    drawing.Classes.push_back(std::move(*made));
  }
}

/**
 *  The extended data of every object names applications the drawing registers; the rest is dropped, since AutoCAD
 *  refuses a drawing with extended data of an application it has not.
 */
void CheckXdata(Drawing &drawing, Report &report)
{
  const Index index{drawing};
  for (Object &object : drawing.Objects)
  {
    Base &base = BaseOf(object);
    std::erase_if(base.Extended,
                  [&](const Xdata &xdata) -> bool
                  {
                    const Object *application = index.At(xdata.AppId);
                    if (application != nullptr && Holds<Tables::AppId>(*application))
                    {
                      return false;
                    }
                    NoteFailure(report,
                                std::format("application {} of the extended data of {} {} missing", HexOf(xdata.AppId),
                                            TypeOf(object), HexOf(base.Self)),
                                base.Self);
                    return true;
                  });
  }
}

/**
 *  Every entity's lineweight and color, and every layer's, are codes a file keeps; AutoCAD opens a drawing with a
 *  lineweight it does not know as it is, and its audit fixes a color out of range.
 *
 *  A true color's index is 0, as AutoCAD writes it before the true color from R2004.
 */
void CheckWidths(Drawing &drawing, Report &report)
{
  for (Object &object : drawing.Objects)
  {
    const Handle self = BaseOf(object).Self;
    Lineweight *lineweight = nullptr;
    Color *color = nullptr;
    Lineweight width = Lineweight::ByLayer;
    int hue = ByLayerColor;
    bool colored = true;
    if (Common *common = CommonOf(object); common != nullptr)
    {
      lineweight = common->Lineweight ? &*common->Lineweight : nullptr;
      color = &common->Color;
    }
    else if (auto *layer = GetIf<Tables::Layer>(&object); layer != nullptr)
    {
      lineweight = &layer->Lineweight;
      color = &layer->Color;
      width = Lineweight::Default;
      hue = White;
      colored = false;
    }
    if (lineweight != nullptr && !Widthly(*lineweight))
    {
      NoteWarning(
          report,
          std::format("lineweight {} of {} {} unknown", std::to_underlying(*lineweight), TypeOf(object), HexOf(self)),
          self);
      *lineweight = width;
    }
    if (color == nullptr)
    {
      continue;
    }
    const int lowest = colored || color->Rgb ? 0 : 1;
    const int highest = colored ? ByLayerColor : ByLayerColor - 1;
    if (color->Index < lowest || color->Index > highest)
    {
      NoteFailure(report, std::format("color {} of {} {} out of range", color->Index, TypeOf(object), HexOf(self)),
                  self);
      color->Index = hue;
    }
  }
}
} // namespace

void Extras(Drawing &drawing, Report &report)
{
  CheckClasses(drawing, report);
  CheckXdata(drawing, report);
  CheckWidths(drawing, report);
}
} // namespace DwgIO::Audit
