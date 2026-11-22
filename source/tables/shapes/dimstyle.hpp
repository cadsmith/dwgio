/***********************************************************************************************************************
 *  @file      source/tables/shapes/dimstyle.hpp
 *  @brief     How a DIMSTYLE record lies in the file, its variables release by release.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_TABLES_SHAPES_DIMSTYLE_HPP
#define DWGIO_TABLES_SHAPES_DIMSTYLE_HPP

#include "fields/codes.hpp"
#include "fields/custom.hpp"
#include "fields/layout.hpp"
#include "fields/nested.hpp"
#include "fields/since.hpp"
#include "fields/together.hpp"
#include "objects/shape.hpp"
#include "tables/shapes/record.hpp"

#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/version.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <string_view>

namespace DwgIO::Tables::Shapes
{
/**
 *  The R13 and R14 variables, in chapter 20.4.68's order.
 */
inline auto OlderDimstyle()
{
  using Fields::Bit, Fields::Byte, Fields::Double, Fields::Short, Fields::Text;
  return Fields::Layout(
      Bit(&DimStyle::Dimtol), Bit(&DimStyle::Dimlim), Bit(&DimStyle::Dimtih), Bit(&DimStyle::Dimtoh),
      Bit(&DimStyle::Dimse1), Bit(&DimStyle::Dimse2), Bit(&DimStyle::Dimalt), Bit(&DimStyle::Dimtofl),
      Bit(&DimStyle::Dimsah), Bit(&DimStyle::Dimtix), Bit(&DimStyle::Dimsoxd), Byte(&DimStyle::Dimaltd),
      Byte(&DimStyle::Dimzin), Bit(&DimStyle::Dimsd1), Bit(&DimStyle::Dimsd2), Byte(&DimStyle::Dimtolj),
      Byte(&DimStyle::Dimjust), Byte(&DimStyle::Dimfit), Bit(&DimStyle::Dimupt), Byte(&DimStyle::Dimtzin),
      Byte(&DimStyle::Dimaltz), Byte(&DimStyle::Dimalttz), Byte(&DimStyle::Dimtad), Short(&DimStyle::Dimunit),
      Short(&DimStyle::Dimaunit), Short(&DimStyle::Dimdec), Short(&DimStyle::Dimtdec), Short(&DimStyle::Dimaltu),
      Short(&DimStyle::Dimalttd), Double(&DimStyle::Dimscale), Double(&DimStyle::Dimasz), Double(&DimStyle::Dimexo),
      Double(&DimStyle::Dimdli), Double(&DimStyle::Dimexe), Double(&DimStyle::Dimrnd), Double(&DimStyle::Dimdle),
      Double(&DimStyle::Dimtp), Double(&DimStyle::Dimtm), Double(&DimStyle::Dimtxt), Double(&DimStyle::Dimcen),
      Double(&DimStyle::Dimtsz), Double(&DimStyle::Dimaltf), Double(&DimStyle::Dimlfac), Double(&DimStyle::Dimtvp),
      Double(&DimStyle::Dimtfac), Double(&DimStyle::Dimgap), Text(&DimStyle::Dimpost), Text(&DimStyle::Dimapost),
      Fields::Part(&DimStyle::Arrows,
                   Fields::Layout(Text(&Arrows::Dimblk), Text(&Arrows::Dimblk1), Text(&Arrows::Dimblk2))),
      Fields::CmColor(&DimStyle::Dimclrd), Fields::CmColor(&DimStyle::Dimclre), Fields::CmColor(&DimStyle::Dimclrt));
}

/**
 *  R14's default DIMFIT.
 */
constexpr int DefaultFit = 3;

/**
 *  R14's default DIMUNIT.
 */
constexpr int DefaultUnit = 2;

/**
 *  R2000 has no field for R14's arrow block names, DIMFIT and DIMUNIT: a value off its default is noted on an R2000
 *  write, since it is left out.
 */
inline auto WriteOlder(const DimStyle &style, const Fields::Output &to) -> bool
{
  const auto note = [&to](std::string_view variable, const auto &value) -> void
  {
    Fields::Note(to, IssueCode::NotKept, std::format("{} {} of R14 not written as R2000", variable, value));
  };
  if (!style.Arrows.Dimblk.empty())
  {
    note("DIMBLK", style.Arrows.Dimblk);
  }
  if (!style.Arrows.Dimblk1.empty())
  {
    note("DIMBLK1", style.Arrows.Dimblk1);
  }
  if (!style.Arrows.Dimblk2.empty())
  {
    note("DIMBLK2", style.Arrows.Dimblk2);
  }
  if (style.Dimfit != DefaultFit)
  {
    note("DIMFIT", style.Dimfit);
  }
  if (style.Dimunit != DefaultUnit)
  {
    note("DIMUNIT", style.Dimunit);
  }
  return true;
}

inline auto ReadOlder(DimStyle & /*style*/, const Fields::Input & /*in*/) -> bool
{
  return true;
}

/**
 *  The R2000 variables, in chapter 20.4.68's order.
 */
inline auto NewerDimstyle()
{
  using Fields::Bit, Fields::Double, Fields::Short, Fields::Text;
  return Fields::Layout(
      Text(&DimStyle::Dimpost), Text(&DimStyle::Dimapost), Double(&DimStyle::Dimscale), Double(&DimStyle::Dimasz),
      Double(&DimStyle::Dimexo), Double(&DimStyle::Dimdli), Double(&DimStyle::Dimexe), Double(&DimStyle::Dimrnd),
      Double(&DimStyle::Dimdle), Double(&DimStyle::Dimtp), Double(&DimStyle::Dimtm),
      Fields::Since(Version::R2007, Double(&DimStyle::Dimfxl), Double(&DimStyle::Dimjogang), Short(&DimStyle::Dimtfill),
                    Fields::CmColor(&DimStyle::Dimtfillclr)),
      Bit(&DimStyle::Dimtol), Bit(&DimStyle::Dimlim), Bit(&DimStyle::Dimtih), Bit(&DimStyle::Dimtoh),
      Bit(&DimStyle::Dimse1), Bit(&DimStyle::Dimse2), Short(&DimStyle::Dimtad), Short(&DimStyle::Dimzin),
      Short(&DimStyle::Dimazin), Fields::Since(Version::R2007, Short(&DimStyle::Dimarcsym)), Double(&DimStyle::Dimtxt),
      Double(&DimStyle::Dimcen), Double(&DimStyle::Dimtsz), Double(&DimStyle::Dimaltf), Double(&DimStyle::Dimlfac),
      Double(&DimStyle::Dimtvp), Double(&DimStyle::Dimtfac), Double(&DimStyle::Dimgap), Double(&DimStyle::Dimaltrnd),
      Bit(&DimStyle::Dimalt), Short(&DimStyle::Dimaltd), Bit(&DimStyle::Dimtofl), Bit(&DimStyle::Dimsah),
      Bit(&DimStyle::Dimtix), Bit(&DimStyle::Dimsoxd), Fields::CmColor(&DimStyle::Dimclrd),
      Fields::CmColor(&DimStyle::Dimclre), Fields::CmColor(&DimStyle::Dimclrt), Short(&DimStyle::Dimadec),
      Short(&DimStyle::Dimdec), Short(&DimStyle::Dimtdec), Short(&DimStyle::Dimaltu), Short(&DimStyle::Dimalttd),
      Short(&DimStyle::Dimaunit), Short(&DimStyle::Dimfrac), Short(&DimStyle::Dimlunit), Short(&DimStyle::Dimdsep),
      Short(&DimStyle::Dimtmove), Short(&DimStyle::Dimjust), Bit(&DimStyle::Dimsd1), Bit(&DimStyle::Dimsd2),
      Short(&DimStyle::Dimtolj), Short(&DimStyle::Dimtzin), Short(&DimStyle::Dimaltz), Short(&DimStyle::Dimalttz),
      Bit(&DimStyle::Dimupt), Short(&DimStyle::Dimatfit), Fields::Since(Version::R2007, Bit(&DimStyle::Dimfxlon)),
      Fields::Since(Version::R2010, Bit(&DimStyle::Dimtxtdirection), Double(&DimStyle::Dimaltmzf),
                    Text(&DimStyle::Dimaltmzs), Double(&DimStyle::Dimmzf), Text(&DimStyle::Dimmzs)),
      Short(&DimStyle::Dimlwd), Short(&DimStyle::Dimlwe));
}
} // namespace DwgIO::Tables::Shapes

namespace DwgIO::Objects
{
template <>
struct Shape<Tables::DimStyle>
{
  static constexpr std::array<std::uint16_t, 1> Types{69};
  static constexpr std::string_view Name = "DIMSTYLE";

  static auto Layout()
  {
    using Tables::DimStyle;
    using namespace Tables::Shapes;
    return Fields::Layout(
        Fields::Part(&DimStyle::Record, RecordLayout()), Fields::Until(Version::R14, Fields::Together(OlderDimstyle())),
        Fields::Since(Version::R2000, Fields::Together(NewerDimstyle()), Fields::Custom(ReadOlder, WriteOlder)),
        Fields::Bit(&DimStyle::Unknown1), Fields::Link(&DimStyle::Dimtxsty, HardPointer),
        Fields::Since(Version::R2000, Fields::Link(&DimStyle::Dimldrblk, HardPointer),
                      Fields::Link(&DimStyle::Dimblk, HardPointer), Fields::Link(&DimStyle::Dimblk1, HardPointer),
                      Fields::Link(&DimStyle::Dimblk2, HardPointer)),
        Fields::Since(Version::R2007, Fields::Link(&DimStyle::Dimltype, HardPointer),
                      Fields::Link(&DimStyle::Dimltex1, HardPointer), Fields::Link(&DimStyle::Dimltex2, HardPointer)));
  }
};
} // namespace DwgIO::Objects

#endif /* DWGIO_TABLES_SHAPES_DIMSTYLE_HPP */
