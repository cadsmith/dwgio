/***********************************************************************************************************************
 *  @file      source/variables/layout.hpp
 *  @brief     The header variables of chapter 9, in the file's order.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_VARIABLES_LAYOUT_HPP
#define DWGIO_VARIABLES_LAYOUT_HPP

#include "fields/codes.hpp"
#include "fields/context.hpp"
#include "fields/layout.hpp"
#include "fields/since.hpp"
#include "fields/when.hpp"

#include "dwgio/header.hpp"
#include "dwgio/version.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace DwgIO::Variables
{
/**
 *  Whether the header stores $CEPSNID: only when $CEPSNTYPE is 3.
 */
inline auto HasCpsnid(const DwgIO::Header &header) -> bool
{
  return header.Cepsntype == 3;
}

/**
 *  The items of chapter 9 for R13 to R2018, in the specification's order, with the ones a release lacks skipped.
 */
inline auto Layout()
{
  using namespace Fields;
  return Fields::Layout(
      Since(Version::R2013, LongLong(&DwgIO::Header::Requiredversions)), Double(&DwgIO::Header::Unknown1),
      Double(&DwgIO::Header::Unknown2), Double(&DwgIO::Header::Unknown3), Double(&DwgIO::Header::Unknown4),
      Text(&DwgIO::Header::Unknown5), Text(&DwgIO::Header::Unknown6), Text(&DwgIO::Header::Unknown7),
      Text(&DwgIO::Header::Unknown8), Long(&DwgIO::Header::Unknown9), Long(&DwgIO::Header::Unknown10),
      Until(Version::R14, Short(&DwgIO::Header::Unknown11)),
      Until(Version::R2000, StreamHandle(&DwgIO::Header::CurrentViewportEntityHeader, 5)), Bit(&DwgIO::Header::Dimaso),
      Bit(&DwgIO::Header::Dimsho), Until(Version::R14, Bit(&DwgIO::Header::Dimsav)), Bit(&DwgIO::Header::Plinegen),
      Bit(&DwgIO::Header::Orthomode), Bit(&DwgIO::Header::Regenmode), Bit(&DwgIO::Header::Fillmode),
      Bit(&DwgIO::Header::Qtextmode), Bit(&DwgIO::Header::Psltscale), Bit(&DwgIO::Header::Limcheck),
      Until(Version::R14, Bit(&DwgIO::Header::Blipmode)), Since(Version::R2004, Bit(&DwgIO::Header::Unknown16)),
      Bit(&DwgIO::Header::Usrtimer), Bit(&DwgIO::Header::Skpoly), Bit(&DwgIO::Header::Angdir),
      Bit(&DwgIO::Header::Splframe), Until(Version::R14, Bit(&DwgIO::Header::Attreq), Bit(&DwgIO::Header::Attdia)),
      Bit(&DwgIO::Header::Mirrtext), Bit(&DwgIO::Header::Worldview),
      Until(Version::R14, Bit(&DwgIO::Header::Wireframe)), Bit(&DwgIO::Header::Tilemode),
      Bit(&DwgIO::Header::Plimcheck), Bit(&DwgIO::Header::Visretain), Until(Version::R14, Bit(&DwgIO::Header::Delobj)),
      Bit(&DwgIO::Header::Dispsilh), Bit(&DwgIO::Header::Pellipse), Short(&DwgIO::Header::Proxygraphics),
      Until(Version::R14, Short(&DwgIO::Header::Dragmode)), Short(&DwgIO::Header::Treedepth),
      Short(&DwgIO::Header::Lunits), Short(&DwgIO::Header::Luprec), Short(&DwgIO::Header::Aunits),
      Short(&DwgIO::Header::Auprec), Until(Version::R14, Short(&DwgIO::Header::Osmode)), Short(&DwgIO::Header::Attmode),
      Until(Version::R14, Short(&DwgIO::Header::Coords)), Short(&DwgIO::Header::Pdmode),
      Until(Version::R14, Short(&DwgIO::Header::Pickstyle)),
      Since(Version::R2004, Long(&DwgIO::Header::Unknown17), Long(&DwgIO::Header::Unknown18),
            Long(&DwgIO::Header::Unknown19)),
      Short(&DwgIO::Header::Useri1), Short(&DwgIO::Header::Useri2), Short(&DwgIO::Header::Useri3),
      Short(&DwgIO::Header::Useri4), Short(&DwgIO::Header::Useri5), Short(&DwgIO::Header::Splinesegs),
      Short(&DwgIO::Header::Surfu), Short(&DwgIO::Header::Surfv), Short(&DwgIO::Header::Surftype),
      Short(&DwgIO::Header::Surftab1), Short(&DwgIO::Header::Surftab2), Short(&DwgIO::Header::Splinetype),
      Short(&DwgIO::Header::Shadedge), Short(&DwgIO::Header::Shadedif), Short(&DwgIO::Header::Unitmode),
      Short(&DwgIO::Header::Maxactvp), Short(&DwgIO::Header::Isolines), Short(&DwgIO::Header::Cmljust),
      Short(&DwgIO::Header::Textqlty), Double(&DwgIO::Header::Ltscale), Double(&DwgIO::Header::Textsize),
      Double(&DwgIO::Header::Tracewid), Double(&DwgIO::Header::Sketchinc), Double(&DwgIO::Header::Filletrad),
      Double(&DwgIO::Header::Thickness), Double(&DwgIO::Header::Angbase), Double(&DwgIO::Header::Pdsize),
      Double(&DwgIO::Header::Plinewid), Double(&DwgIO::Header::Userr1), Double(&DwgIO::Header::Userr2),
      Double(&DwgIO::Header::Userr3), Double(&DwgIO::Header::Userr4), Double(&DwgIO::Header::Userr5),
      Double(&DwgIO::Header::Chamfera), Double(&DwgIO::Header::Chamferb), Double(&DwgIO::Header::Chamferc),
      Double(&DwgIO::Header::Chamferd), Double(&DwgIO::Header::Facetres), Double(&DwgIO::Header::Cmlscale),
      Double(&DwgIO::Header::Celtscale), Text(&DwgIO::Header::Menuname), Long(&DwgIO::Header::TdcreateDays),
      Long(&DwgIO::Header::TdcreateMilliseconds), Long(&DwgIO::Header::TdupdateDays),
      Long(&DwgIO::Header::TdupdateMilliseconds),
      Since(Version::R2004, Long(&DwgIO::Header::Unknown20), Long(&DwgIO::Header::Unknown21),
            Long(&DwgIO::Header::Unknown22)),
      Long(&DwgIO::Header::TdindwgDays), Long(&DwgIO::Header::TdindwgMilliseconds),
      Long(&DwgIO::Header::TdusrtimerDays), Long(&DwgIO::Header::TdusrtimerMilliseconds),
      CmColor(&DwgIO::Header::Cecolor), DataHandle(&DwgIO::Header::Handseed, 0),
      StreamHandle(&DwgIO::Header::Clayer, 5), StreamHandle(&DwgIO::Header::Textstyle, 5),
      StreamHandle(&DwgIO::Header::Celtype, 5), Since(Version::R2007, StreamHandle(&DwgIO::Header::Cmaterial, 5)),
      StreamHandle(&DwgIO::Header::Dimstyle, 5), StreamHandle(&DwgIO::Header::Cmlstyle, 5),
      Since(Version::R2000, Double(&DwgIO::Header::Psvpscale)), Point3(&DwgIO::Header::Pinsbase),
      Point3(&DwgIO::Header::Pextmin), Point3(&DwgIO::Header::Pextmax), RawPoint2(&DwgIO::Header::Plimmin),
      RawPoint2(&DwgIO::Header::Plimmax), Double(&DwgIO::Header::Pelevation), Point3(&DwgIO::Header::Pucsorg),
      Point3(&DwgIO::Header::Pucsxdir), Point3(&DwgIO::Header::Pucsydir), StreamHandle(&DwgIO::Header::Pucsname, 5),
      Since(Version::R2000, StreamHandle(&DwgIO::Header::Pucsorthoref, 5), Short(&DwgIO::Header::Pucsorthoview),
            StreamHandle(&DwgIO::Header::Pucsbase, 5), Point3(&DwgIO::Header::Pucsorgtop),
            Point3(&DwgIO::Header::Pucsorgbottom), Point3(&DwgIO::Header::Pucsorgleft),
            Point3(&DwgIO::Header::Pucsorgright), Point3(&DwgIO::Header::Pucsorgfront),
            Point3(&DwgIO::Header::Pucsorgback)),
      Point3(&DwgIO::Header::Insbase), Point3(&DwgIO::Header::Extmin), Point3(&DwgIO::Header::Extmax),
      RawPoint2(&DwgIO::Header::Limmin), RawPoint2(&DwgIO::Header::Limmax), Double(&DwgIO::Header::Elevation),
      Point3(&DwgIO::Header::Ucsorg), Point3(&DwgIO::Header::Ucsxdir), Point3(&DwgIO::Header::Ucsydir),
      StreamHandle(&DwgIO::Header::Ucsname, 5),
      Since(Version::R2000, StreamHandle(&DwgIO::Header::Ucsorthoref, 5), Short(&DwgIO::Header::Ucsorthoview),
            StreamHandle(&DwgIO::Header::Ucsbase, 5), Point3(&DwgIO::Header::Ucsorgtop),
            Point3(&DwgIO::Header::Ucsorgbottom), Point3(&DwgIO::Header::Ucsorgleft),
            Point3(&DwgIO::Header::Ucsorgright), Point3(&DwgIO::Header::Ucsorgfront),
            Point3(&DwgIO::Header::Ucsorgback), Text(&DwgIO::Header::Dimpost), Text(&DwgIO::Header::Dimapost)),
      Until(Version::R14, Bit(&DwgIO::Header::Dimtol), Bit(&DwgIO::Header::Dimlim), Bit(&DwgIO::Header::Dimtih),
            Bit(&DwgIO::Header::Dimtoh), Bit(&DwgIO::Header::Dimse1), Bit(&DwgIO::Header::Dimse2),
            Bit(&DwgIO::Header::Dimalt), Bit(&DwgIO::Header::Dimtofl), Bit(&DwgIO::Header::Dimsah),
            Bit(&DwgIO::Header::Dimtix), Bit(&DwgIO::Header::Dimsoxd), Byte(&DwgIO::Header::Dimaltd),
            Byte(&DwgIO::Header::Dimzin), Bit(&DwgIO::Header::Dimsd1), Bit(&DwgIO::Header::Dimsd2),
            Byte(&DwgIO::Header::Dimtolj), Byte(&DwgIO::Header::Dimjust), Byte(&DwgIO::Header::Dimfit),
            Bit(&DwgIO::Header::Dimupt), Byte(&DwgIO::Header::Dimtzin), Byte(&DwgIO::Header::Dimaltz),
            Byte(&DwgIO::Header::Dimalttz), Byte(&DwgIO::Header::Dimtad), Short(&DwgIO::Header::Dimunit),
            Short(&DwgIO::Header::Dimaunit), Short(&DwgIO::Header::Dimdec), Short(&DwgIO::Header::Dimtdec),
            Short(&DwgIO::Header::Dimaltu), Short(&DwgIO::Header::Dimalttd), StreamHandle(&DwgIO::Header::Dimtxsty, 5)),
      Double(&DwgIO::Header::Dimscale), Double(&DwgIO::Header::Dimasz), Double(&DwgIO::Header::Dimexo),
      Double(&DwgIO::Header::Dimdli), Double(&DwgIO::Header::Dimexe), Double(&DwgIO::Header::Dimrnd),
      Double(&DwgIO::Header::Dimdle), Double(&DwgIO::Header::Dimtp), Double(&DwgIO::Header::Dimtm),
      Since(Version::R2007, Double(&DwgIO::Header::Dimfxl), Double(&DwgIO::Header::Dimjogang),
            Short(&DwgIO::Header::Dimtfill), CmColor(&DwgIO::Header::Dimtfillclr)),
      Since(Version::R2000, Bit(&DwgIO::Header::Dimtol), Bit(&DwgIO::Header::Dimlim), Bit(&DwgIO::Header::Dimtih),
            Bit(&DwgIO::Header::Dimtoh), Bit(&DwgIO::Header::Dimse1), Bit(&DwgIO::Header::Dimse2),
            Short(&DwgIO::Header::Dimtad), Short(&DwgIO::Header::Dimzin), Short(&DwgIO::Header::Dimazin)),
      Since(Version::R2007, Short(&DwgIO::Header::Dimarcsym)), Double(&DwgIO::Header::Dimtxt),
      Double(&DwgIO::Header::Dimcen), Double(&DwgIO::Header::Dimtsz), Double(&DwgIO::Header::Dimaltf),
      Double(&DwgIO::Header::Dimlfac), Double(&DwgIO::Header::Dimtvp), Double(&DwgIO::Header::Dimtfac),
      Double(&DwgIO::Header::Dimgap),
      Until(Version::R14, Text(&DwgIO::Header::Dimpost), Text(&DwgIO::Header::Dimapost),
            Text(&DwgIO::Header::DimblkName), Text(&DwgIO::Header::Dimblk1Name), Text(&DwgIO::Header::Dimblk2Name)),
      Since(Version::R2000, Double(&DwgIO::Header::Dimaltrnd), Bit(&DwgIO::Header::Dimalt),
            Short(&DwgIO::Header::Dimaltd), Bit(&DwgIO::Header::Dimtofl), Bit(&DwgIO::Header::Dimsah),
            Bit(&DwgIO::Header::Dimtix), Bit(&DwgIO::Header::Dimsoxd)),
      CmColor(&DwgIO::Header::Dimclrd), CmColor(&DwgIO::Header::Dimclre), CmColor(&DwgIO::Header::Dimclrt),
      Since(Version::R2000, Short(&DwgIO::Header::Dimadec), Short(&DwgIO::Header::Dimdec),
            Short(&DwgIO::Header::Dimtdec), Short(&DwgIO::Header::Dimaltu), Short(&DwgIO::Header::Dimalttd),
            Short(&DwgIO::Header::Dimaunit), Short(&DwgIO::Header::Dimfrac), Short(&DwgIO::Header::Dimlunit),
            Short(&DwgIO::Header::Dimdsep), Short(&DwgIO::Header::Dimtmove), Short(&DwgIO::Header::Dimjust),
            Bit(&DwgIO::Header::Dimsd1), Bit(&DwgIO::Header::Dimsd2), Short(&DwgIO::Header::Dimtolj),
            Short(&DwgIO::Header::Dimtzin), Short(&DwgIO::Header::Dimaltz), Short(&DwgIO::Header::Dimalttz),
            Bit(&DwgIO::Header::Dimupt), Short(&DwgIO::Header::Dimatfit),
            Since(Version::R2007, Bit(&DwgIO::Header::Dimfxlon)),
            Since(Version::R2010, Bit(&DwgIO::Header::Dimtxtdirection), Double(&DwgIO::Header::Dimaltmzf),
                  Text(&DwgIO::Header::Dimaltmzs), Double(&DwgIO::Header::Dimmzf), Text(&DwgIO::Header::Dimmzs)),
            StreamHandle(&DwgIO::Header::Dimtxsty, 5), StreamHandle(&DwgIO::Header::Dimldrblk, 5),
            StreamHandle(&DwgIO::Header::Dimblk, 5), StreamHandle(&DwgIO::Header::Dimblk1, 5),
            StreamHandle(&DwgIO::Header::Dimblk2, 5),
            Since(Version::R2007, StreamHandle(&DwgIO::Header::Dimltype, 5), StreamHandle(&DwgIO::Header::Dimltex1, 5),
                  StreamHandle(&DwgIO::Header::Dimltex2, 5)),
            Short(&DwgIO::Header::Dimlwd), Short(&DwgIO::Header::Dimlwe)),
      StreamHandle(&DwgIO::Header::BlockControlObject, 3), StreamHandle(&DwgIO::Header::LayerControlObject, 3),
      StreamHandle(&DwgIO::Header::StyleControlObject, 3), StreamHandle(&DwgIO::Header::LTypeControlObject, 3),
      StreamHandle(&DwgIO::Header::ViewControlObject, 3), StreamHandle(&DwgIO::Header::UcsControlObject, 3),
      StreamHandle(&DwgIO::Header::VPortControlObject, 3), StreamHandle(&DwgIO::Header::AppIdControlObject, 3),
      StreamHandle(&DwgIO::Header::DimStyleControlObject, 3),
      Until(Version::R2000, StreamHandle(&DwgIO::Header::ViewportEntityHeaderControlObject, 3)),
      StreamHandle(&DwgIO::Header::DictionaryAcadGroup, 5), StreamHandle(&DwgIO::Header::DictionaryAcadMLineStyle, 5),
      StreamHandle(&DwgIO::Header::DictionaryNamedObjects, 3),
      Since(Version::R2000, Short(&DwgIO::Header::Tstackalign), Short(&DwgIO::Header::Tstacksize),
            Text(&DwgIO::Header::Hyperlinkbase), Text(&DwgIO::Header::Stylesheet),
            StreamHandle(&DwgIO::Header::DictionaryLayouts, 5), StreamHandle(&DwgIO::Header::DictionaryPlotSettings, 5),
            StreamHandle(&DwgIO::Header::DictionaryPlotStyles, 5),
            Since(Version::R2004, StreamHandle(&DwgIO::Header::DictionaryMaterials, 5),
                  StreamHandle(&DwgIO::Header::DictionaryColors, 5)),
            Since(Version::R2007, StreamHandle(&DwgIO::Header::DictionaryVisualStyles, 5)),
            Since(Version::R2013, StreamHandle(&DwgIO::Header::DictionaryUnknown, 5)), Long(&DwgIO::Header::Flags),
            Short(&DwgIO::Header::Insunits), Short(&DwgIO::Header::Cepsntype)),
      When(HasCpsnid, StreamHandle(&DwgIO::Header::Cpsnid, 5)),
      Since(Version::R2000, Text(&DwgIO::Header::Fingerprintguid), Text(&DwgIO::Header::Versionguid)),
      Since(Version::R2004, Byte(&DwgIO::Header::Sortents), Byte(&DwgIO::Header::Indexctl),
            Byte(&DwgIO::Header::Hidetext), Byte(&DwgIO::Header::Xclipframe), Byte(&DwgIO::Header::Dimassoc),
            Byte(&DwgIO::Header::Halogap), Short(&DwgIO::Header::Obscuredcolor),
            Short(&DwgIO::Header::Intersectioncolor), Byte(&DwgIO::Header::Obscuredltype),
            Byte(&DwgIO::Header::Intersectiondisplay), Text(&DwgIO::Header::Projectname)),
      StreamHandle(&DwgIO::Header::BlockRecordPaperSpace, 5), StreamHandle(&DwgIO::Header::BlockRecordModelSpace, 5),
      StreamHandle(&DwgIO::Header::LTypeByLayer, 5), StreamHandle(&DwgIO::Header::LTypeByBlock, 5),
      StreamHandle(&DwgIO::Header::LTypeContinuous, 5),
      Since(Version::R2007, Bit(&DwgIO::Header::Cameradisplay), Long(&DwgIO::Header::Unknown23),
            Long(&DwgIO::Header::Unknown24), Double(&DwgIO::Header::Unknown25), Double(&DwgIO::Header::Stepspersec),
            Double(&DwgIO::Header::Stepsize), Double(&DwgIO::Header::Dwfprec3d), Double(&DwgIO::Header::Lenslength),
            Double(&DwgIO::Header::Cameraheight), Byte(&DwgIO::Header::Solidhist), Byte(&DwgIO::Header::Showhist),
            Double(&DwgIO::Header::Psolwidth), Double(&DwgIO::Header::Psolheight), Double(&DwgIO::Header::Loftang1),
            Double(&DwgIO::Header::Loftang2), Double(&DwgIO::Header::Loftmag1), Double(&DwgIO::Header::Loftmag2),
            Short(&DwgIO::Header::Loftparam), Byte(&DwgIO::Header::Loftnormals), Double(&DwgIO::Header::Latitude),
            Double(&DwgIO::Header::Longitude), Double(&DwgIO::Header::Northdirection), Long(&DwgIO::Header::Timezone),
            Byte(&DwgIO::Header::Lightglyphdisplay), Byte(&DwgIO::Header::Tilemodelightsynch),
            Byte(&DwgIO::Header::Dwfframe), Byte(&DwgIO::Header::Dgnframe), Bit(&DwgIO::Header::Realworldscale),
            CmColor(&DwgIO::Header::Interferecolor), StreamHandle(&DwgIO::Header::Interfereobjvs, 5),
            StreamHandle(&DwgIO::Header::Interferevpvs, 5), StreamHandle(&DwgIO::Header::Dragvs, 5),
            Byte(&DwgIO::Header::Cshadow), Double(&DwgIO::Header::Shadowplanelocation)),
      Until(Version::R2004, Short(&DwgIO::Header::Unknown12), Short(&DwgIO::Header::Unknown13),
            Short(&DwgIO::Header::Unknown14), Short(&DwgIO::Header::Unknown15)));
}

/**
 *  Reads the header variables of a section body; a body cut short gives what fit and an issue.
 */
[[nodiscard]] auto Read(std::span<const std::byte> body, const Fields::Context &with) -> DwgIO::Header;

/**
 *  Writes the header variables, then the padding bits as they were, or one bits to the byte when there are none.
 */
[[nodiscard]] auto Write(const DwgIO::Header &header, const Fields::Context &with) -> std::vector<std::byte>;
} // namespace DwgIO::Variables

#endif /* DWGIO_VARIABLES_LAYOUT_HPP */
