/***********************************************************************************************************************
 *  @file      source/audit/roots.cpp
 *  @brief     The audit of the tables' controls, the root dictionaries and the required records.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "audit/checks.hpp"
#include "audit/common.hpp"
#include "edit/add.hpp"
#include "making/records.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/header.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/tables/appid.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/dimstyle.hpp"
#include "dwgio/tables/layer.hpp"
#include "dwgio/tables/ltype.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Audit
{
namespace
{
using Tables::Table;

/**
 *  R2004 and later have no viewport entity headers.
 */
auto IsTableDropped(const Drawing &drawing, Tables::Table table) -> bool
{
  return table == Tables::Table::ViewportEntityHeader && drawing.Release >= Version::R2004;
}

/**
 *  Every table has its control, and the header names it.
 */
void CheckControls(Drawing &drawing, Report &report)
{
  for (const Root &root : Controls)
  {
    if (IsTableDropped(drawing, root.Table))
    {
      continue;
    }
    const std::string_view word = Edit::WordOf(root.Table);
    Handle &named = drawing.Header.*root.Member;
    if (Making::ControlIn(drawing, root.Table) == nullptr)
    {
      // AutoCAD opens an R2000 drawing without its viewport entity header table and refuses one without any other.
      if (root.Table == Tables::Table::ViewportEntityHeader)
      {
        NoteWarning(report, std::format("{} table missing", word), {});
      }
      else
      {
        NoteFailure(report, std::format("{} table missing", word), {});
      }
      named = ReserveHandle(drawing, named);
      drawing.Objects.emplace_back(Making::ControlOf(root.Table, named));
    }
    const Handle self = Making::ControlIn(drawing, root.Table)->Base.Self;
    if (named != self)
    {
      NoteFailure(report, std::format("{} table not the header's", word), self);
      named = self;
    }
  }
}

struct Keyed
{
  std::string_view Name;
  Handle Header::*Member;
  /**
   *  Whether the dictionary came with R2000; a release before has none and no header member for it.
   */
  bool Modern;
};

constexpr std::array<Keyed, 5> Dictionaries{
    {{.Name = "ACAD_GROUP", .Member = &Header::DictionaryAcadGroup, .Modern = false},
     {.Name = "ACAD_LAYOUT", .Member = &Header::DictionaryLayouts, .Modern = true},
     {.Name = "ACAD_MLINESTYLE", .Member = &Header::DictionaryAcadMLineStyle, .Modern = false},
     {.Name = "ACAD_PLOTSETTINGS", .Member = &Header::DictionaryPlotSettings, .Modern = true},
     {.Name = "ACAD_PLOTSTYLENAME", .Member = &Header::DictionaryPlotStyles, .Modern = true}}};

/**
 *  The root dictionary and the five the header names are there and entered; no entry names nothing.
 */
void CheckDictionaries(Drawing &drawing, Report &report)
{
  Header &header = drawing.Header;
  if (DictionaryAt(drawing, header.DictionaryNamedObjects) == nullptr)
  {
    NoteFailure(report, "dictionary of named objects missing", {});
    const Handle self = ReserveHandle(drawing, header.DictionaryNamedObjects);
    drawing.Objects.emplace_back(Making::DictionaryOf(self, {}, {}));
    header.DictionaryNamedObjects = self;
  }
  const Handle root = header.DictionaryNamedObjects;
  const bool modern = drawing.Release >= Version::R2000;
  for (const Keyed &wanted : Dictionaries)
  {
    if (wanted.Modern && !modern)
    {
      continue;
    }
    Handle &named = header.*wanted.Member;
    const Object *object = View::Entry(drawing, *DictionaryAt(drawing, root), wanted.Name);
    const auto *entered = object == nullptr ? nullptr : GetIf<Objects::Dictionary>(object);
    Handle self = entered == nullptr ? Handle{} : entered->Base.Self;
    if (!self)
    {
      NoteWarning(report, std::format("dictionary {} missing", wanted.Name), root);
      if (DictionaryAt(drawing, named) != nullptr)
      {
        self = named;
      }
      else
      {
        self = ReserveHandle(drawing, named);
        drawing.Objects.emplace_back(Making::DictionaryOf(self, root, {}));
      }
      SetEntry(*DictionaryAt(drawing, root), wanted.Name, self);
    }
    if (named != self)
    {
      NoteFailure(report, std::format("dictionary {} not the header's", wanted.Name), self);
      named = self;
    }
  }
}

/**
 *  No entry and no default of any dictionary names nothing.
 */
void CheckDangling(Drawing &drawing, Report &report)
{
  const Index index{drawing};
  for (Object &object : drawing.Objects)
  {
    auto *dictionary = GetIf<Objects::Dictionary>(&object);
    if (dictionary == nullptr)
    {
      continue;
    }
    const Handle self = dictionary->Base.Self;
    std::erase_if(dictionary->Entries,
                  [&](const Objects::Entry &entry) -> bool
                  {
                    if (index.At(entry.Object) != nullptr)
                    {
                      return false;
                    }
                    NoteWarning(report, std::format("entry {} of dictionary {} names nothing", entry.Name, HexOf(self)),
                                self);
                    return true;
                  });
    if (dictionary->Default && index.At(*dictionary->Default) == nullptr)
    {
      NoteWarning(report, std::format("default of dictionary {} names nothing", HexOf(self)), self);
      dictionary->Default.reset();
    }
  }
}

/**
 *  Drops the entries of every control that name no record of its table, or that repeat one, and gives every listed
 *  record its table as owner.
 */
void CheckEntries(Drawing &drawing, Report &report)
{
  for (const Root &root : Controls)
  {
    if (IsTableDropped(drawing, root.Table))
    {
      continue;
    }
    Tables::Control &control = *Making::ControlIn(drawing, root.Table);
    const std::string_view word = Edit::WordOf(root.Table);
    std::erase_if(control.Entries,
                  [&](Handle entry) -> bool
                  {
                    // AutoCAD itself writes null entries in a view table; they are left as they are.
                    const Object *object = Find(drawing, entry);
                    if (!entry || (object != nullptr && Making::TableIn(*object) == root.Table))
                    {
                      return false;
                    }
                    NoteFailure(report, std::format("entry {} of the {} table is no {}", HexOf(entry), word, word),
                                control.Base.Self);
                    return true;
                  });
    std::unordered_set<std::uint64_t> listed;
    std::erase_if(control.Entries,
                  [&](Handle entry) -> bool
                  {
                    if (!entry || listed.insert(entry.Value).second)
                    {
                      return false;
                    }
                    // AutoCAD's audit fixes a record listed twice as an error.
                    NoteFailure(report, std::format("{} {} listed twice", word, Making::NameIn(*Find(drawing, entry))),
                                entry);
                    return true;
                  });
    for (const Handle entry : control.Entries)
    {
      if (!entry)
      {
        continue;
      }
      Object &object = *Find(drawing, entry);
      if (BaseOf(object).Owner != control.Base.Self)
      {
        NoteFailure(report, std::format("owner of {} {} is not its table", word, Making::NameIn(object)), entry);
        BaseOf(object).Owner = control.Base.Self;
      }
    }
  }
}

/**
 *  Lists a made record in its table and appends it.
 */
void PlaceRecord(Drawing &drawing, Table table, Object record)
{
  Tables::Control &control = *Making::ControlIn(drawing, table);
  BaseOf(record).Owner = control.Base.Self;
  control.Entries.push_back(BaseOf(record).Self);
  drawing.Objects.push_back(std::move(record));
}

/**
 *  The BLOCK or ENDBLK a block record owns already, or null.
 */
template <class EntityT>
auto FindFrame(const Drawing &drawing, Handle record) -> Handle
{
  for (const Object &object : drawing.Objects)
  {
    if (const auto *entity = GetIf<EntityT>(&object); entity != nullptr && entity->Base.Owner == record)
    {
      return entity->Base.Self;
    }
  }
  return {};
}

/**
 *  The layout of a block record, or null.
 */
auto LayoutOf(const Drawing &drawing, Handle record) -> Handle
{
  for (const Object &object : drawing.Objects)
  {
    if (const auto *layout = GetIf<Objects::Layout>(&object); layout != nullptr && layout->Block == record)
    {
      return layout->Base.Self;
    }
  }
  return {};
}

/**
 *  Makes a space: its record under the handle, its BLOCK, ENDBLK and layout where the drawing has none.
 */
void MakeSpace(Drawing &drawing, Handle self, bool model)
{
  const Handle owner = drawing.Header.BlockControlObject;
  const std::string_view name = model ? "*Model_Space" : "*Paper_Space";
  const Handle block = FindFrame<Entities::Block>(drawing, self);
  const Handle end = FindFrame<Entities::EndBlk>(drawing, self);
  Tables::BlockRecord record =
      Making::BlockOf(Making::MakeRecord(self, owner, std::string{name}), block ? block : Edit::MakeHandle(drawing),
                      end ? end : Edit::MakeHandle(drawing));
  const Space space = model ? Space::Model : Space::Paper;
  if (!block)
  {
    drawing.Objects.emplace_back(Making::MakeBlock(record, space, drawing.Header.Clayer));
  }
  if (!end)
  {
    drawing.Objects.emplace_back(Making::MakeEndBlk(record, space, drawing.Header.Clayer));
  }
  record.Layout = LayoutOf(drawing, self);
  if (!record.Layout && drawing.Release >= Version::R2000) // A release before R2000 has no layouts.
  {
    record.Layout = MakeLayout(drawing, self, model);
  }
  Tables::Control &control = *Making::ControlIn(drawing, Table::Block);
  (model ? control.ModelSpace : control.PaperSpace) = self;
  drawing.Objects.emplace_back(std::move(record));
  auto *made = GetIf<Tables::BlockRecord>(Find(drawing, self));
  Relink(Chain{.Owner = self,
               .First = &made->First,
               .Last = &made->Last,
               .SeqEnd = nullptr,
               .List = drawing.Release >= Version::R2004 ? &made->Entities : nullptr},
         CollectOwned(drawing, self));
}

struct Need
{
  Tables::Table Table;
  std::string_view Name;
  Handle Header::*Member;
  void (*Make)(Drawing &, Handle);
};

constexpr std::array<Need, 9> Needs{
    {{.Table = Table::LType,
      .Name = "ByBlock",
      .Member = &Header::LTypeByBlock,
      .Make = [](Drawing &drawing, Handle self) -> void
      { PlaceRecord(drawing, Table::LType, Making::LtypeOf(Making::MakeRecord(self, {}, "ByBlock"), "")); }},
     {.Table = Table::LType,
      .Name = "ByLayer",
      .Member = &Header::LTypeByLayer,
      .Make = [](Drawing &drawing, Handle self) -> void
      { PlaceRecord(drawing, Table::LType, Making::LtypeOf(Making::MakeRecord(self, {}, "ByLayer"), "")); }},
     {.Table = Table::LType,
      .Name = "Continuous",
      .Member = &Header::LTypeContinuous,
      .Make = [](Drawing &drawing, Handle self) -> void
      {
        PlaceRecord(drawing, Table::LType, Making::LtypeOf(Making::MakeRecord(self, {}, "Continuous"), "Solid line"));
      }},
     {.Table = Table::Layer,
      .Name = "0",
      .Member = &Header::Clayer,
      .Make = [](Drawing &drawing, Handle self) -> void
      {
        const Objects::Dictionary *styles = View::Dictionary(drawing, "ACAD_PLOTSTYLENAME"); // None before R2000.
        const Handle style = styles == nullptr ? Handle{} : styles->Default.value_or(Handle{});
        PlaceRecord(
            drawing, Table::Layer,
            Making::LayerZero(Making::MakeRecord(self, {}, "0"), Present(drawing, Table::LType, "Continuous"), style));
      }},
     {.Table = Table::Style,
      .Name = "Standard",
      .Member = &Header::Textstyle,
      .Make = [](Drawing &drawing, Handle self) -> void
      { PlaceRecord(drawing, Table::Style, Making::StandardStyle(Making::MakeRecord(self, {}, "Standard"))); }},
     {.Table = Table::DimStyle,
      .Name = "Standard",
      .Member = &Header::Dimstyle,
      .Make = [](Drawing &drawing, Handle self) -> void
      {
        PlaceRecord(drawing, Table::DimStyle,
                    Making::StandardDimstyle(Making::MakeRecord(self, {}, "Standard"),
                                             Present(drawing, Table::Style, "Standard")));
      }},
     {.Table = Table::AppId,
      .Name = "ACAD",
      .Member = nullptr,
      .Make = [](Drawing &drawing, Handle self) -> void
      {
        PlaceRecord(drawing, Table::AppId,
                    Tables::AppId{.Record = Making::MakeRecord(self, {}, "ACAD"), .Unknown1 = 0});
      }},
     {.Table = Table::Block,
      .Name = "*Model_Space",
      .Member = &Header::BlockRecordModelSpace,
      .Make = [](Drawing &drawing, Handle self) -> void { MakeSpace(drawing, self, true); }},
     {.Table = Table::Block,
      .Name = "*Paper_Space",
      .Member = &Header::BlockRecordPaperSpace,
      .Make = [](Drawing &drawing, Handle self) -> void { MakeSpace(drawing, self, false); }}}};

/**
 *  What the header names among the records, and the record it falls back to.
 */
struct Reference
{
  Handle Header::*Member;
  Tables::Table Table;
  std::string_view Fallback;
  std::string_view What;
};

constexpr std::array<Reference, 10> References{
    {{.Member = &Header::Clayer, .Table = Table::Layer, .Fallback = "0", .What = "current layer"},
     {.Member = &Header::Textstyle, .Table = Table::Style, .Fallback = "Standard", .What = "current text style"},
     {.Member = &Header::Dimstyle, .Table = Table::DimStyle, .Fallback = "Standard", .What = "current dimension style"},
     {.Member = &Header::Celtype, .Table = Table::LType, .Fallback = "ByLayer", .What = "current linetype"},
     {.Member = &Header::Dimtxsty, .Table = Table::Style, .Fallback = "Standard", .What = "dimension text style"},
     {.Member = &Header::LTypeByBlock, .Table = Table::LType, .Fallback = "ByBlock", .What = "ByBlock linetype"},
     {.Member = &Header::LTypeByLayer, .Table = Table::LType, .Fallback = "ByLayer", .What = "ByLayer linetype"},
     {.Member = &Header::LTypeContinuous,
      .Table = Table::LType,
      .Fallback = "Continuous",
      .What = "Continuous linetype"},
     {.Member = &Header::BlockRecordModelSpace,
      .Table = Table::Block,
      .Fallback = "*Model_Space",
      .What = "model space"},
     {.Member = &Header::BlockRecordPaperSpace,
      .Table = Table::Block,
      .Fallback = "*Paper_Space",
      .What = "paper space"}}};

/**
 *  The records AutoCAD will not open a drawing without are there, and the header names records of the right kind.
 */
void CheckRequired(Drawing &drawing, Report &report)
{
  for (const Need &need : Needs)
  {
    if (!Present(drawing, need.Table, need.Name))
    {
      NoteFailure(report, std::format("{} {} missing", Edit::WordOf(need.Table), need.Name), {});
      const Handle kept = need.Member == nullptr ? Handle{} : drawing.Header.*need.Member;
      need.Make(drawing, ReserveHandle(drawing, kept));
    }
  }
  for (const Reference &reference : References)
  {
    Handle &named = drawing.Header.*reference.Member;
    const Object *object = Find(drawing, named);
    if (object == nullptr || Making::TableIn(*object) != reference.Table)
    {
      const std::string_view word = Edit::WordOf(reference.Table);
      NoteFailure(report, std::format("{} {} is no {}", reference.What, HexOf(named), word), named);
      named = Present(drawing, reference.Table, reference.Fallback);
    }
  }
}

/**
 *  Every record is listed in its table once, and the spaces and the ByBlock and ByLayer linetypes are the header's.
 */
void CheckListings(Drawing &drawing, Report &report)
{
  const Header &header = drawing.Header;
  Tables::Control &blocks = *Making::ControlIn(drawing, Table::Block);
  if (blocks.ModelSpace != header.BlockRecordModelSpace)
  {
    NoteFailure(report, "model space of the block table is not the header's", blocks.Base.Self);
    blocks.ModelSpace = header.BlockRecordModelSpace;
  }
  if (blocks.PaperSpace != header.BlockRecordPaperSpace)
  {
    NoteFailure(report, "paper space of the block table is not the header's", blocks.Base.Self);
    blocks.PaperSpace = header.BlockRecordPaperSpace;
  }
  Tables::Control &linetypes = *Making::ControlIn(drawing, Table::LType);
  if (linetypes.ByBlock != header.LTypeByBlock)
  {
    NoteFailure(report, "ByBlock of the linetype table is not the header's", linetypes.Base.Self);
    linetypes.ByBlock = header.LTypeByBlock;
  }
  if (linetypes.ByLayer != header.LTypeByLayer)
  {
    NoteFailure(report, "ByLayer of the linetype table is not the header's", linetypes.Base.Self);
    linetypes.ByLayer = header.LTypeByLayer;
  }
  for (const Root &root : Controls)
  {
    if (IsTableDropped(drawing, root.Table))
    {
      continue;
    }
    Tables::Control &control = *Making::ControlIn(drawing, root.Table);
    for (Object &object : drawing.Objects)
    {
      if (Making::TableIn(object) != root.Table)
      {
        continue;
      }
      const Handle self = BaseOf(object).Self;
      const bool apart = self == control.ModelSpace || self == control.PaperSpace || self == control.ByBlock ||
                         self == control.ByLayer;
      if (!apart && !std::ranges::contains(control.Entries, self))
      {
        NoteFailure(report, std::format("{} {} not listed", Edit::WordOf(root.Table), Making::NameIn(object)), self);
        control.Entries.push_back(self);
        BaseOf(object).Owner = control.Base.Self;
      }
    }
  }
}
} // namespace

void Roots(Drawing &drawing, Report &report)
{
  CheckControls(drawing, report);
  CheckDictionaries(drawing, report);
  CheckDangling(drawing, report);
  CheckEntries(drawing, report);
  CheckRequired(drawing, report);
  CheckListings(drawing, report);
}
} // namespace DwgIO::Audit
