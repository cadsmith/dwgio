/***********************************************************************************************************************
 *  @file      source/releases/entities.cpp
 *  @brief     The entities whose fields another release holds differently.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/entities.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "releases/leader.hpp"
#include "releases/lookup.hpp"
#include "releases/mtext.hpp"
#include "releases/passage.hpp"
#include "releases/styles.hpp"
#include "releases/untyped.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/leader.hpp"
#include "dwgio/entities/mtext.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/entities/spline.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <cstdint>
#include <format>
#include <optional>
#include <utility>

namespace DwgIO::Releases
{
void ConvertSpline(Entities::Spline &spline, const Crossings &crossing)
{
  constexpr std::int32_t ByFits = 9;
  constexpr std::int32_t Custom = 15;
  if (crossing.R2013Up)
  {
    spline.Curve.Options = spline.Curve.FitTolerance ? ByFits : 0;
    spline.Curve.Parameter = spline.Curve.FitTolerance ? 0 : Custom;
  }
}

void ConvertAnnotation(Entities::MultiLeader &leader, const Passage &passage, const Crossings &crossing)
{
  ConvertVersion(VersionPlace{.Base = &leader.Base, .Version = &leader.MLeader.Version}, passage, crossing);
  if (crossing.R2010Up && !leader.MLeader.Arrowheads.empty())
  {
    Note(passage, leader.Base.Self, std::format("arrowheads of {} not kept by R2010", passage.What));
    leader.MLeader.Arrowheads.clear();
  }
}

void ConvertAnnotation(Entities::Leader &leader, const Passage &passage, const Crossings &crossing)
{
  if (crossing.R2000Up)
  {
    RaiseLeader(leader);
  }
  if (crossing.R2010Up)
  {
    leader.Height = 0;
    leader.Width = 0;
  }
  if (crossing.R2010Down)
  {
    BoxLeader(leader, *passage.Lookup);
  }
  if (crossing.R2000Down)
  {
    const auto *acad = passage.Lookup->Record<Tables::AppId>("ACAD");
    if (!LowerLeader(leader, *passage.Lookup, acad == nullptr ? Handle{} : acad->Record.Base.Self))
    {
      Note(passage, leader.Base.Self, std::format("application ACAD missing for the arrow of {}", passage.What));
    }
  }
}

void ConvertAnnotation(Entities::MText &text, const Passage &passage, [[maybe_unused]] const Crossings &crossing)
{
  const Version to = passage.To;
  const std::int32_t count = text.Annotative ? 0 : text.Columns.Count;
  const auto *acad = passage.Lookup->Record<Tables::AppId>("ACAD");
  const Version from = passage.Drawing->Release;
  if (!ConvertMText(text,
                    TextContext{.From = from, .To = to, .Acad = acad == nullptr ? Handle{} : acad->Record.Base.Self}))
  {
    Note(passage, text.Base.Self, std::format("application ACAD missing for {}", passage.What));
  }
  else if (from >= Version::R2018 && count > 1) // Only a downgrade can start at R2018.
  {
    Note(passage, text.Base.Self,
         std::format("{} columns of {} written as one MTEXT, where AutoCAD's save writes one per column", count,
                     passage.What));
  }
}

void ConvertUntyped(Unknown &unknown, const Drawing &drawing, Version to)
{
  const Encoding::Scheme scheme = Encoding::SchemeOf(drawing.Codepage);
  if (std::optional<Opaque> own = Carry(unknown, CarryContext{.From = drawing.Release, .To = to, .Text = &scheme}))
  {
    unknown.Body = std::move(own);
    unknown.Bytes.clear();
  }
}
} // namespace DwgIO::Releases
