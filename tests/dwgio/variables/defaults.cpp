/***********************************************************************************************************************
 *  @file      tests/dwgio/variables/defaults.cpp
 *  @brief     The header a drawing made from nothing takes, against the empty fixtures'.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "support/fixtures.hpp"

#include "dwgio/create.hpp"
#include "dwgio/header.hpp"
#include "dwgio/load.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <expected>

namespace DwgIO
{
TEST_CASE("variables/defaults: a new drawing's header matches the empty fixture's apart from dates and handles")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("empty-r2000.dwg"));
  REQUIRE(loaded);
  const Header &theirs = loaded->Drawing.Header;
  Header ours = DefaultsOf(Version::R2000);
  CHECK(ours.Ltscale == 1.0);
  CHECK(ours.Textsize == 2.5);
  CHECK(ours.Dimscale == 1.0);
  CHECK(ours.Handseed == Handle{});
  CHECK(ours.TdcreateDays >= theirs.TdcreateDays);
  CHECK(ours.TdupdateDays == ours.TdcreateDays);
  CHECK(ours.TdupdateMilliseconds == ours.TdcreateMilliseconds);
  CHECK(ours.Fingerprintguid.size() == theirs.Fingerprintguid.size());
  CHECK(ours.Fingerprintguid != ours.Versionguid);
  // What the fixture holds of its own moment, its identity and its handles, copied over before the comparison.
  ours.TdcreateDays = theirs.TdcreateDays;
  ours.TdcreateMilliseconds = theirs.TdcreateMilliseconds;
  ours.TdupdateDays = theirs.TdupdateDays;
  ours.TdupdateMilliseconds = theirs.TdupdateMilliseconds;
  ours.TdindwgDays = theirs.TdindwgDays;
  ours.TdindwgMilliseconds = theirs.TdindwgMilliseconds;
  ours.TdusrtimerDays = theirs.TdusrtimerDays;
  ours.TdusrtimerMilliseconds = theirs.TdusrtimerMilliseconds;
  ours.Fingerprintguid = theirs.Fingerprintguid;
  ours.Versionguid = theirs.Versionguid;
  ours.Handseed = theirs.Handseed;
  ours.Clayer = theirs.Clayer;
  ours.Textstyle = theirs.Textstyle;
  ours.Celtype = theirs.Celtype;
  ours.Dimstyle = theirs.Dimstyle;
  ours.Cmlstyle = theirs.Cmlstyle;
  ours.Dimtxsty = theirs.Dimtxsty;
  ours.BlockControlObject = theirs.BlockControlObject;
  ours.LayerControlObject = theirs.LayerControlObject;
  ours.StyleControlObject = theirs.StyleControlObject;
  ours.LTypeControlObject = theirs.LTypeControlObject;
  ours.ViewControlObject = theirs.ViewControlObject;
  ours.UcsControlObject = theirs.UcsControlObject;
  ours.VPortControlObject = theirs.VPortControlObject;
  ours.AppIdControlObject = theirs.AppIdControlObject;
  ours.DimStyleControlObject = theirs.DimStyleControlObject;
  ours.ViewportEntityHeaderControlObject = theirs.ViewportEntityHeaderControlObject;
  ours.DictionaryAcadGroup = theirs.DictionaryAcadGroup;
  ours.DictionaryAcadMLineStyle = theirs.DictionaryAcadMLineStyle;
  ours.DictionaryNamedObjects = theirs.DictionaryNamedObjects;
  ours.DictionaryLayouts = theirs.DictionaryLayouts;
  ours.DictionaryPlotSettings = theirs.DictionaryPlotSettings;
  ours.DictionaryPlotStyles = theirs.DictionaryPlotStyles;
  ours.Cpsnid = theirs.Cpsnid;
  ours.BlockRecordPaperSpace = theirs.BlockRecordPaperSpace;
  ours.BlockRecordModelSpace = theirs.BlockRecordModelSpace;
  ours.LTypeByLayer = theirs.LTypeByLayer;
  ours.LTypeByBlock = theirs.LTypeByBlock;
  ours.LTypeContinuous = theirs.LTypeContinuous;
  CHECK(ours == theirs);

  // The R14 defaults differ in what only R14 keeps, at the values of the empty R14 fixture.
  const std::expected<Loaded, Failure> older = Load(ReadFixture("empty-r14.dwg"));
  REQUIRE(older);
  const Header &olders = older->Drawing.Header;
  Header mine = DefaultsOf(Version::R14);
  CHECK(mine.Attreq == olders.Attreq);
  CHECK(mine.Attdia == olders.Attdia);
  CHECK(mine.Unknown11 == olders.Unknown11);
  CHECK(mine.Delobj == olders.Delobj);
  CHECK(mine.Dragmode == olders.Dragmode);
  CHECK(mine.Osmode == olders.Osmode);
  CHECK(mine.Coords == olders.Coords);
  CHECK(mine.Pickstyle == olders.Pickstyle);
  CHECK(mine.Dimfit == olders.Dimfit);
  CHECK(mine.Dimunit == olders.Dimunit);
  mine.Attreq = false;
  mine.Attdia = false;
  mine.Unknown11 = 0;
  mine.Delobj = false;
  mine.Dragmode = 0;
  mine.Osmode = 0;
  mine.Coords = 0;
  mine.Pickstyle = 0;
  mine.Dimfit = 0;
  mine.Dimunit = 0;
  Header plain = DefaultsOf(Version::R2000);
  for (Header *each : {&mine, &plain})
  {
    each->TdcreateDays = 0;
    each->TdcreateMilliseconds = 0;
    each->TdupdateDays = 0;
    each->TdupdateMilliseconds = 0;
    each->Fingerprintguid.clear();
    each->Versionguid.clear();
  }
  CHECK(mine == plain);
}

TEST_CASE("variables/defaults: the R2004 members match the empty R2004 fixture's apart from the dictionaries")
{
  const std::expected<Loaded, Failure> loaded = Load(ReadFixture("empty-r2004.dwg"));
  REQUIRE(loaded);
  const Header &theirs = loaded->Drawing.Header;
  const Header ours = DefaultsOf(Version::R2004);
  CHECK(ours.Dimassoc == 2);
  CHECK(ours.Sortents == theirs.Sortents);
  CHECK(ours.Hidetext == theirs.Hidetext);
  CHECK(ours.Xclipframe == theirs.Xclipframe);
  CHECK(ours.Obscuredcolor == theirs.Obscuredcolor);
  CHECK(ours.Intersectioncolor == theirs.Intersectioncolor);
  CHECK(ours.Unknown17 == theirs.Unknown17);
  CHECK(ours.Unknown22 == theirs.Unknown22);
  CHECK(ours.Projectname.empty());
  CHECK(ours.DictionaryMaterials == Handle{});
  CHECK(ours.DictionaryColors == Handle{});
  CHECK(DefaultsOf(Version::R2000).Dimassoc == 0);
  CHECK(DefaultsOf(Version::R2000).Sortents == 0);
}
} // namespace DwgIO
