/***********************************************************************************************************************
 *  @file      dwgio/tables/dimstyle.hpp
 *  @brief     The DIMSTYLE record: one member per DIM variable.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_DIMSTYLE_HPP
#define DWGIO_TABLES_DIMSTYLE_HPP

#include "dwgio/api.hpp"
#include "dwgio/color.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/tables/record.hpp"

#include <numbers>
#include <string>

namespace DwgIO::Tables
{
/**
 *  The arrow blocks by name, as a file up to R14 stores them; later releases give them as handles.
 */
struct DWGIO_API Arrows
{
  std::string Dimblk{};  /**< The block drawn at both ends; empty for the closed filled arrow. Group 5. */
  std::string Dimblk1{}; /**< The block at the first end when DIMSAH is on. Group 6.                    */
  std::string Dimblk2{}; /**< The block at the second end when DIMSAH is on. Group 7.                   */

  auto operator==(const Arrows &) const -> bool = default;
};

/**
 *  A DIMSTYLE record: one member per DIM variable.
 */
struct DWGIO_API DimStyle
{
  Tables::Record Record{};
  Tables::Arrows Arrows{}; /**< Up to R14. */
  /**
   *  The prefix and suffix of measurements, with <> where the measurement goes. Group 3.
   */
  std::string Dimpost{};
  std::string Dimapost{}; /**< The prefix and suffix of alternate measurements. Group 4.                            */
  double Dimscale{1};     /**< The factor every size of the style is scaled by. Group 40.                           */
  double Dimasz{0.18};    /**< The size of arrowheads. Group 41.                                                    */
  double Dimexo{0.0625};  /**< How far extension lines start from the points they measure. Group 42.                */
  double Dimdli{0.38};    /**< The spacing between the dimension lines of baseline dimensions. Group 43.            */
  double Dimexe{0.18};    /**< How far extension lines run past the dimension line. Group 44.                       */
  double Dimrnd{};        /**< The value distances are rounded to; 0 for none. Group 45.                            */
  double Dimdle{};        /**< How far the dimension line runs past extension lines when ticks are drawn. Group 46. */
  double Dimtp{};         /**< The plus tolerance. Group 47.                                                        */
  double Dimtm{};         /**< The minus tolerance. Group 48.                                                       */
  double Dimfxl{1};       /**< The fixed length of extension lines. Group 49, from R2007.                           */
  /**
   *  The angle of a jogged radius dimension's jog, in radians. Group 50, from R2007.
   */
  double Dimjogang{std::numbers::pi / 4};
  /**
   *  The text's background: 0 none, 1 the drawing's background, 2 DIMTFILLCLR. Group 69, from R2007.
   */
  int Dimtfill{};
  /**
   *  The text's background color. Group 70, from R2007.
   */
  DwgIO::Color Dimtfillclr{.Index = 0, .Rgb = {}, .Transparency = {}};
  bool Dimtol{};     /**< Whether tolerances are appended to measurements. Group 71.       */
  bool Dimlim{};     /**< Whether measurements are given as limits. Group 72.              */
  bool Dimtih{true}; /**< Whether text between extension lines stays horizontal. Group 73. */
  bool Dimtoh{true}; /**< Whether text outside extension lines stays horizontal. Group 74. */
  bool Dimse1{};     /**< Whether the first extension line is left out. Group 75.          */
  bool Dimse2{};     /**< Whether the second extension line is left out. Group 76.         */
  /**
   *  Where text stands against the dimension line: 0 centered, 1 above, 2 outside, 3 JIS, 4 below. Group 77.
   */
  int Dimtad{};
  /**
   *  Which zeros measurements drop: 0 to 3 for feet and inches, 4 leading, 8 trailing. Group 78.
   */
  int Dimzin{};
  int Dimazin{}; /**< Which zeros angles drop: 1 leading, 2 trailing, 3 both. Group 79, from R2000. */
  /**
   *  Where an arc length's symbol goes: 0 before the text, 1 above it, 2 none. Group 90, from R2007.
   */
  int Dimarcsym{};
  double Dimtxt{0.18};  /**< The text height. Group 140.                                                        */
  double Dimcen{0.09};  /**< The size of center marks; negative to draw center lines, 0 for none. Group 141.    */
  double Dimtsz{};      /**< The size of ticks drawn in place of arrowheads; 0 for arrowheads. Group 142.       */
  double Dimaltf{25.4}; /**< The factor alternate units are measured by. Group 143.                             */
  double Dimlfac{1};    /**< The factor linear measurements are scaled by. Group 144.                           */
  double Dimtvp{};      /**< How far text stands above or below the dimension line when DIMTAD is 0. Group 145. */
  double Dimtfac{1};    /**< The height of tolerance text over the text height. Group 146.                      */
  /**
   *  The gap around text where it breaks the dimension line; negative to box the text. Group 147.
   */
  double Dimgap{0.09};
  double Dimaltrnd{}; /**< The value alternate measurements are rounded to. Group 148, from R2000. */
  bool Dimalt{};      /**< Whether alternate units are added to measurements. Group 170.           */
  int Dimaltd{2};     /**< The decimal places of alternate units. Group 171.                       */
  /**
   *  Whether the dimension line is drawn between extension lines when text is outside. Group 172.
   */
  bool Dimtofl{};
  bool Dimsah{};   /**< Whether the arrows come from DIMBLK1 and DIMBLK2 rather than DIMBLK. Group 173. */
  bool Dimtix{};   /**< Whether text is kept between extension lines. Group 174.                        */
  bool Dimsoxd{};  /**< Whether dimension lines outside extension lines are left out. Group 175.        */
  Color Dimclrd{}; /**< The color of dimension lines, arrowheads and leaders. Group 176.                */
  Color Dimclre{}; /**< The color of extension lines. Group 177.                                        */
  Color Dimclrt{}; /**< The color of text. Group 178.                                                   */
  int Dimadec{};   /**< The decimal places of angles. Group 179.                                        */
  int Dimdec{4};   /**< The decimal places of measurements. Group 271.                                  */
  int Dimtdec{4};  /**< The decimal places of tolerances. Group 272.                                    */
  int Dimaltu{2};  /**< The format of alternate units, as DIMLUNIT. Group 273.                          */
  int Dimalttd{2}; /**< The decimal places of alternate tolerances. Group 274.                          */
  /**
   *  The format of angles: 0 degrees, 1 degrees minutes seconds, 2 gradians, 3 radians, 4 surveyor's. Group 275.
   */
  int Dimaunit{};
  int Dimfrac{}; /**< How fractions stack: 0 horizontal, 1 diagonal, 2 not stacked. Group 276, from R2000. */
  /**
   *  The format of measurements: 1 scientific, 2 decimal, 3 engineering, 4 architectural, 5 fractional, 6 as Windows
   *  sets it. Group 277, from R2000.
   */
  int Dimlunit{2};
  int Dimdsep{'.'}; /**< The character that separates decimals. Group 278, from R2000. */
  /**
   *  How text moves: 0 with the dimension line, 1 with a leader, 2 freely. Group 279, from R2000.
   */
  int Dimtmove{};
  /**
   *  Where text stands along the dimension line: 0 centered, 1 and 2 by the first and second extension line, 3 and 4
   *  above them. Group 280.
   */
  int Dimjust{};
  bool Dimsd1{};  /**< Whether the first half of the dimension line is left out. Group 281.                  */
  bool Dimsd2{};  /**< Whether the second half of the dimension line is left out. Group 282.                 */
  int Dimtolj{1}; /**< Where tolerances stand against the measurement: 0 bottom, 1 middle, 2 top. Group 283. */
  int Dimtzin{};  /**< Which zeros tolerances drop, as DIMZIN. Group 284.                                    */
  int Dimaltz{};  /**< Which zeros alternate units drop, as DIMZIN. Group 285.                               */
  int Dimalttz{}; /**< Which zeros alternate tolerances drop, as DIMZIN. Group 286.                          */
  bool Dimupt{};  /**< Whether the cursor places the text as well as the dimension line. Group 288.          */
  /**
   *  How text and arrows are placed when both do not fit, as R14 had it, 0 to 5. Group 287, up to R14.
   */
  int Dimfit{3};
  /**
   *  What moves outside when text and arrows do not fit: 0 both, 1 arrows, 2 text, 3 whichever fits best. Group 289,
   *  from R2000.
   */
  int Dimatfit{3};
  bool Dimfxlon{};        /**< Whether extension lines take the fixed length DIMFXL. Group 290, from R2007. */
  bool Dimtxtdirection{}; /**< Whether the text reads right to left. Group 295, from R2010.                 */
  /**
   *  The factor alternate units below one are shown in a smaller unit by. From R2010.
   */
  double Dimaltmzf{100};
  std::string Dimaltmzs{}; /**< The suffix of alternate units shown in the smaller unit. From R2010.   */
  double Dimmzf{100};      /**< The factor units below one are shown in a smaller unit by. From R2010. */
  std::string Dimmzs{};    /**< The suffix of units shown in the smaller unit. From R2010.             */
  /**
   *  The format of measurements as R14 had it, before DIMLUNIT and DIMFRAC split it. Group 270, up to R14.
   */
  int Dimunit{2};
  /**
   *  The lineweight of dimension lines, in hundredths of a millimeter; -1 ByLayer, -2 ByBlock, -3 the default. Group
   *  371, from R2000.
   */
  int Dimlwd{-2};
  int Dimlwe{-2}; /**< The lineweight of extension lines, as DIMLWD. Group 372, from R2000. */
  /**
   *  Unnamed in the specification: a bit after the variables, which sets the 1 bit of group 70 when on.
   */
  bool Unknown1{};
  Handle Dimtxsty{}; /**< The text style of the text. Group 340. */
  /**
   *  The block record of leader arrows; none for the closed filled arrow. Group 341, from R2000.
   */
  Handle Dimldrblk{};
  /**
   *  The block record of both arrows; none for the closed filled arrow. Group 342, from R2000.
   */
  Handle Dimblk{};
  Handle Dimblk1{};  /**< The block record of the first arrow when DIMSAH is on. Group 343, from R2000.  */
  Handle Dimblk2{};  /**< The block record of the second arrow when DIMSAH is on. Group 344, from R2000. */
  Handle Dimltype{}; /**< The linetype of dimension lines. Group 345, from R2007.                        */
  Handle Dimltex1{}; /**< The linetype of the first extension line. Group 346, from R2007.               */
  Handle Dimltex2{}; /**< The linetype of the second extension line. Group 347, from R2007.              */

  auto operator==(const DimStyle &) const -> bool = default;
};
} // namespace DwgIO::Tables

#endif /* DWGIO_TABLES_DIMSTYLE_HPP */
