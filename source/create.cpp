/***********************************************************************************************************************
 *  @file      source/create.cpp
 *  @brief     A new drawing holding what AutoCAD's own new drawing holds.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/create.hpp"

#include "making/records.hpp"
#include "releases/header.hpp"
#include "releases/object.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/objects/mlinestyle.hpp"
#include "dwgio/objects/placeholder.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace DwgIO
{
namespace
{
using Tables::Table;
using namespace Making;

// The handles, numbered as AutoCAD numbers the same objects in its own new drawing, the rest following on.
constexpr Handle BlockControl{.Value = 0x1};
constexpr Handle LayerControl{.Value = 0x2};
constexpr Handle StyleControl{.Value = 0x3};
constexpr Handle LtypeControl{.Value = 0x5};
constexpr Handle ViewControl{.Value = 0x6};
constexpr Handle UcsControl{.Value = 0x7};
constexpr Handle VportControl{.Value = 0x8};
constexpr Handle AppidControl{.Value = 0x9};
constexpr Handle DimstyleControl{.Value = 0xA};
constexpr Handle Root{.Value = 0xC};
constexpr Handle Groups{.Value = 0xD};
constexpr Handle PlotStyles{.Value = 0xE};
constexpr Handle Normal{.Value = 0xF};
constexpr Handle Layer0{.Value = 0x10};
constexpr Handle Standard{.Value = 0x11};
constexpr Handle Acad{.Value = 0x12};
constexpr Handle ByBlock{.Value = 0x14};
constexpr Handle ByLayer{.Value = 0x15};
constexpr Handle Continuous{.Value = 0x16};
constexpr Handle MlineStyles{.Value = 0x17};
constexpr Handle StandardMlineStyle{.Value = 0x18};
constexpr Handle PlotSettings{.Value = 0x19};
constexpr Handle Layouts{.Value = 0x1A};
constexpr Handle ModelRecord{.Value = 0x1F};
constexpr Handle ModelBlock{.Value = 0x20};
constexpr Handle ModelEnd{.Value = 0x21};
constexpr Handle ModelLayout{.Value = 0x22};
constexpr Handle PaperRecord{.Value = 0x23};
constexpr Handle PaperBlock{.Value = 0x24};
constexpr Handle PaperEnd{.Value = 0x25};
constexpr Handle PaperLayout{.Value = 0x26};
constexpr Handle Iso25{.Value = 0x27};
constexpr Handle StandardDim{.Value = 0x28};
constexpr Handle Active{.Value = 0x29};
constexpr Handle ViewportHeaders{.Value = 0x2A};
constexpr Handle Seed{.Value = 0x2B};

constexpr int FirstClass = 500;

auto MakeControl(Table table, Handle self, std::vector<Handle> entries) -> Tables::Control
{
  Tables::Control control = ControlOf(table, self);
  control.Entries = std::move(entries);
  return control;
}

void SetRoots(Header &header)
{
  header.Handseed = Seed;
  header.Clayer = Layer0;
  header.Textstyle = Standard;
  header.Celtype = ByLayer;
  header.Dimstyle = Iso25;
  header.Cmlstyle = StandardMlineStyle;
  header.Dimtxsty = Standard;
  header.BlockControlObject = BlockControl;
  header.LayerControlObject = LayerControl;
  header.StyleControlObject = StyleControl;
  header.LTypeControlObject = LtypeControl;
  header.ViewControlObject = ViewControl;
  header.UcsControlObject = UcsControl;
  header.VPortControlObject = VportControl;
  header.AppIdControlObject = AppidControl;
  header.DimStyleControlObject = DimstyleControl;
  header.ViewportEntityHeaderControlObject = ViewportHeaders;
  header.DictionaryAcadGroup = Groups;
  header.DictionaryAcadMLineStyle = MlineStyles;
  header.DictionaryNamedObjects = Root;
  header.DictionaryLayouts = Layouts;
  header.DictionaryPlotSettings = PlotSettings;
  header.DictionaryPlotStyles = PlotStyles;
  header.BlockRecordPaperSpace = PaperRecord;
  header.BlockRecordModelSpace = ModelRecord;
  header.LTypeByLayer = ByLayer;
  header.LTypeByBlock = ByBlock;
  header.LTypeContinuous = Continuous;
}
} // namespace

auto Create(Version release) -> Drawing
{
  Drawing drawing;
  drawing.Release = Version::R2000;
  drawing.Header = DefaultsOf(Version::R2000);
  SetRoots(drawing.Header);
  drawing.Classes = {ClassOf(FirstClass, "AcDbDictionaryWithDefault", "ACDBDICTIONARYWDFLT"),
                     ClassOf(FirstClass + 1, "AcDbPlaceHolder", "ACDBPLACEHOLDER"),
                     ClassOf(FirstClass + 2, "AcDbLayout", "LAYOUT")};
  std::vector<Object> &objects = drawing.Objects;

  Tables::Control blocks = ControlOf(Table::Block, BlockControl);
  blocks.ModelSpace = ModelRecord;
  blocks.PaperSpace = PaperRecord;
  objects.emplace_back(std::move(blocks));
  objects.emplace_back(MakeControl(Table::Layer, LayerControl, {Layer0}));
  objects.emplace_back(MakeControl(Table::Style, StyleControl, {Standard}));
  Tables::Control ltypes = MakeControl(Table::LType, LtypeControl, {Continuous});
  ltypes.ByBlock = ByBlock;
  ltypes.ByLayer = ByLayer;
  objects.emplace_back(std::move(ltypes));
  objects.emplace_back(ControlOf(Table::View, ViewControl));
  objects.emplace_back(ControlOf(Table::Ucs, UcsControl));
  objects.emplace_back(MakeControl(Table::VPort, VportControl, {Active}));
  objects.emplace_back(MakeControl(Table::AppId, AppidControl, {Acad}));
  objects.emplace_back(MakeControl(Table::DimStyle, DimstyleControl, {StandardDim, Iso25}));
  objects.emplace_back(ControlOf(Table::ViewportEntityHeader, ViewportHeaders));

  Tables::BlockRecord model = BlockOf(MakeRecord(ModelRecord, BlockControl, "*Model_Space"), ModelBlock, ModelEnd);
  model.Layout = ModelLayout;
  Tables::BlockRecord paper = BlockOf(MakeRecord(PaperRecord, BlockControl, "*Paper_Space"), PaperBlock, PaperEnd);
  paper.Layout = PaperLayout;
  objects.emplace_back(model);
  objects.emplace_back(MakeBlock(model, Space::Model, Layer0));
  objects.emplace_back(MakeEndBlk(model, Space::Model, Layer0));
  objects.emplace_back(paper);
  objects.emplace_back(MakeBlock(paper, Space::Paper, Layer0));
  objects.emplace_back(MakeEndBlk(paper, Space::Paper, Layer0));

  objects.emplace_back(LayerZero(MakeRecord(Layer0, LayerControl, "0"), Continuous, Normal));
  objects.emplace_back(StandardStyle(MakeRecord(Standard, StyleControl, "Standard")));
  objects.emplace_back(LtypeOf(MakeRecord(ByBlock, LtypeControl, "ByBlock"), ""));
  objects.emplace_back(LtypeOf(MakeRecord(ByLayer, LtypeControl, "ByLayer"), ""));
  objects.emplace_back(LtypeOf(MakeRecord(Continuous, LtypeControl, "Continuous"), "Solid line"));
  objects.emplace_back(ActiveVport(MakeRecord(Active, VportControl, "*Active")));
  objects.emplace_back(Tables::AppId{.Record = MakeRecord(Acad, AppidControl, "ACAD"), .Unknown1 = 0});
  objects.emplace_back(StandardDimstyle(MakeRecord(StandardDim, DimstyleControl, "Standard"), Standard));
  objects.emplace_back(MakeDimStyle(drawing.Header, MakeRecord(Iso25, DimstyleControl, "ISO-25")));

  objects.emplace_back(DictionaryOf(Root, {},
                                    {Objects::Entry{.Name = "ACAD_GROUP", .Object = Groups},
                                     Objects::Entry{.Name = "ACAD_LAYOUT", .Object = Layouts},
                                     Objects::Entry{.Name = "ACAD_MLINESTYLE", .Object = MlineStyles},
                                     Objects::Entry{.Name = "ACAD_PLOTSETTINGS", .Object = PlotSettings},
                                     Objects::Entry{.Name = "ACAD_PLOTSTYLENAME", .Object = PlotStyles}}));
  objects.emplace_back(DictionaryOf(Groups, Root, {}));
  objects.emplace_back(DictionaryOf(Layouts, Root,
                                    {Objects::Entry{.Name = "Layout1", .Object = PaperLayout},
                                     Objects::Entry{.Name = "Model", .Object = ModelLayout}}));
  objects.emplace_back(
      DictionaryOf(MlineStyles, Root, {Objects::Entry{.Name = "Standard", .Object = StandardMlineStyle}}));
  objects.emplace_back(DictionaryOf(PlotSettings, Root, {}));
  Objects::Dictionary styles = DictionaryOf(PlotStyles, Root, {Objects::Entry{.Name = "Normal", .Object = Normal}});
  styles.Default = Normal;
  objects.emplace_back(std::move(styles));
  objects.emplace_back(Objects::Placeholder{.Base = MakeReactingBase(Normal, PlotStyles)});

  Objects::Layout modelLayout = LayoutOf(MakeReactingBase(ModelLayout, Layouts), "Model", ModelRecord);
  modelLayout.Plot = ModelPlot();
  modelLayout.Viewport = Active;
  objects.emplace_back(std::move(modelLayout));
  Objects::Layout paperLayout = LayoutOf(MakeReactingBase(PaperLayout, Layouts), "Layout1", PaperRecord);
  paperLayout.Plot.Device = "none_device";
  paperLayout.Order = 1;
  objects.emplace_back(std::move(paperLayout));

  objects.emplace_back(StandardMline(MakeReactingBase(StandardMlineStyle, MlineStyles)));
  if (release == Version::R2000)
  {
    return drawing;
  }
  // Made in R2000's terms, then translated as a save would, each object against the drawing as made: a block's list
  // comes from the chain its entities still carry there.
  Drawing made = drawing;
  made.Header = Releases::ConvertHeader(drawing.Header, drawing, release).Header;
  for (std::size_t at = 0; at < objects.size(); ++at)
  {
    made.Objects[at] = Releases::ConvertObject(objects[at], drawing, release).Object;
  }
  if (release >= Version::R2004)
  {
    // R2004 and later keep no viewport entity headers.
    std::erase_if(made.Objects, [](const Object &object) -> bool { return BaseOf(object).Self == ViewportHeaders; });
  }
  made.Release = release;
  made.Maintenance = MaintenanceOf(release);
  return made;
}
} // namespace DwgIO
