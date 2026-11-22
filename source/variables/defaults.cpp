/***********************************************************************************************************************
 *  @file      source/variables/defaults.cpp
 *  @brief     The header variables of a new drawing, as AutoCAD's empty drawings have them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "encoding/guid.hpp"

#include "dwgio/color.hpp"
#include "dwgio/create.hpp"
#include "dwgio/header.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/version.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <format>
#include <random>
#include <string>

namespace DwgIO
{
namespace
{
/**
 *  The Julian day number of 1970-01-01, the day AutoCAD's dates count from the Julian epoch.
 */
constexpr std::int32_t UnixEpochDay = 2440588;
constexpr int Era = 1900;
constexpr int OlderUnknown = 256;
constexpr int OlderDragmode = 2;
constexpr int OlderOsmode = 37;
constexpr int OlderDimfit = 3;
constexpr int OlderDimunit = 2;
constexpr int MinutesPerHour = 60;
constexpr int SecondsPerMinute = 60;
constexpr int Thousand = 1000;

/**
 *  A GUID in the braces and hexadecimal AutoCAD writes, drawn fresh.
 */
auto Guid() -> std::string
{
  std::random_device device;
  std::mt19937_64 engine{device()};
  const std::uint64_t high = engine();
  const std::uint64_t low = engine();
  Encoding::Guid guid{.Data1 = static_cast<std::uint32_t>(high >> 32U),
                      .Data2 = static_cast<std::uint16_t>(high >> 16U),
                      .Data3 = static_cast<std::uint16_t>(high),
                      .Data4 = {}};
  for (std::size_t at = 0; at < guid.Data4.size(); ++at)
  {
    guid.Data4.at(at) = static_cast<std::uint8_t>(low >> (56U - (8U * at)));
  }
  return Encoding::FormatGuid(guid, true);
}

/**
 *  The local civil time of a moment, through the call each platform has.
 */
auto Local(std::time_t seconds) -> std::tm
{
  std::tm local{};
#ifdef _WIN32
  localtime_s(&local, &seconds);
#else
  localtime_r(&seconds, &local);
#endif
  return local;
}

/**
 *  The moment of the call as AutoCAD keeps a date: the Julian day in local time and the milliseconds into it.
 */
void SetDate(Header &header)
{
  using namespace std::chrono;
  const system_clock::time_point now = system_clock::now();
  const std::tm local = Local(system_clock::to_time_t(now));
  const sys_days day{year{local.tm_year + Era} / (local.tm_mon + 1) / local.tm_mday};
  const auto within = duration_cast<milliseconds>(now.time_since_epoch()).count() % Thousand;
  header.TdcreateDays = static_cast<std::int32_t>(day.time_since_epoch().count()) + UnixEpochDay;
  const std::int32_t sinceMidnight =
      (((local.tm_hour * MinutesPerHour) + local.tm_min) * SecondsPerMinute) + local.tm_sec;
  header.TdcreateMilliseconds = (sinceMidnight * Thousand) + static_cast<std::int32_t>(within);
  header.TdupdateDays = header.TdcreateDays;
  header.TdupdateMilliseconds = header.TdcreateMilliseconds;
}

/**
 *  What only a release before R2000 keeps, at the values of AutoCAD's empty R14 drawing.
 */
void Older(Header &header)
{
  header.Unknown11 = OlderUnknown;
  header.Attreq = true;
  header.Attdia = true;
  header.Delobj = true;
  header.Dragmode = OlderDragmode;
  header.Osmode = OlderOsmode;
  header.Coords = 1;
  header.Pickstyle = 1;
  header.Dimfit = OlderDimfit;
  header.Dimunit = OlderDimunit;
}

/**
 *  What only R2004 and later keep, at the values of AutoCAD's empty R2004 drawing; the unknown longs read as text
 *  ("0000", "M510") and are kept as the file has them.
 *
 *  The dictionaries of materials and colors stay null: a made drawing has none.
 */
void Newer(Header &header)
{
  header.Unknown16 = false;
  header.Unknown17 = 808464432;
  header.Unknown18 = 486869248;
  header.Unknown19 = 1295333680;
  header.Unknown20 = 1295003696;
  header.Unknown21 = 486539264;
  header.Unknown22 = 808464432;
  header.Sortents = 127;
  header.Indexctl = 0;
  header.Hidetext = 1;
  header.Xclipframe = 2;
  header.Dimassoc = 2;
  header.Halogap = 0;
  header.Obscuredcolor = 257;
  header.Intersectioncolor = 257;
  header.Obscuredltype = 0;
  header.Intersectiondisplay = 0;
  header.Projectname.clear();
}

/**
 *  What only R2007 and later keep, at the values of AutoCAD's empty R2010, R2013 and R2018 drawings, which agree.
 *
 *  The handles stay null: a made drawing has no materials or visual styles.
 */
void Latest(Header &header)
{
  constexpr double Jog = 0.78539816339744828;
  constexpr double ZeroFactor = 100; // DIMMZF and DIMALTMZF, the sub-units' factor.
  constexpr std::int32_t Unknown24 = 10;
  constexpr double Steps = 2;
  constexpr double Size = 6;
  constexpr double Lens = 50;
  constexpr double Width = 5;
  constexpr double Height = 80;
  constexpr double Square = 1.5707963267948966;
  constexpr int Parameter = 7;
  constexpr double Latitude = 37.795;
  constexpr double Longitude = -122.394;
  constexpr std::int32_t Zone = -8000;
  header.Requiredversions = 0;
  header.Dimfxl = 1;
  header.Dimjogang = Jog;
  header.Dimtfill = 0;
  header.Dimtfillclr = Color{.Index = 0};
  header.Dimarcsym = 0;
  header.Dimfxlon = false;
  header.Dimtxtdirection = false;
  header.Dimaltmzf = ZeroFactor;
  header.Dimaltmzs.clear();
  header.Dimmzf = ZeroFactor;
  header.Dimmzs.clear();
  header.Cameradisplay = false;
  header.Unknown23 = 0;
  header.Unknown24 = Unknown24;
  header.Unknown25 = 1;
  header.Stepspersec = Steps;
  header.Stepsize = Size;
  header.Dwfprec3d = 2;
  header.Lenslength = Lens;
  header.Cameraheight = 0;
  header.Solidhist = 0;
  header.Showhist = 1;
  header.Psolwidth = Width;
  header.Psolheight = Height;
  header.Loftang1 = Square;
  header.Loftang2 = Square;
  header.Loftmag1 = 0;
  header.Loftmag2 = 0;
  header.Loftparam = Parameter;
  header.Loftnormals = 1;
  header.Latitude = Latitude;
  header.Longitude = Longitude;
  header.Northdirection = 0;
  header.Timezone = Zone;
  header.Lightglyphdisplay = 1;
  header.Tilemodelightsynch = 1;
  header.Dwfframe = 2;
  header.Dgnframe = 0;
  header.Realworldscale = true;
  header.Interferecolor = Color{.Index = 1};
  header.Cshadow = 0;
  header.Shadowplanelocation = 0;
}
} // namespace

auto DefaultsOf(Version release) -> Header
{
  Header header;
  header.Unknown1 = 412148564080.0;
  header.Unknown2 = 1.0;
  header.Unknown3 = 1.0;
  header.Unknown4 = 1.0;
  header.Unknown5 = "m";
  header.Unknown6 = "";
  header.Unknown7 = "";
  header.Unknown8 = "";
  header.Unknown9 = 0;
  header.Unknown10 = 0;
  header.Unknown11 = 0;
  header.Dimaso = true;
  header.Dimsho = true;
  header.Dimsav = false;
  header.Plinegen = false;
  header.Orthomode = false;
  header.Regenmode = true;
  header.Fillmode = true;
  header.Qtextmode = false;
  header.Psltscale = true;
  header.Limcheck = false;
  header.Blipmode = false;
  header.Usrtimer = true;
  header.Skpoly = false;
  header.Angdir = false;
  header.Splframe = false;
  header.Attreq = false;
  header.Attdia = false;
  header.Mirrtext = false;
  header.Worldview = true;
  header.Wireframe = false;
  header.Tilemode = true;
  header.Plimcheck = false;
  header.Visretain = true;
  header.Delobj = false;
  header.Dispsilh = false;
  header.Pellipse = false;
  header.Proxygraphics = 1;
  header.Dragmode = 0;
  header.Treedepth = 3020;
  header.Lunits = 2;
  header.Luprec = 4;
  header.Aunits = 0;
  header.Auprec = 0;
  header.Osmode = 0;
  header.Attmode = 1;
  header.Coords = 0;
  header.Pdmode = 0;
  header.Pickstyle = 0;
  header.Useri1 = 0;
  header.Useri2 = 0;
  header.Useri3 = 0;
  header.Useri4 = 0;
  header.Useri5 = 0;
  header.Splinesegs = 8;
  header.Surfu = 6;
  header.Surfv = 6;
  header.Surftype = 6;
  header.Surftab1 = 6;
  header.Surftab2 = 6;
  header.Splinetype = 6;
  header.Shadedge = 3;
  header.Shadedif = 70;
  header.Unitmode = 0;
  header.Maxactvp = 64;
  header.Isolines = 4;
  header.Cmljust = 0;
  header.Textqlty = 50;
  header.Ltscale = 1.0;
  header.Textsize = 2.5;
  header.Tracewid = 1.0;
  header.Sketchinc = 1.0;
  header.Filletrad = 0.0;
  header.Thickness = 0.0;
  header.Angbase = 0.0;
  header.Pdsize = 0.0;
  header.Plinewid = 0.0;
  header.Userr1 = 0.0;
  header.Userr2 = 0.0;
  header.Userr3 = 0.0;
  header.Userr4 = 0.0;
  header.Userr5 = 0.0;
  header.Chamfera = 0.0;
  header.Chamferb = 0.0;
  header.Chamferc = 0.0;
  header.Chamferd = 0.0;
  header.Facetres = 0.5;
  header.Cmlscale = 20.0;
  header.Celtscale = 1.0;
  header.Menuname = ".";
  header.Cecolor = Color{.Index = 256, .Rgb = {}, .Transparency = {}};
  header.Psvpscale = 0.0;
  header.Pinsbase = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Pextmin = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Pextmax = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Plimmin = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Plimmax = Vector{.X = 12, .Y = 9, .Z = 0};
  header.Pelevation = 0.0;
  header.Pucsorg = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Pucsxdir = Vector{.X = 1, .Y = 0, .Z = 0};
  header.Pucsydir = Vector{.X = 0, .Y = 1, .Z = 0};
  header.Pucsorthoview = 0;
  header.Pucsorgtop = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Pucsorgbottom = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Pucsorgleft = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Pucsorgright = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Pucsorgfront = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Pucsorgback = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Insbase = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Extmin = Vector{.X = 1e+20, .Y = 1e+20, .Z = 1e+20};
  header.Extmax = Vector{.X = -1e+20, .Y = -1e+20, .Z = -1e+20};
  header.Limmin = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Limmax = Vector{.X = 420, .Y = 297, .Z = 0};
  header.Elevation = 0.0;
  header.Ucsorg = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Ucsxdir = Vector{.X = 1, .Y = 0, .Z = 0};
  header.Ucsydir = Vector{.X = 0, .Y = 1, .Z = 0};
  header.Ucsorthoview = 0;
  header.Ucsorgtop = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Ucsorgbottom = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Ucsorgleft = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Ucsorgright = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Ucsorgfront = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Ucsorgback = Vector{.X = 0, .Y = 0, .Z = 0};
  header.Dimpost = "";
  header.Dimapost = "";
  header.Dimtol = false;
  header.Dimlim = false;
  header.Dimtih = false;
  header.Dimtoh = false;
  header.Dimse1 = false;
  header.Dimse2 = false;
  header.Dimalt = false;
  header.Dimtofl = true;
  header.Dimsah = false;
  header.Dimtix = false;
  header.Dimsoxd = false;
  header.Dimaltd = 3;
  header.Dimzin = 8;
  header.Dimsd1 = false;
  header.Dimsd2 = false;
  header.Dimtolj = 0;
  header.Dimjust = 0;
  header.Dimfit = 0;
  header.Dimupt = false;
  header.Dimtzin = 8;
  header.Dimaltz = 0;
  header.Dimalttz = 0;
  header.Dimtad = 1;
  header.Dimunit = 0;
  header.Dimaunit = 0;
  header.Dimdec = 2;
  header.Dimtdec = 2;
  header.Dimaltu = 2;
  header.Dimalttd = 3;
  header.Dimscale = 1.0;
  header.Dimasz = 2.5;
  header.Dimexo = 0.625;
  header.Dimdli = 3.75;
  header.Dimexe = 1.25;
  header.Dimrnd = 0.0;
  header.Dimdle = 0.0;
  header.Dimtp = 0.0;
  header.Dimtm = 0.0;
  header.Dimazin = 0;
  header.Dimtxt = 2.5;
  header.Dimcen = 2.5;
  header.Dimtsz = 0.0;
  header.Dimaltf = 0.039370078740159999;
  header.Dimlfac = 1.0;
  header.Dimtvp = 0.0;
  header.Dimtfac = 1.0;
  header.Dimgap = 0.625;
  header.DimblkName = "";
  header.Dimblk1Name = "";
  header.Dimblk2Name = "";
  header.Dimaltrnd = 0.0;
  header.Dimclrd = Color{.Index = 0, .Rgb = {}, .Transparency = {}};
  header.Dimclre = Color{.Index = 0, .Rgb = {}, .Transparency = {}};
  header.Dimclrt = Color{.Index = 0, .Rgb = {}, .Transparency = {}};
  header.Dimadec = 0;
  header.Dimfrac = 0;
  header.Dimlunit = 2;
  header.Dimdsep = 44;
  header.Dimtmove = 0;
  header.Dimatfit = 3;
  header.Dimlwd = -2;
  header.Dimlwe = -2;
  header.Tstackalign = 1;
  header.Tstacksize = 70;
  header.Hyperlinkbase = "";
  header.Stylesheet = "";
  header.Flags = 10781;
  header.Insunits = 4;
  header.Cepsntype = 0;
  header.Unknown12 = -18379;
  header.Unknown13 = -7190;
  header.Unknown14 = 16444;
  header.Unknown15 = -5410;
  SetDate(header);
  header.Fingerprintguid = Guid();
  header.Versionguid = Guid();
  if (release < Version::R2000)
  {
    Older(header);
  }
  if (release >= Version::R2004)
  {
    Newer(header);
  }
  if (release >= Version::R2007)
  {
    Latest(header);
  }
  return header;
}
} // namespace DwgIO
