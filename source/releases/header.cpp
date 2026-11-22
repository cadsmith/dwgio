/***********************************************************************************************************************
 *  @file      source/releases/header.cpp
 *  @brief     The header as another release holds it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/header.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "fields/context.hpp"
#include "releases/dimstyle.hpp"
#include "releases/lookup.hpp"
#include "releases/omission.hpp"
#include "releases/passage.hpp"
#include "variables/layout.hpp"

#include "dwgio/base.hpp"
#include "dwgio/create.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <format>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  What only R2000 keeps, from a header: the defaults when upgrading to R2000, an empty header when downgrading to R14,
 *  which keeps none of it; the handles to the arrow blocks and the dictionaries are set by the caller.
 */
void SetR2000Variables(Header &header, const Header &fresh)
{
  header.Psvpscale = fresh.Psvpscale;
  header.Pucsorthoref = fresh.Pucsorthoref;
  header.Pucsorthoview = fresh.Pucsorthoview;
  header.Pucsbase = fresh.Pucsbase;
  header.Pucsorgtop = fresh.Pucsorgtop;
  header.Pucsorgbottom = fresh.Pucsorgbottom;
  header.Pucsorgleft = fresh.Pucsorgleft;
  header.Pucsorgright = fresh.Pucsorgright;
  header.Pucsorgfront = fresh.Pucsorgfront;
  header.Pucsorgback = fresh.Pucsorgback;
  header.Ucsorthoref = fresh.Ucsorthoref;
  header.Ucsorthoview = fresh.Ucsorthoview;
  header.Ucsbase = fresh.Ucsbase;
  header.Ucsorgtop = fresh.Ucsorgtop;
  header.Ucsorgbottom = fresh.Ucsorgbottom;
  header.Ucsorgleft = fresh.Ucsorgleft;
  header.Ucsorgright = fresh.Ucsorgright;
  header.Ucsorgfront = fresh.Ucsorgfront;
  header.Ucsorgback = fresh.Ucsorgback;
  header.Dimazin = fresh.Dimazin;
  header.Dimaltrnd = fresh.Dimaltrnd;
  header.Dimadec = fresh.Dimadec;
  header.Dimfrac = fresh.Dimfrac;
  header.Dimlunit = fresh.Dimlunit;
  header.Dimdsep = fresh.Dimdsep;
  header.Dimtmove = fresh.Dimtmove;
  header.Dimatfit = fresh.Dimatfit;
  header.Dimlwd = fresh.Dimlwd;
  header.Dimlwe = fresh.Dimlwe;
  header.Tstackalign = fresh.Tstackalign;
  header.Tstacksize = fresh.Tstacksize;
  header.Hyperlinkbase = fresh.Hyperlinkbase;
  header.Stylesheet = fresh.Stylesheet;
  header.Flags = fresh.Flags;
  header.Insunits = fresh.Insunits;
  header.Cepsntype = fresh.Cepsntype;
  header.Cpsnid = fresh.Cpsnid;
  header.Fingerprintguid = fresh.Fingerprintguid;
  header.Versionguid = fresh.Versionguid;
}

/**
 *  What only R14 keeps, set to the defaults.
 */
void SetR14Variables(Header &header, const Header &fresh)
{
  header.Unknown11 = fresh.Unknown11;
  header.Dimsav = fresh.Dimsav;
  header.Blipmode = fresh.Blipmode;
  header.Attreq = fresh.Attreq;
  header.Attdia = fresh.Attdia;
  header.Wireframe = fresh.Wireframe;
  header.Delobj = fresh.Delobj;
  header.Dragmode = fresh.Dragmode;
  header.Osmode = fresh.Osmode;
  header.Coords = fresh.Coords;
  header.Pickstyle = fresh.Pickstyle;
  header.Dimfit = fresh.Dimfit;
  header.Dimunit = fresh.Dimunit;
}

/**
 *  The header's pointers of R2007 and later cleared where they name an object left out, each noted: the current
 *  material and the visual styles of dragging and interference.
 */
void ClearHeaderPointers(Header &header, std::span<const Handle> leftOut, std::vector<Issue> &issues)
{
  for (const auto &[pointer, name] :
       {std::pair{&header.Cmaterial, "CMATERIAL"}, std::pair{&header.Dragvs, "DRAGVS"},
        std::pair{&header.Interfereobjvs, "INTERFEREOBJVS"}, std::pair{&header.Interferevpvs, "INTERFEREVPVS"}})
  {
    const Handle was = *pointer;
    if (ClearLeftOut(*pointer, leftOut))
    {
      issues.push_back(Issue{.Code = IssueCode::NotKept,
                             .What = std::format("{} of the header cleared, {} left out", name, HexOf(was))});
    }
  }
}

/**
 *  Null when the root has no dictionary of that name.
 */
auto RootEntry(const Drawing &drawing, std::string_view name) -> Handle
{
  const Objects::Dictionary *dictionary = View::Dictionary(drawing, name);
  return dictionary == nullptr ? Handle{} : dictionary->Base.Self;
}

/**
 *  What only R2004 keeps, from a header: the defaults when upgrading to R2004, an empty header when downgrading, which
 *  silently drops them as the older layout has no place for them.
 */
void SetR2004Variables(Header &header, const Header &fresh)
{
  header.Unknown16 = fresh.Unknown16;
  header.Unknown17 = fresh.Unknown17;
  header.Unknown18 = fresh.Unknown18;
  header.Unknown19 = fresh.Unknown19;
  header.Unknown20 = fresh.Unknown20;
  header.Unknown21 = fresh.Unknown21;
  header.Unknown22 = fresh.Unknown22;
  header.Sortents = fresh.Sortents;
  header.Indexctl = fresh.Indexctl;
  header.Hidetext = fresh.Hidetext;
  header.Xclipframe = fresh.Xclipframe;
  header.Dimassoc = fresh.Dimassoc;
  header.Halogap = fresh.Halogap;
  header.Obscuredcolor = fresh.Obscuredcolor;
  header.Intersectioncolor = fresh.Intersectioncolor;
  header.Obscuredltype = fresh.Obscuredltype;
  header.Intersectiondisplay = fresh.Intersectiondisplay;
  header.Projectname = fresh.Projectname;
}

/**
 *  What only R2007 and later keep, from a header: the defaults when upgrading to R2007, an empty header when
 *  downgrading, which silently drops them as the older layout has no place for them.
 */
void SetR2007Variables(Header &header, const Header &fresh)
{
  header.Cmaterial = fresh.Cmaterial;
  header.Dimfxl = fresh.Dimfxl;
  header.Dimjogang = fresh.Dimjogang;
  header.Dimtfill = fresh.Dimtfill;
  header.Dimtfillclr = fresh.Dimtfillclr;
  header.Dimarcsym = fresh.Dimarcsym;
  header.Dimfxlon = fresh.Dimfxlon;
  header.Dimltype = fresh.Dimltype;
  header.Dimltex1 = fresh.Dimltex1;
  header.Dimltex2 = fresh.Dimltex2;
  header.DictionaryVisualStyles = fresh.DictionaryVisualStyles;
  header.Cameradisplay = fresh.Cameradisplay;
  header.Unknown23 = fresh.Unknown23;
  header.Unknown24 = fresh.Unknown24;
  header.Unknown25 = fresh.Unknown25;
  header.Stepspersec = fresh.Stepspersec;
  header.Stepsize = fresh.Stepsize;
  header.Dwfprec3d = fresh.Dwfprec3d;
  header.Lenslength = fresh.Lenslength;
  header.Cameraheight = fresh.Cameraheight;
  header.Solidhist = fresh.Solidhist;
  header.Showhist = fresh.Showhist;
  header.Psolwidth = fresh.Psolwidth;
  header.Psolheight = fresh.Psolheight;
  header.Loftang1 = fresh.Loftang1;
  header.Loftang2 = fresh.Loftang2;
  header.Loftmag1 = fresh.Loftmag1;
  header.Loftmag2 = fresh.Loftmag2;
  header.Loftparam = fresh.Loftparam;
  header.Loftnormals = fresh.Loftnormals;
  header.Latitude = fresh.Latitude;
  header.Longitude = fresh.Longitude;
  header.Northdirection = fresh.Northdirection;
  header.Timezone = fresh.Timezone;
  header.Lightglyphdisplay = fresh.Lightglyphdisplay;
  header.Tilemodelightsynch = fresh.Tilemodelightsynch;
  header.Dwfframe = fresh.Dwfframe;
  header.Dgnframe = fresh.Dgnframe;
  header.Realworldscale = fresh.Realworldscale;
  header.Interferecolor = fresh.Interferecolor;
  header.Interfereobjvs = fresh.Interfereobjvs;
  header.Interferevpvs = fresh.Interferevpvs;
  header.Dragvs = fresh.Dragvs;
  header.Cshadow = fresh.Cshadow;
  header.Shadowplanelocation = fresh.Shadowplanelocation;
}

/**
 *  What only R2010 and later keep, from a header: the defaults when upgrading to R2010, an empty header when
 *  downgrading.
 */
void SetR2010Variables(Header &header, const Header &fresh)
{
  header.Dimtxtdirection = fresh.Dimtxtdirection;
  header.Dimaltmzf = fresh.Dimaltmzf;
  header.Dimaltmzs = fresh.Dimaltmzs;
  header.Dimmzf = fresh.Dimmzf;
  header.Dimmzs = fresh.Dimmzs;
}

/**
 *  What only R2013 and later keep, from a header: the defaults when upgrading to R2013, an empty header when
 *  downgrading.
 */
void SetR2013Variables(Header &header, const Header &fresh)
{
  header.Requiredversions = fresh.Requiredversions;
  header.DictionaryUnknown = fresh.DictionaryUnknown;
}

/**
 *  The header as R2000 holds it, from R14's terms.
 */
void RaiseHeader(Header &made, const Header &header, const Passage &passage)
{
  const Drawing &drawing = *passage.Drawing;
  SetR2000Variables(made, DefaultsOf(Version::R2000));
  const Units units = SplitUnits(header.Dimunit);
  const Fit fit = SplitFit(header.Dimfit);
  made.Dimlunit = units.Lunit;
  made.Dimfrac = units.Frac;
  made.Dimatfit = fit.Atfit;
  made.Dimtmove = fit.Tmove;
  made.Dimblk = ArrowRecord(header.DimblkName, passage, {});
  made.Dimblk1 = ArrowRecord(header.Dimblk1Name, passage, {});
  made.Dimblk2 = ArrowRecord(header.Dimblk2Name, passage, {});
  made.Dimldrblk = {};
  made.DictionaryLayouts = RootEntry(drawing, "ACAD_LAYOUT");
  made.DictionaryPlotSettings = RootEntry(drawing, "ACAD_PLOTSETTINGS");
  made.DictionaryPlotStyles = RootEntry(drawing, "ACAD_PLOTSTYLENAME");
}

/**
 *  The header as R14 holds it, from R2000's terms: what R14 does not keep is not held, as the model mirrors the file.
 */
void LowerHeader(Header &made, const Header &header, const Passage &passage)
{
  const Drawing &drawing = *passage.Drawing;
  SetR14Variables(made, DefaultsOf(Version::R14));
  made.Dimunit = JoinUnits(Units{.Lunit = header.Dimlunit, .Frac = header.Dimfrac});
  made.Dimfit = JoinFit(Fit{.Atfit = header.Dimatfit, .Tmove = header.Dimtmove});
  made.DimblkName = ArrowName(drawing, header.Dimblk);
  made.Dimblk1Name = ArrowName(drawing, header.Dimblk1);
  made.Dimblk2Name = ArrowName(drawing, header.Dimblk2);
  SetR2000Variables(made, Header{});
  made.Dimblk = {};
  made.Dimblk1 = {};
  made.Dimblk2 = {};
  made.Dimldrblk = {};
  made.DictionaryLayouts = {};
  made.DictionaryPlotSettings = {};
  made.DictionaryPlotStyles = {};
}
} // namespace

auto ConvertHeader(const DwgIO::Header &header, const Drawing &drawing, Version to) -> HeaderTranslation
{
  HeaderTranslation translation{.Header = header, .Issues = {}};
  if (to == drawing.Release)
  {
    return translation;
  }
  Header &made = translation.Header;
  const Lookup found{drawing};
  const Passage passage{
      .Drawing = &drawing, .Lookup = &found, .Issues = &translation.Issues, .What = "header", .To = to};
  const Version from = drawing.Release;
  if (CrossesUp(from, to, Version::R2000))
  {
    RaiseHeader(made, header, passage);
  }
  if (CrossesDown(from, to, Version::R2000))
  {
    LowerHeader(made, header, passage);
  }
  if (CrossesUp(from, to, Version::R2004))
  {
    SetR2004Variables(made, DefaultsOf(to));
    made.DictionaryMaterials = RootEntry(drawing, "ACAD_MATERIALS");
    made.DictionaryColors = RootEntry(drawing, "ACAD_COLOR");
    made.ViewportEntityHeaderControlObject = {}; // R2004 keeps no viewport entity headers, nor their table.
  }
  if (CrossesDown(from, to, Version::R2004))
  {
    SetR2004Variables(made, Header{});
    made.DictionaryMaterials = {};
    made.DictionaryColors = {};
  }
  if (CrossesUp(from, to, Version::R2007))
  {
    SetR2007Variables(made, DefaultsOf(to));
    made.DictionaryVisualStyles = RootEntry(drawing, "ACAD_VISUALSTYLE");
    made.Unknown12 = 0; // The four shorts R14 to R2004 end the header with, which R2007 leaves out.
    made.Unknown13 = 0;
    made.Unknown14 = 0;
    made.Unknown15 = 0;
  }
  if (CrossesDown(from, to, Version::R2007))
  {
    // The four shorts R14 to R2004 end the header with, which R2007 and later leave out, are AutoCAD's.
    const Header fresh = DefaultsOf(to);
    SetR2007Variables(made, Header{});
    made.Unknown12 = fresh.Unknown12;
    made.Unknown13 = fresh.Unknown13;
    made.Unknown14 = fresh.Unknown14;
    made.Unknown15 = fresh.Unknown15;
  }
  if (CrossesUp(from, to, Version::R2010))
  {
    SetR2010Variables(made, DefaultsOf(to));
  }
  if (CrossesDown(from, to, Version::R2010))
  {
    SetR2010Variables(made, Header{});
  }
  if (CrossesUp(from, to, Version::R2013))
  {
    SetR2013Variables(made, DefaultsOf(to));
  }
  if (CrossesDown(from, to, Version::R2013))
  {
    SetR2013Variables(made, Header{});
  }
  ClearHeaderPointers(made, CollectLeftOut(drawing, to), translation.Issues);
  // A file keeps the bits that fill the header's last byte as padding; they come from writing it, so a header loads
  // back equal to the one saved.
  const Encoding::Scheme scheme = Encoding::SchemeOf(drawing.Codepage);
  std::vector<Issue> scratch;
  const Fields::Context with{.Release = to, .Text = &scheme, .Issues = &scratch};
  made.Padding.clear();
  made.Padding = Variables::Read(Variables::Write(made, with), with).Padding;
  return translation;
}
} // namespace DwgIO::Releases
