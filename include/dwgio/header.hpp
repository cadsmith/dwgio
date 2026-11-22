/***********************************************************************************************************************
 *  @file      dwgio/header.hpp
 *  @brief     The header variables of R14 to R2018.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_HEADER_HPP
#define DWGIO_HEADER_HPP

#include "dwgio/api.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/vector.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DwgIO
{
/**
 *  The header variables for R14 to R2018, one member per item in the specification's order; an item two releases store
 *  differently has one member of the wider type.
 */
struct DWGIO_API Header
{
  double Unknown1{};                    /**< Unnamed in the specification; normally 412148564080.0. */
  double Unknown2{1};                   /**< Unnamed in the specification; normally 1.0.            */
  double Unknown3{1};                   /**< Unnamed in the specification; normally 1.0.            */
  double Unknown4{1};                   /**< Unnamed in the specification; normally 1.0.            */
  std::string Unknown5{};               /**< Unnamed in the specification; "m" or empty.            */
  std::string Unknown6{};               /**< Unnamed in the specification; normally empty.          */
  std::string Unknown7{};               /**< Unnamed in the specification; normally empty.          */
  std::string Unknown8{};               /**< Unnamed in the specification; normally empty.          */
  std::int32_t Unknown9{};              /**< Unnamed in the specification; 24 or 0.                 */
  std::int32_t Unknown10{};             /**< Unnamed in the specification; normally 0.              */
  int Unknown11{};                      /**< Unnamed in the specification. R14 only.                */
  Handle CurrentViewportEntityHeader{}; /**< The current viewport entity header, up to R2000.       */
  bool Dimaso{};
  bool Dimsho{};
  bool Dimsav{}; /**< R14 only. */
  bool Plinegen{};
  bool Orthomode{};
  bool Regenmode{};
  bool Fillmode{};
  bool Qtextmode{};
  bool Psltscale{};
  bool Limcheck{};
  bool Blipmode{}; /**< R14 only. */
  bool Usrtimer{};
  bool Skpoly{};
  bool Angdir{};
  bool Splframe{};
  bool Attreq{}; /**< R14 only. */
  bool Attdia{}; /**< R14 only. */
  bool Mirrtext{};
  bool Worldview{};
  bool Wireframe{}; /**< R14 only. */
  bool Tilemode{};
  bool Plimcheck{};
  bool Visretain{};
  bool Delobj{}; /**< R14 only. */
  bool Dispsilh{};
  bool Pellipse{};
  int Proxygraphics{};
  int Dragmode{}; /**< R14 only. */
  int Treedepth{};
  int Lunits{};
  int Luprec{};
  int Aunits{};
  int Auprec{};
  int Osmode{}; /**< R14 only. */
  int Attmode{};
  int Coords{}; /**< R14 only. */
  int Pdmode{};
  int Pickstyle{}; /**< R14 only. */
  int Useri1{};
  int Useri2{};
  int Useri3{};
  int Useri4{};
  int Useri5{};
  int Splinesegs{};
  int Surfu{};
  int Surfv{};
  int Surftype{};
  int Surftab1{};
  int Surftab2{};
  int Splinetype{};
  int Shadedge{};
  int Shadedif{};
  int Unitmode{};
  int Maxactvp{};
  int Isolines{};
  int Cmljust{};
  int Textqlty{};
  double Ltscale{};
  double Textsize{};
  double Tracewid{};
  double Sketchinc{};
  double Filletrad{};
  double Thickness{};
  double Angbase{};
  double Pdsize{};
  double Plinewid{};
  double Userr1{};
  double Userr2{};
  double Userr3{};
  double Userr4{};
  double Userr5{};
  double Chamfera{};
  double Chamferb{};
  double Chamferc{};
  double Chamferd{};
  double Facetres{};
  double Cmlscale{};
  double Celtscale{};
  std::string Menuname{};              /**< Up to R2004.                   */
  std::int32_t TdcreateDays{};         /**< The Julian day.                */
  std::int32_t TdcreateMilliseconds{}; /**< The milliseconds into the day. */
  std::int32_t TdupdateDays{};         /**< The Julian day.                */
  std::int32_t TdupdateMilliseconds{}; /**< The milliseconds into the day. */
  std::int32_t TdindwgDays{};
  std::int32_t TdindwgMilliseconds{}; /**< The milliseconds into the day. */
  std::int32_t TdusrtimerDays{};
  std::int32_t TdusrtimerMilliseconds{}; /**< The milliseconds into the day. */
  Color Cecolor{};
  Handle Handseed{};  /**< The next handle to give out. */
  Handle Clayer{};    /**< The current layer.           */
  Handle Textstyle{}; /**< The current text style.      */
  Handle Celtype{};   /**< The current linetype.        */
  Handle Dimstyle{};  /**< The current dimension style. */
  Handle Cmlstyle{};  /**< The current multiline style. */
  double Psvpscale{}; /**< From R2000.                  */
  Vector Pinsbase{};
  Vector Pextmin{};
  Vector Pextmax{};
  Vector Plimmin{};
  Vector Plimmax{};
  double Pelevation{};
  Vector Pucsorg{};
  Vector Pucsxdir{};
  Vector Pucsydir{};
  Handle Pucsname{};      /**< The paper space UCS. */
  Handle Pucsorthoref{};  /**< From R2000.          */
  int Pucsorthoview{};    /**< From R2000.          */
  Handle Pucsbase{};      /**< From R2000.          */
  Vector Pucsorgtop{};    /**< From R2000.          */
  Vector Pucsorgbottom{}; /**< From R2000.          */
  Vector Pucsorgleft{};   /**< From R2000.          */
  Vector Pucsorgright{};  /**< From R2000.          */
  Vector Pucsorgfront{};  /**< From R2000.          */
  Vector Pucsorgback{};   /**< From R2000.          */
  Vector Insbase{};
  Vector Extmin{};
  Vector Extmax{};
  Vector Limmin{};
  Vector Limmax{};
  double Elevation{};
  Vector Ucsorg{};
  Vector Ucsxdir{};
  Vector Ucsydir{};
  Handle Ucsname{};      /**< The model space UCS. */
  Handle Ucsorthoref{};  /**< From R2000.          */
  int Ucsorthoview{};    /**< From R2000.          */
  Handle Ucsbase{};      /**< From R2000.          */
  Vector Ucsorgtop{};    /**< From R2000.          */
  Vector Ucsorgbottom{}; /**< From R2000.          */
  Vector Ucsorgleft{};   /**< From R2000.          */
  Vector Ucsorgright{};  /**< From R2000.          */
  Vector Ucsorgfront{};  /**< From R2000.          */
  Vector Ucsorgback{};   /**< From R2000.          */
  std::string Dimpost{};
  std::string Dimapost{};
  bool Dimtol{};
  bool Dimlim{};
  bool Dimtih{};
  bool Dimtoh{};
  bool Dimse1{};
  bool Dimse2{};
  bool Dimalt{};
  bool Dimtofl{};
  bool Dimsah{};
  bool Dimtix{};
  bool Dimsoxd{};
  int Dimaltd{};
  int Dimzin{};
  bool Dimsd1{};
  bool Dimsd2{};
  int Dimtolj{};
  int Dimjust{};
  int Dimfit{}; /**< R14 only. */
  bool Dimupt{};
  int Dimtzin{};
  int Dimaltz{};
  int Dimalttz{};
  int Dimtad{};
  int Dimunit{}; /**< R14 only. */
  int Dimaunit{};
  int Dimdec{};
  int Dimtdec{};
  int Dimaltu{};
  int Dimalttd{};
  Handle Dimtxsty{};
  double Dimscale{};
  double Dimasz{};
  double Dimexo{};
  double Dimdli{};
  double Dimexe{};
  double Dimrnd{};
  double Dimdle{};
  double Dimtp{};
  double Dimtm{};
  int Dimazin{}; /**< From R2000. */
  double Dimtxt{};
  double Dimcen{};
  double Dimtsz{};
  double Dimaltf{};
  double Dimlfac{};
  double Dimtvp{};
  double Dimtfac{};
  double Dimgap{};
  std::string DimblkName{};  /**< R14 only.   */
  std::string Dimblk1Name{}; /**< R14 only.   */
  std::string Dimblk2Name{}; /**< R14 only.   */
  double Dimaltrnd{};        /**< From R2000. */
  Color Dimclrd{};
  Color Dimclre{};
  Color Dimclrt{};
  int Dimadec{};      /**< From R2000. */
  int Dimfrac{};      /**< From R2000. */
  int Dimlunit{};     /**< From R2000. */
  int Dimdsep{};      /**< From R2000. */
  int Dimtmove{};     /**< From R2000. */
  int Dimatfit{};     /**< From R2000. */
  Handle Dimldrblk{}; /**< From R2000. */
  Handle Dimblk{};    /**< From R2000. */
  Handle Dimblk1{};   /**< From R2000. */
  Handle Dimblk2{};   /**< From R2000. */
  int Dimlwd{};       /**< From R2000. */
  int Dimlwe{};       /**< From R2000. */
  Handle BlockControlObject{};
  Handle LayerControlObject{};
  Handle StyleControlObject{};
  Handle LTypeControlObject{};
  Handle ViewControlObject{};
  Handle UcsControlObject{};
  Handle VPortControlObject{};
  Handle AppIdControlObject{};
  Handle DimStyleControlObject{};
  Handle ViewportEntityHeaderControlObject{}; /**< Up to R2000. */
  Handle DictionaryAcadGroup{};
  Handle DictionaryAcadMLineStyle{};
  Handle DictionaryNamedObjects{}; /**< The named objects dictionary, the root of the others.             */
  int Tstackalign{};               /**< The stacked text alignment, from R2000; 1 by default, not in DXF. */
  int Tstacksize{};                /**< The stacked text size, from R2000; 70 by default, not in DXF.     */
  std::string Hyperlinkbase{};     /**< From R2000.                                                       */
  std::string Stylesheet{};        /**< From R2000.                                                       */
  Handle DictionaryLayouts{};      /**< The ACAD_LAYOUT dictionary, from R2000.                           */
  Handle DictionaryPlotSettings{}; /**< The ACAD_PLOTSETTINGS dictionary, from R2000.                     */
  Handle DictionaryPlotStyles{};   /**< The ACAD_PLOTSTYLENAME dictionary, from R2000.                    */
  /**
   *  From R2000: $CELWEIGHT in bits 0 to 4, $ENDCAPS in 5 and 6, $JOINSTYLE in 7 and 8, then $LWDISPLAY, $XEDIT,
   *  $EXTNAMES, $PSTYLEMODE and $OLESTARTUP as 0x0200, 0x0400, 0x0800, 0x2000 and 0x4000.
   */
  std::int32_t Flags{};
  int Insunits{};                /**< From R2000.                                   */
  int Cepsntype{};               /**< From R2000.                                   */
  Handle Cpsnid{};               /**< From R2000, stored only when $CEPSNTYPE is 3. */
  std::string Fingerprintguid{}; /**< From R2000.                                   */
  std::string Versionguid{};     /**< From R2000.                                   */
  Handle BlockRecordPaperSpace{};
  Handle BlockRecordModelSpace{};
  Handle LTypeByLayer{};
  Handle LTypeByBlock{};
  Handle LTypeContinuous{};
  int Unknown12{};  /**< Unnamed in the specification. From R14 to R2004.                                            */
  int Unknown13{};  /**< Unnamed in the specification. From R14 to R2004.                                            */
  int Unknown14{};  /**< Unnamed in the specification. From R14 to R2004.                                            */
  int Unknown15{};  /**< Unnamed in the specification. From R14 to R2004.                                            */
  bool Unknown16{}; /**< Unnamed in the specification: a bit after BLIPMODE's place. From R2004.                     */
  int Unknown17{};  /**< Unnamed in the specification: the first of three longs after PICKSTYLE's place. From R2004. */
  int Unknown18{};  /**< Unnamed in the specification: the second of three longs after PICKSTYLE's place. From R2004. */
  int Unknown19{};  /**< Unnamed in the specification: the third of three longs after PICKSTYLE's place. From R2004. */
  int Unknown20{};  /**< Unnamed in the specification: the first of three longs after TDUPDATE. From R2004. */
  int Unknown21{};  /**< Unnamed in the specification: the second of three longs after TDUPDATE. From R2004. */
  int Unknown22{};  /**< Unnamed in the specification: the third of three longs after TDUPDATE. From R2004. */
  Handle DictionaryMaterials{};    /**< The ACAD_MATERIAL dictionary, from R2004.                 */
  Handle DictionaryColors{};       /**< The ACAD_COLOR dictionary, from R2004.                    */
  int Sortents{};                  /**< From R2004.                                               */
  int Indexctl{};                  /**< From R2004.                                               */
  int Hidetext{};                  /**< From R2004.                                               */
  int Xclipframe{};                /**< From R2004; 0 or 1 before R2010.                          */
  int Dimassoc{};                  /**< From R2004.                                               */
  int Halogap{};                   /**< From R2004.                                               */
  int Obscuredcolor{};             /**< From R2004.                                               */
  int Intersectioncolor{};         /**< From R2004.                                               */
  int Obscuredltype{};             /**< From R2004.                                               */
  int Intersectiondisplay{};       /**< From R2004.                                               */
  std::string Projectname{};       /**< From R2004.                                               */
  std::int64_t Requiredversions{}; /**< From R2013.                                               */
  Handle Cmaterial{};              /**< The material new entities take, from R2007.               */
  double Dimfxl{};                 /**< From R2007.                                               */
  double Dimjogang{};              /**< From R2007.                                               */
  int Dimtfill{};                  /**< From R2007.                                               */
  Color Dimtfillclr{};             /**< From R2007.                                               */
  int Dimarcsym{};                 /**< From R2007.                                               */
  bool Dimfxlon{};                 /**< From R2007.                                               */
  bool Dimtxtdirection{};          /**< From R2010.                                               */
  double Dimaltmzf{};              /**< From R2010.                                               */
  std::string Dimaltmzs{};         /**< From R2010.                                               */
  double Dimmzf{};                 /**< From R2010.                                               */
  std::string Dimmzs{};            /**< From R2010.                                               */
  Handle Dimltype{};               /**< From R2007.                                               */
  Handle Dimltex1{};               /**< From R2007.                                               */
  Handle Dimltex2{};               /**< From R2007.                                               */
  Handle DictionaryVisualStyles{}; /**< The ACAD_VISUALSTYLE dictionary, from R2007.              */
  Handle DictionaryUnknown{};      /**< A dictionary the specification does not name, from R2013. */
  bool Cameradisplay{};            /**< From R2007.                                               */
  /**
   *  Unnamed in the specification: the first of two longs and a double after CAMERADISPLAY; 0 in a new drawing. From
   *  R2007.
   */
  std::int32_t Unknown23{};
  std::int32_t
      Unknown24{};       /**< Unnamed in the specification: the second long after CAMERADISPLAY; 10 in a new drawing. */
  double Unknown25{};    /**< Unnamed in the specification: the double after CAMERADISPLAY; 1 in a new drawing.    */
  double Stepspersec{};  /**< From R2007.                                                */
  double Stepsize{};     /**< From R2007.                                                */
  double Dwfprec3d{};    /**< From R2007.                                                */
  double Lenslength{};   /**< From R2007.                                                */
  double Cameraheight{}; /**< From R2007.                                                */
  int Solidhist{};       /**< From R2007.                                                */
  int Showhist{};        /**< From R2007.                                                */
  double Psolwidth{};    /**< From R2007.                                                */
  double Psolheight{};   /**< From R2007.                                                */
  double Loftang1{};     /**< From R2007.                                                */
  double Loftang2{};     /**< From R2007.                                                */
  double Loftmag1{};     /**< From R2007.                                                */
  double Loftmag2{};     /**< From R2007.                                                */
  int Loftparam{};       /**< From R2007.                                                */
  int Loftnormals{};     /**< From R2007.                                                */
  double Latitude{};     /**< From R2007.                                                */
  double Longitude{};    /**< From R2007.                                                */
  double Northdirection{};      /**< From R2007.                                                */
  std::int32_t Timezone{};      /**< From R2007.                                                */
  int Lightglyphdisplay{};      /**< From R2007.                                                */
  int Tilemodelightsynch{};     /**< From R2007.                                                */
  int Dwfframe{};               /**< From R2007.                                                */
  int Dgnframe{};               /**< From R2007.                                                */
  bool Realworldscale{};        /**< From R2007.                                                */
  Color Interferecolor{};       /**< From R2007.                                                */
  Handle Interfereobjvs{};      /**< The visual style of interference objects, from R2007.      */
  Handle Interferevpvs{};       /**< The visual style of the interference viewport, from R2007. */
  Handle Dragvs{};              /**< The visual style while dragging, from R2007.               */
  int Cshadow{};                /**< From R2007.                                                */
  double Shadowplanelocation{}; /**< From R2007.                                                */
  /**
   *  The bits after the last item up to the body's end, as the file had them, the last byte filled with zeros; from
   *  R2007 the bits after the handles.
   */
  std::vector<std::byte> Padding{};

  auto operator==(const Header &) const -> bool = default;
};
} // namespace DwgIO

#endif /* DWGIO_HEADER_HPP */
