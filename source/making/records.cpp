/***********************************************************************************************************************
 *  @file      source/making/records.cpp
 *  @brief     The objects AutoCAD's new drawing holds, made one at a time.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "making/records.hpp"

#include "dwgio/base.hpp"
#include "dwgio/class.hpp"
#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/plot.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/record.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/tables/vport.hpp"
#include "dwgio/vector.hpp"
#include "dwgio/view.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Making
{
namespace
{
/**
 *  The bytes a linetype keeps for its shape strings before R2004.
 */
constexpr std::size_t StringsBytes = 256;
/**
 *  The plot flag of the model layout.
 */
constexpr int ModelType = 1024;
/**
 *  Plot units of millimetres.
 */
constexpr int Millimetres = 1;
constexpr double A4Width = 210;
constexpr double A4Height = 297;
constexpr double A4Side = 7.5;
constexpr double A4End = 20;
/**
 *  The item class AutoCAD writes for every object class.
 */
constexpr int ItemClass = 499;
constexpr double Half = 0.5;
} // namespace

auto MakeBase(Handle self, Handle owner) -> Base
{
  return Base{.Self = self, .Owner = owner, .Dictionary = {}, .Reactors = {}, .Extended = {}};
}

auto MakeReactingBase(Handle self, Handle owner) -> Base
{
  return Base{.Self = self, .Owner = owner, .Dictionary = {}, .Reactors = {owner}, .Extended = {}};
}

auto MakeRecord(Handle self, Handle owner, std::string name) -> Tables::Record
{
  return Tables::Record{.Base = MakeBase(self, owner),
                        .Name = std::move(name),
                        .Referenced = true,
                        .XrefIndex = -1,
                        .Dependent = false,
                        .Source = {}};
}

auto ControlOf(Tables::Table table, Handle self) -> Tables::Control
{
  return Tables::Control{.Base = MakeBase(self, {}),
                         .Table = table,
                         .Entries = {},
                         .ModelSpace = {},
                         .PaperSpace = {},
                         .ByBlock = {},
                         .ByLayer = {},
                         .RepeatedEntries = {}};
}

auto LayerZero(Tables::Record record, Handle linetype, Handle style) -> Tables::Layer
{
  Tables::Layer zero;
  zero.Record = std::move(record);
  zero.Style = style;
  zero.Linetype = linetype;
  return zero;
}

auto StandardStyle(Tables::Record record) -> Tables::Style
{
  Tables::Style standard;
  standard.Record = std::move(record);
  standard.Font = "Arial.ttf";
  return standard;
}

auto LtypeOf(Tables::Record record, std::string description) -> Tables::LType
{
  Tables::LType ltype;
  ltype.Record = std::move(record);
  ltype.Description = std::move(description);
  ltype.Strings.assign(StringsBytes, std::byte{0});
  return ltype;
}

auto StandardDimstyle(Tables::Record record, Handle textstyle) -> Tables::DimStyle
{
  Tables::DimStyle plain;
  plain.Record = std::move(record);
  plain.Dimtxsty = textstyle;
  return plain;
}

auto MakeDimStyle(const Header &header, Tables::Record record) -> Tables::DimStyle
{
  Tables::DimStyle style;
  style.Record = std::move(record);
  style.Arrows =
      Tables::Arrows{.Dimblk = header.DimblkName, .Dimblk1 = header.Dimblk1Name, .Dimblk2 = header.Dimblk2Name};
  style.Dimpost = header.Dimpost;
  style.Dimapost = header.Dimapost;
  style.Dimscale = header.Dimscale;
  style.Dimasz = header.Dimasz;
  style.Dimexo = header.Dimexo;
  style.Dimdli = header.Dimdli;
  style.Dimexe = header.Dimexe;
  style.Dimrnd = header.Dimrnd;
  style.Dimdle = header.Dimdle;
  style.Dimtp = header.Dimtp;
  style.Dimtm = header.Dimtm;
  style.Dimtol = header.Dimtol;
  style.Dimlim = header.Dimlim;
  style.Dimtih = header.Dimtih;
  style.Dimtoh = header.Dimtoh;
  style.Dimse1 = header.Dimse1;
  style.Dimse2 = header.Dimse2;
  style.Dimtad = header.Dimtad;
  style.Dimzin = header.Dimzin;
  style.Dimazin = header.Dimazin;
  style.Dimtxt = header.Dimtxt;
  style.Dimcen = header.Dimcen;
  style.Dimtsz = header.Dimtsz;
  style.Dimaltf = header.Dimaltf;
  style.Dimlfac = header.Dimlfac;
  style.Dimtvp = header.Dimtvp;
  style.Dimtfac = header.Dimtfac;
  style.Dimgap = header.Dimgap;
  style.Dimaltrnd = header.Dimaltrnd;
  style.Dimalt = header.Dimalt;
  style.Dimaltd = header.Dimaltd;
  style.Dimtofl = header.Dimtofl;
  style.Dimsah = header.Dimsah;
  style.Dimtix = header.Dimtix;
  style.Dimsoxd = header.Dimsoxd;
  style.Dimclrd = header.Dimclrd;
  style.Dimclre = header.Dimclre;
  style.Dimclrt = header.Dimclrt;
  style.Dimadec = header.Dimadec;
  style.Dimdec = header.Dimdec;
  style.Dimtdec = header.Dimtdec;
  style.Dimaltu = header.Dimaltu;
  style.Dimalttd = header.Dimalttd;
  style.Dimaunit = header.Dimaunit;
  style.Dimfrac = header.Dimfrac;
  style.Dimlunit = header.Dimlunit;
  style.Dimdsep = header.Dimdsep;
  style.Dimtmove = header.Dimtmove;
  style.Dimjust = header.Dimjust;
  style.Dimsd1 = header.Dimsd1;
  style.Dimsd2 = header.Dimsd2;
  style.Dimtolj = header.Dimtolj;
  style.Dimtzin = header.Dimtzin;
  style.Dimaltz = header.Dimaltz;
  style.Dimalttz = header.Dimalttz;
  style.Dimupt = header.Dimupt;
  style.Dimatfit = header.Dimatfit;
  style.Dimlwd = header.Dimlwd;
  style.Dimlwe = header.Dimlwe;
  style.Dimtxsty = header.Textstyle;
  return style;
}

auto ActiveVport(Tables::Record record) -> Tables::VPort
{
  Tables::VPort vport;
  vport.Record = std::move(record);
  vport.ViewCenter = Vector{.X = A4Width / 2, .Y = A4Height / 2, .Z = 0};
  vport.ViewHeight = A4Height;
  vport.ViewWidth = A4Width;
  return vport;
}

auto BlockOf(Tables::Record record, Handle block, Handle end) -> Tables::BlockRecord
{
  Tables::BlockRecord made;
  made.Record = std::move(record);
  made.Block = block;
  made.EndBlk = end;
  return made;
}

auto MakeBlockCommon(Space space, Handle layer) -> Common
{
  Common common;
  common.Space = space;
  common.Lineweight = Lineweight::ByLayer;
  common.Layer = layer;
  return common;
}

auto MakeBlock(const Tables::BlockRecord &record, Space space, Handle layer) -> Entities::Block
{
  return Entities::Block{.Base = MakeBase(record.Block, record.Record.Base.Self),
                         .Common = MakeBlockCommon(space, layer),
                         .Name = record.Record.Name};
}

auto MakeEndBlk(const Tables::BlockRecord &record, Space space, Handle layer) -> Entities::EndBlk
{
  return Entities::EndBlk{.Base = MakeBase(record.EndBlk, record.Record.Base.Self),
                          .Common = MakeBlockCommon(space, layer)};
}

auto DictionaryOf(Handle self, Handle owner, std::vector<Objects::Entry> entries) -> Objects::Dictionary
{
  return Objects::Dictionary{.Base = owner ? MakeReactingBase(self, owner) : MakeBase(self, owner),
                             .HardOwner = false,
                             .Cloning = 1,
                             .Entries = std::move(entries),
                             .Default = {}};
}

auto ModelPlot() -> Objects::Plot
{
  Objects::Plot plot;
  plot.Device = "none_device";
  plot.Size = "ISO_A4_(210.00_x_297.00_MM)";
  plot.Left = A4Side;
  plot.Bottom = A4End;
  plot.Right = A4Side;
  plot.Top = A4End;
  plot.Width = A4Width;
  plot.Height = A4Height;
  plot.Flags = 688 + ModelType;
  plot.Units = Millimetres;
  plot.Type = 0;
  return plot;
}

auto LayoutOf(Base base, std::string name, Handle block) -> Objects::Layout
{
  Objects::Layout layout;
  layout.Base = std::move(base);
  layout.Name = std::move(name);
  layout.Block = block;
  return layout;
}

auto StandardMline(Base base) -> Objects::MLineStyle
{
  Objects::MLineStyle mline;
  mline.Base = std::move(base);
  mline.Name = "STANDARD";
  mline.Elements = {
      Objects::MLineStyles::Element{.Offset = Half, .Color = {}, .Linetype = Objects::MLineStyles::Element{}.Linetype},
      Objects::MLineStyles::Element{
          .Offset = -Half, .Color = {}, .Linetype = Objects::MLineStyles::Element{}.Linetype}};
  return mline;
}

auto ClassOf(int number, std::string cpp, std::string dxf) -> Class
{
  return Class{.Number = number,
               .ProxyFlags = 0,
               .Application = "ObjectDBX Classes",
               .CppName = std::move(cpp),
               .DxfName = std::move(dxf),
               .WasZombie = false,
               .ItemClass = ItemClass,
               .Instances = 0,
               .DwgVersion = 0,
               .Maintenance = 0,
               .Unknown1 = 0,
               .Unknown2 = 0};
}

auto SpaceOf(const Drawing &drawing, Handle record) -> Space
{
  if (record == drawing.Header.BlockRecordModelSpace)
  {
    return Space::Model;
  }
  return record == drawing.Header.BlockRecordPaperSpace ? Space::Paper : Space::Owner;
}

auto ControlIn(Drawing &drawing, Tables::Table table) -> Tables::Control *
{
  for (Object &object : drawing.Objects)
  {
    if (auto *control = GetIf<Tables::Control>(&object); control != nullptr && control->Table == table)
    {
      return control;
    }
  }
  return nullptr;
}

auto TableIn(const Object &object) -> std::optional<Tables::Table>
{
  return Visit(
      [](const auto &kind) -> std::optional<Tables::Table>
      {
        if constexpr (requires { kind.Record.Name; })
        {
          return View::TableOf<std::remove_cvref_t<decltype(kind)>>;
        }
        else
        {
          return std::nullopt;
        }
      },
      object);
}

auto NameIn(const Object &object) -> std::string_view
{
  return Visit(
      [](const auto &kind) -> std::string_view
      {
        if constexpr (requires { kind.Record.Name; })
        {
          return kind.Record.Name;
        }
        else
        {
          return {};
        }
      },
      object);
}
} // namespace DwgIO::Making
