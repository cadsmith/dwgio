/***********************************************************************************************************************
 *  @file      source/releases/acis.cpp
 *  @brief     The ACIS entities as the release a save writes holds them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/acis.hpp"

#include "acis/convert.hpp"
#include "entities/shapes/geometry.hpp"
#include "releases/guid.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/geometry.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
template <typename KindT>
constexpr bool Acis = false;

template <>
constexpr bool Acis<Entities::Region> = true;

template <>
constexpr bool Acis<Entities::Solid3d> = true;

template <>
constexpr bool Acis<Entities::Body> = true;

template <Entities::SurfaceKind KindV>
constexpr bool Acis<Entities::SurfaceOf<KindV>> = true;

/**
 *  Whether AutoCAD remakes a kind's body when it reads it from an older release: a procedural surface's.
 */
template <typename KindT>
constexpr bool Procedural =
    std::is_same_v<KindT, Entities::ExtrudedSurface> || std::is_same_v<KindT, Entities::RevolvedSurface> ||
    std::is_same_v<KindT, Entities::LoftedSurface> || std::is_same_v<KindT, Entities::SweptSurface>;

/**
 *  The ACAD extended data AutoCAD gives an extruded or swept surface before R2007: the text `AcDbSavedByObjectVersion`
 *  and a short of 0. AutoCAD refuses such a surface of R2004 without it.
 */
constexpr int TextCode = 1000;
constexpr int ShortCode = 1070;
constexpr std::string_view Saved = "AcDbSavedByObjectVersion";

auto IsSaved(const Xdata &xdata) -> bool
{
  if (xdata.Tags.size() != 2)
  {
    return false;
  }
  const std::string *text = std::get_if<std::string>(&xdata.Tags.front().Value);
  return text != nullptr && *text == Saved;
}

/**
 *  The geometry's fields of R2007 and R2013 across them; a NURBS surface's second stream lost across R2013.
 */
void Across(Entities::Geometry &geometry, Version from, Version to)
{
  if (to < Version::R2007)
  {
    geometry.Unknown1 = 0;
  }
  if ((from >= Version::R2013) != (to >= Version::R2013))
  {
    geometry.Second = Entities::Acis{};
  }
  if (to < Version::R2013)
  {
    geometry.Stored = false;
    geometry.Guid.clear();
    geometry.Unknown2 = 0;
  }
  else if (from < Version::R2013)
  {
    geometry.Guid = Entities::Shapes::NoGuid; // What AutoCAD writes for a body not in the data store.
  }
}

/**
 *  A body as the release holds it in the object, or as it was when it cannot be: FindStranding leaves such an entity
 *  out. A second stream that cannot be is dropped.
 */
void ConvertBody(Entities::Geometry &geometry, Version to, bool remade)
{
  if (std::expected<Entities::Acis, std::string> body = Acis::Convert(geometry.Body, to, remade))
  {
    geometry.Body = std::move(*body);
  }
  if (std::expected<Entities::Acis, std::string> second = Acis::Convert(geometry.Second, to, remade))
  {
    geometry.Second = std::move(*second);
  }
  else
  {
    geometry.Second = Entities::Acis{};
  }
}

/**
 *  An ACIS entity upgraded to R2013: its body moves into the data store, which MakeStore makes, and its GUID back from
 *  the extended data that held it below R2013.
 *
 *  The entity says its body is in the store only with a GUID, and a NURBS surface never, as AutoCAD writes them;
 *  AutoCAD refuses a file whose entity says so without one.
 */
template <typename KindT>
void RaiseAcis(KindT &kind, const Drawing &drawing)
{
  const bool guided = TakeGuid(kind.Base, kind.Geometry, drawing);
  const bool bodied = kind.Geometry.Body.Version != 0;
  if constexpr (std::is_same_v<KindT, Entities::Surface> || std::is_same_v<KindT, Entities::NurbSurface>)
  {
    kind.Geometry.Stored = false;
  }
  else
  {
    kind.Geometry.Stored = guided && bodied;
  }
  if (!kind.Geometry.Stored)
  {
    kind.Geometry.Guid = Entities::Shapes::NoGuid;
  }
  kind.Base.Stored = bodied; // Below R2013 an object has no such bit.
}

/**
 *  The saved-by-version extended data of an extruded or swept surface, added when downgrading below R2007, dropped when
 *  upgrading from it; an issue when the drawing has no ACAD application to give it.
 */
auto ConvertSavedVersion(Base &base, const Drawing &drawing, Version to) -> std::optional<Issue>
{
  if (to >= Version::R2007)
  {
    std::erase_if(base.Extended, IsSaved);
    return std::nullopt;
  }
  if (std::ranges::any_of(base.Extended, IsSaved))
  {
    return std::nullopt;
  }
  const auto *acad = View::Record<Tables::AppId>(drawing, "ACAD");
  if (acad == nullptr)
  {
    return Issue{.Code = IssueCode::Damaged,
                 .Subject = base.Self,
                 .What = std::format("application ACAD missing for the version of {}", HexOf(base.Self))};
  }
  base.Extended.push_back(Xdata{
      .Application = {},
      .AppId = acad->Record.Base.Self,
      .Tags = {Tag{.Code = TextCode, .Value = std::string{Saved}}, Tag{.Code = ShortCode, .Value = std::int64_t{0}}}});
  return std::nullopt;
}

/**
 *  A history node of an extrusion or a sweep with the saved-by-version extended data its surface has.
 */
void ConvertHistoryNode(Translation &translation, const Drawing &drawing, Version to)
{
  auto *unknown = GetIf<Unknown>(&translation.Object);
  const std::string_view name = unknown != nullptr ? unknown->Class.View() : std::string_view{};
  if (name != "ACSH_EXTRUSION_CLASS" && name != "ACSH_SWEEP_CLASS")
  {
    return;
  }
  if (std::optional<Issue> issue = ConvertSavedVersion(unknown->Base, drawing, to))
  {
    translation.Issues.push_back(std::move(*issue));
  }
}

/**
 *  A surface as the kind of the other name.
 */
template <typename ToT, typename FromT>
auto AsKind(FromT &surface) -> Object
{
  return ToT{.Base = std::move(surface.Base),
             .Common = std::move(surface.Common),
             .Geometry = std::move(surface.Geometry),
             .UIsolines = surface.UIsolines,
             .VIsolines = surface.VIsolines,
             .Data = {}};
}
} // namespace

void ConvertAcis(Translation &translation, const Drawing &drawing, Version to)
{
  const Version from = drawing.Release;
  Visit(
      [&translation, &drawing, from, to](auto &kind) -> void
      {
        using KindT = std::remove_cvref_t<decltype(kind)>;
        if constexpr (Acis<KindT>)
        {
          if (from >= Version::R2013 && to < Version::R2013)
          {
            KeepGuid(kind.Base, kind.Geometry, drawing);
          }
          Across(kind.Geometry, from, to);
          if (to < Version::R2013)
          {
            ConvertBody(kind.Geometry, to, Procedural<KindT>);
          }
          else if (from < Version::R2013)
          {
            RaiseAcis(kind, drawing);
          }
        }
        if constexpr (std::is_same_v<KindT, Entities::Solid3d>)
        {
          kind.History = to < Version::R2007 ? Handle{} : kind.History;
        }
        if constexpr (std::is_same_v<KindT, Entities::ExtrudedSurface> || std::is_same_v<KindT, Entities::SweptSurface>)
        {
          if (std::optional<Issue> issue = ConvertSavedVersion(kind.Base, drawing, to))
          {
            translation.Issues.push_back(std::move(*issue));
          }
        }
      },
      translation.Object);
  Object &object = translation.Object;
  ConvertHistoryNode(translation, drawing, to);
  if (auto *plain = GetIf<Entities::Surface>(&object); plain != nullptr && to >= Version::R2013)
  {
    object = AsKind<Entities::NurbSurface>(*plain);
  }
  else if (auto *nurbs = GetIf<Entities::NurbSurface>(&object); nurbs != nullptr && to < Version::R2013)
  {
    object = AsKind<Entities::Surface>(*nurbs);
  }
}

auto FindStranding(const Object &object, Version from, Version to) -> std::optional<std::string>
{
  const Entities::Geometry *geometry = GeometryOf(object);
  if (geometry == nullptr)
  {
    return std::nullopt;
  }
  const Base &base = BaseOf(object);
  const std::string what = std::format("{} {} left out", TypeOf(object), HexOf(base.Self));
  if (from >= Version::R2013 && to < Version::R2013 && geometry->Body.Version == 0 && base.Stored)
  {
    return std::format("{}, its body not found in the data store", what);
  }
  const std::expected<Entities::Acis, std::string> body = Acis::Convert(geometry->Body, to, IsProcedural(object));
  if (!body)
  {
    return std::format("{}, its body cannot be written as {}: {}", what, NameOf(to), body.error());
  }
  return std::nullopt;
}

auto GeometryOf(Object &object) -> Entities::Geometry *
{
  return Visit(
      [](auto &kind) -> Entities::Geometry *
      {
        if constexpr (Acis<std::remove_cvref_t<decltype(kind)>>)
        {
          return &kind.Geometry;
        }
        else
        {
          return nullptr;
        }
      },
      object);
}

auto GeometryOf(const Object &object) -> const Entities::Geometry *
{
  return Visit(
      [](const auto &kind) -> const Entities::Geometry *
      {
        if constexpr (Acis<std::remove_cvref_t<decltype(kind)>>)
        {
          return &kind.Geometry;
        }
        else
        {
          return nullptr;
        }
      },
      object);
}

auto IsProcedural(const Object &object) -> bool
{
  return Visit([](const auto &kind) -> bool { return Procedural<std::remove_cvref_t<decltype(kind)>>; }, object);
}
} // namespace DwgIO::Releases
