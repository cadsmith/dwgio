/***********************************************************************************************************************
 *  @file      source/releases/object.cpp
 *  @brief     An object as another release holds it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/object.hpp"

#include "releases/acis.hpp"
#include "releases/chain.hpp"
#include "releases/common.hpp"
#include "releases/dimstyle.hpp"
#include "releases/entities.hpp"
#include "releases/layouts.hpp"
#include "releases/lookup.hpp"
#include "releases/passage.hpp"
#include "releases/scene.hpp"
#include "releases/styles.hpp"
#include "releases/viewport.hpp"

#include "dwgio/base.hpp"
#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/hatch.hpp"
#include "dwgio/entities/insert.hpp"
#include "dwgio/entities/ole2frame.hpp"
#include "dwgio/entities/polyline.hpp"
#include "dwgio/entities/raster.hpp"
#include "dwgio/entities/spline.hpp"
#include "dwgio/entities/viewport.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/mleaderstyle.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/plotsettings.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <format>
#include <type_traits>

namespace DwgIO::Releases
{
namespace
{
/**
 *  Overloaded in releases/entities.hpp for MULTILEADER, LEADER and MTEXT.
 */
template <typename KindT>
void ConvertAnnotation([[maybe_unused]] KindT &kind, [[maybe_unused]] const Passage &passage,
                       [[maybe_unused]] const Crossings &crossing)
{
}

template <typename KindT>
void ConvertCurve(KindT &kind, const Crossings &crossing)
{
  if constexpr (std::is_same_v<KindT, Entities::Spline>)
  {
    ConvertSpline(kind, crossing);
  }
}

/**
 *  A kind's own fields across a release boundary.
 */
template <typename KindT>
void ConvertKind(KindT &kind, const Passage &passage, const Crossings &crossing)
{
  const Handle self = BaseOf(Object{kind}).Self;
  if constexpr (std::is_same_v<KindT, Entities::Insert>)
  {
    ConvertOwned(Ownership{.List = &kind.Owned, .First = &kind.First, .Last = &kind.Last, .Owner = self}, passage,
                 crossing);
  }
  else if constexpr (std::is_same_v<KindT, Entities::Polyline>)
  {
    ConvertOwned(Ownership{.List = &kind.Vertices, .First = &kind.First, .Last = &kind.Last, .Owner = self}, passage,
                 crossing);
  }
  else if constexpr (std::is_same_v<KindT, Entities::Hatch>)
  {
    if (crossing.R2004Down && kind.Gradient.Shown != 0)
    {
      Note(passage, self, std::format("gradient of {} not kept by R2000", passage.What));
    }
    if (crossing.R2004Down)
    {
      kind.Gradient = {};
    }
  }
  else if constexpr (std::is_same_v<KindT, Entities::Viewport>)
  {
    ConvertView(kind, passage, crossing);
    if (crossing.R2004Up)
    {
      kind.Header = {}; // R2004 keeps no viewport entity headers.
    }
    if (crossing.R2004Down && kind.ShadePlotMode != 0)
    {
      Note(passage, self, std::format("shade plot of {} not kept by R2000", passage.What));
    }
    if (crossing.R2004Down)
    {
      kind.ShadePlotMode = 0;
    }
  }
  else if constexpr (std::is_same_v<KindT, Objects::Layout>)
  {
    if (crossing.R2004Down)
    {
      LowerLayout(kind, passage);
    }
  }
}

/**
 *  What an image, a wipeout or an OLE frame drops when downgrading below the release that brought it: the inverted clip
 *  of R2010, noted when set; the frame's mode and trailing byte of R2000.
 */
template <typename KindT>
void ConvertFrame(KindT &kind, const Passage &passage, const Crossings &crossing)
{
  if constexpr (requires { kind.Raster.Outside; })
  {
    if (crossing.R2010Down && kind.Raster.Outside)
    {
      Note(passage, kind.Base.Self, std::format("inverted clip of {} not kept before R2010", passage.What));
    }
    if (crossing.R2010Down)
    {
      kind.Raster.Outside = false;
    }
  }
  else if constexpr (std::is_same_v<KindT, Entities::Ole2Frame>)
  {
    if (crossing.R2000Down)
    {
      kind.Mode = 0;
      kind.Unknown1 = 0;
    }
  }
}

void ConvertBlockRecord(Tables::BlockRecord &kind, const Passage &passage, const Crossings &crossing)
{
  if (crossing.R2000Down)
  {
    kind.Layout = {};
    kind.Inserts.clear();
    kind.Loaded = false;
    if (!kind.Description.empty())
    {
      Note(passage, kind.Record.Base.Self, std::format("description of {} not kept by R14", passage.What));
      kind.Description.clear();
    }
  }
  ConvertOwned(
      Ownership{.List = &kind.Entities, .First = &kind.First, .Last = &kind.Last, .Owner = kind.Record.Base.Self},
      passage, crossing);
}
} // namespace

auto ConvertObject(const DwgIO::Object &object, const Drawing &drawing, Version to) -> Translation
{
  if (to == drawing.Release)
  {
    return Translation{.Object = object, .Issues = {}};
  }
  return ConvertObject(object, Lookup{drawing}, to);
}

auto ConvertObject(const DwgIO::Object &object, const Lookup &found, Version to) -> Translation
{
  const Drawing &drawing = found.Drawing();
  Translation translation{.Object = object, .Issues = {}};
  if (to == drawing.Release)
  {
    return translation;
  }
  const Passage passage{.Drawing = &drawing,
                        .Lookup = &found,
                        .Issues = &translation.Issues,
                        .What = std::format("{} {}", TypeOf(object), HexOf(BaseOf(object).Self)),
                        .To = to};
  const Version from = drawing.Release;
  const Crossings crossing = CrossingsOf(from, to);
  Visit(
      [&](auto &kind) -> void
      {
        using KindT = std::remove_cvref_t<decltype(kind)>;
        if constexpr (std::is_same_v<KindT, Unknown>)
        {
          if (kind.Entity) // An untyped object's common data are read only for an entity.
          {
            ConvertCommon(kind.Common, kind.Base, passage);
          }
          ConvertUntyped(kind, drawing, to);
        }
        else if constexpr (requires { kind.Common; })
        {
          ConvertCommon(kind.Common, kind.Base, passage);
          ConvertKind(kind, passage, crossing);
          ConvertAnnotation(kind, passage, crossing);
          ConvertCurve(kind, crossing);
          ConvertFrame(kind, passage, crossing);
        }
        else if constexpr (std::is_same_v<KindT, Objects::Layout>)
        {
          ConvertKind(kind, passage, crossing);
        }
        else if constexpr (std::is_same_v<KindT, Tables::LType>)
        {
          ConvertLType(kind, passage, crossing);
        }
        else if constexpr (std::is_same_v<KindT, Objects::MLineStyle>)
        {
          ConvertMLineStyle(kind, passage, crossing);
        }
        else if constexpr (std::is_same_v<KindT, Objects::MLeaderStyle>)
        {
          ConvertVersion(VersionPlace{.Base = &kind.Base, .Version = &kind.Revision}, passage, crossing);
        }
        else if constexpr (std::is_same_v<KindT, Objects::TableStyle>)
        {
          ConvertTableStyle(kind, passage, crossing);
        }
        else if constexpr (std::is_same_v<KindT, Tables::Layer>)
        {
          if (crossing.R2000Down)
          {
            LowerLayer(kind, passage);
          }
        }
        else if constexpr (std::is_same_v<KindT, Tables::DimStyle>)
        {
          ConvertDimStyle(kind, passage, crossing);
        }
        else if constexpr (std::is_same_v<KindT, Tables::BlockRecord>)
        {
          ConvertBlockRecord(kind, passage, crossing);
        }
        else if constexpr (std::is_same_v<KindT, Objects::PlotSettings>)
        {
          ConvertPlotSettings(kind, passage, crossing);
        }
      },
      translation.Object);
  ConvertScene(translation.Object, Way{.Drawing = &drawing, .From = from, .To = to}, translation.Issues);
  ConvertAcis(translation, drawing, to);
  return translation;
}
} // namespace DwgIO::Releases
