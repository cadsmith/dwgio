/***********************************************************************************************************************
 *  @file      tests/dwgio/audit/rasters.cpp
 *  @brief     The audit of images and underlays whose definition is gone.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/acad.hpp"
#include "support/findings.hpp"
#include "support/fixtures.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/line.hpp"
#include "dwgio/entities/raster.hpp"
#include "dwgio/entities/underlay.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"

#include <doctest/doctest.h>

#include <format>
#include <string>
#include <variant>
#include <vector>

namespace DwgIO
{
namespace
{
constexpr Handle Nowhere{.Value = 0x999};

/**
 *  A copy of the first object of the kind in the drawing, without its handles.
 */
template <typename KindT>
auto First(const Drawing &drawing) -> KindT
{
  for (const Object &object : drawing.Objects)
  {
    if (const auto *kind = GetIf<KindT>(&object))
    {
      KindT copy = *kind;
      copy.Base = {}; // A copy to add, under a handle of its own.
      return copy;
    }
  }
  FAIL_CHECK("no such object");
  return KindT{};
}
} // namespace

TEST_CASE("audit/rasters: an image or underlay without its definition is dropped and a lost reactor cleared")
{
  Drawing rasters = Load(ReadFixture("rasters-r2018.dwg")).value().Drawing;
  CHECK(Audit::Check(rasters).Findings.empty());
  Entities::Image image = First<Entities::Image>(rasters);
  image.Raster.Definition = Nowhere;
  const Handle undefined = *Edit::Add(rasters, image);
  image = First<Entities::Image>(rasters);
  image.Raster.Reactor = *Edit::Add(rasters, Entities::Line{});
  const Handle unreacted = *Edit::Add(rasters, image);
  const Handle line = image.Raster.Reactor;
  CHECK(FindingTexts(Audit::Check(rasters)) ==
        std::vector<std::string>{std::format("image definition 999 of IMAGE {} missing, left out", HexOf(undefined)),
                                 std::format("image reactor {} of IMAGE {} missing", HexOf(line), HexOf(unreacted))});
  CHECK(Audit::Repair(rasters).IsClean());
  CHECK(Find(rasters, undefined) == nullptr);
  CHECK(Get<Entities::Image>(*Find(rasters, unreacted)).Raster.Reactor == Handle{});
  CHECK(Audit::Check(rasters).Findings.empty());

  Drawing kinds = Load(ReadFixture("kinds-r2018.dwg")).value().Drawing;
  CHECK(Audit::Check(kinds).Findings.empty());
  const Handle wrong = *Edit::Add(kinds, Entities::Line{});
  Entities::PdfUnderlay pdf = First<Entities::PdfUnderlay>(kinds);
  pdf.Data.Definition = First<Entities::DwfUnderlay>(kinds).Data.Definition;
  const Handle crossed = *Edit::Add(kinds, pdf);
  Entities::DwfUnderlay dwf = First<Entities::DwfUnderlay>(kinds);
  dwf.Data.Definition = wrong;
  const Handle lined = *Edit::Add(kinds, dwf);
  Entities::DgnUnderlay dgn = First<Entities::DgnUnderlay>(kinds);
  dgn.Data.Definition = wrong;
  const Handle drawn = *Edit::Add(kinds, dgn);
  CHECK(FindingTexts(Audit::Check(kinds)) ==
        std::vector<std::string>{
            std::format("PDF definition {} of PDFUNDERLAY {} missing, left out", HexOf(pdf.Data.Definition),
                        HexOf(crossed)),
            std::format("DWF definition {} of DWFUNDERLAY {} missing, left out", HexOf(wrong), HexOf(lined)),
            std::format("DGN definition {} of DGNUNDERLAY {} missing, left out", HexOf(wrong), HexOf(drawn))});
  CHECK(Audit::Repair(kinds).IsClean());
  CHECK(Find(kinds, crossed) == nullptr);
  CHECK(Find(kinds, lined) == nullptr);
  CHECK(Find(kinds, drawn) == nullptr);
}
} // namespace DwgIO
