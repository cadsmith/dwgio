/***********************************************************************************************************************
 *  @file      source/audit/references.cpp
 *  @brief     The audit of what each handle an object names must be.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "audit/checks.hpp"
#include "audit/common.hpp"
#include "audit/pointers.hpp"
#include "making/records.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/mline.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/version.hpp"
#include "dwgio/view.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace DwgIO::Audit
{
namespace
{
using Tables::Table;

/**
 *  The STANDARD multiline style, through the ACAD_MLINESTYLE dictionary, or null.
 */
auto StandardMline(const Drawing &drawing) -> Handle
{
  // The dictionary is there by now: the tables check made it when the drawing lacked it.
  const Object *standard = View::Entry(drawing, *View::Dictionary(drawing, "ACAD_MLINESTYLE"), "Standard");
  return standard != nullptr && Fits(*standard, Wanted::MlineStyle) ? BaseOf(*standard).Self : Handle{};
}

/**
 *  The Standard multileader style, through the ACAD_MLEADERSTYLE dictionary, or null.
 */
auto StandardMleader(const Drawing &drawing) -> Handle
{
  const Objects::Dictionary *styles = View::Dictionary(drawing, "ACAD_MLEADERSTYLE");
  const Object *standard = styles == nullptr ? nullptr : View::Entry(drawing, *styles, "Standard");
  return standard != nullptr && Fits(*standard, Wanted::MleaderStyle) ? BaseOf(*standard).Self : Handle{};
}

/**
 *  The Standard table style, through the ACAD_TABLESTYLE dictionary, or null.
 */
auto StandardTable(const Drawing &drawing) -> Handle
{
  const Objects::Dictionary *styles = View::Dictionary(drawing, "ACAD_TABLESTYLE");
  const Object *standard = styles == nullptr ? nullptr : View::Entry(drawing, *styles, "Standard");
  return standard != nullptr && Fits(*standard, Wanted::TableStyle) ? BaseOf(*standard).Self : Handle{};
}

/**
 *  What a reference of a kind falls back to when it names nothing of the kind; a plot style and an arrowhead fall back
 *  to none, a multileader style and a table style to Standard, and a block is never asked, its insert, layout,
 *  multileader or table being left out instead.
 */
auto Fallback(const Drawing &drawing, const Referring &referring) -> Handle
{
  if (referring.Kind == Wanted::Layer)
  {
    return Present(drawing, Table::Layer, "0");
  }
  if (referring.Kind == Wanted::Linetype)
  {
    return referring.Origin == nullptr ? Present(drawing, Table::LType, "Continuous") : Handle{};
  }
  if (referring.Kind == Wanted::TextStyle)
  {
    return Present(drawing, Table::Style, "Standard");
  }
  if (referring.Kind == Wanted::DimStyle)
  {
    return Present(drawing, Table::DimStyle, "Standard");
  }
  if (referring.Kind == Wanted::MleaderStyle)
  {
    return StandardMleader(drawing);
  }
  if (referring.Kind == Wanted::TableStyle)
  {
    return StandardTable(drawing);
  }
  return referring.Kind == Wanted::MlineStyle ? StandardMline(drawing) : Handle{};
}

/**
 *  Whether an entity shows what a reference names, and is left out without it: an image's definition, an underlay's.
 */
auto IsShown(Wanted wanted) -> bool
{
  return wanted == Wanted::ImageDef || wanted == Wanted::PdfDefinition || wanted == Wanted::DwfDefinition ||
         wanted == Wanted::DgnDefinition;
}

/**
 *  Every handle an object names is of the kind it wants; an insert or a layout of a block the drawing lacks is left
 *  out, and an image or underlay whose definition it lacks; a multiline whose style is gone takes STANDARD, made and
 *  entered when the drawing has none.
 */
void CheckNamed(Drawing &drawing, Report &report)
{
  std::unordered_set<std::uint64_t> removed;
  std::vector<Handle> unstyled;
  const Index index{drawing};
  for (Object &object : drawing.Objects)
  {
    const Handle self = BaseOf(object).Self;
    for (const Referring &referring : ReferringOf(object))
    {
      const Object *found = index.At(*referring.Target);
      if (found != nullptr && Fits(*found, referring.Kind))
      {
        continue;
      }
      const std::string what = std::format("{} {} of {} {} missing", WordOf(referring.Kind), HexOf(*referring.Target),
                                           TypeOf(object), HexOf(self));
      if (referring.Kind == Wanted::Block || IsShown(referring.Kind))
      {
        // AutoCAD refuses a drawing with an insert of a block it lacks, and makes a new layout for one.
        (Holds<Objects::Layout>(object) ? NoteWarning : NoteFailure)(report, what + ", left out", self);
        removed.insert(self.Value);
        break;
      }
      NoteFailure(report, what, self);
      *referring.Target = Fallback(drawing, referring);
      if (referring.Origin != nullptr)
      {
        *referring.Origin = Origin::ByLayer;
      }
      if (referring.Kind == Wanted::MlineStyle && !*referring.Target)
      {
        unstyled.push_back(self);
      }
    }
  }
  EraseObjects(drawing, removed);
  if (unstyled.empty())
  {
    return;
  }
  const Handle styles = View::Dictionary(drawing, "ACAD_MLINESTYLE")->Base.Self;
  const Handle standard = Edit::MakeHandle(drawing);
  SetEntry(*DictionaryAt(drawing, styles), "Standard", standard);
  drawing.Objects.emplace_back(Making::StandardMline(Making::MakeReactingBase(standard, styles)));
  for (const Handle self : unstyled)
  {
    GetIf<Entities::MLine>(Find(drawing, self))->Style = standard;
  }
}

/**
 *  What a block record lacks of its framing.
 */
struct Lack
{
  Handle Record;
  bool Begin;
  bool End;
  bool Layout;
};

template <class KindT>
auto Names(const Drawing &drawing, Handle handle) -> bool
{
  const Object *object = Find(drawing, handle);
  return object != nullptr && Holds<KindT>(*object);
}

/**
 *  What a block record lacks, noted: its BLOCK, its ENDBLK, and for a space its layout.
 *
 *  AutoCAD opens a drawing without them and makes them itself. A block's layout naming no layout is cleared.
 */
auto Lacks(const Drawing &drawing, Tables::BlockRecord &record, Report &report) -> Lack
{
  const Handle self = record.Record.Base.Self;
  const bool space = self == drawing.Header.BlockRecordModelSpace || self == drawing.Header.BlockRecordPaperSpace;
  const bool layout = Names<Objects::Layout>(drawing, record.Layout);
  const bool modern = drawing.Release >= Version::R2000; // A release before R2000 has no layouts.
  const Lack lack{.Record = self,
                  .Begin = !Names<Entities::Block>(drawing, record.Block),
                  .End = !Names<Entities::EndBlk>(drawing, record.EndBlk),
                  .Layout = modern && space && !layout};
  if (lack.Begin)
  {
    NoteWarning(report, std::format("BLOCK of block {} missing", record.Record.Name), self);
  }
  if (lack.End)
  {
    NoteWarning(report, std::format("ENDBLK of block {} missing", record.Record.Name), self);
  }
  if (lack.Layout)
  {
    NoteWarning(report, std::format("layout of block {} missing", record.Record.Name), self);
  }
  else if (!space && record.Layout && !layout)
  {
    NoteWarning(report, std::format("layout {} of block {} is no layout", HexOf(record.Layout), record.Record.Name),
                self);
    record.Layout = {};
  }
  return lack;
}

/**
 *  Makes what a block record lacks.
 */
void MakeMissing(Drawing &drawing, const Lack &lack)
{
  const Space space = Making::SpaceOf(drawing, lack.Record);
  const Handle layer = drawing.Header.Clayer;
  const auto record = [&drawing, &lack] -> Tables::BlockRecord &
  {
    return *GetIf<Tables::BlockRecord>(Find(drawing, lack.Record));
  };
  if (lack.Begin)
  {
    record().Block = Edit::MakeHandle(drawing);
    drawing.Objects.emplace_back(Making::MakeBlock(record(), space, layer));
  }
  if (lack.End)
  {
    record().EndBlk = Edit::MakeHandle(drawing);
    drawing.Objects.emplace_back(Making::MakeEndBlk(record(), space, layer));
  }
  if (lack.Layout)
  {
    const Handle made = MakeLayout(drawing, lack.Record, lack.Record == drawing.Header.BlockRecordModelSpace);
    record().Layout = made;
  }
}

/**
 *  Every block record has its BLOCK and ENDBLK, and a space its layout.
 */
void CheckFraming(Drawing &drawing, Report &report)
{
  std::vector<Lack> lacking;
  for (Object &object : drawing.Objects)
  {
    if (auto *record = GetIf<Tables::BlockRecord>(&object); record != nullptr)
    {
      const Lack lack = Lacks(drawing, *record, report);
      if (lack.Begin || lack.End || lack.Layout)
      {
        lacking.push_back(lack);
      }
    }
  }
  for (const Lack &lack : lacking)
  {
    MakeMissing(drawing, lack);
  }
}
} // namespace

void References(Drawing &drawing, Report &report)
{
  CheckNamed(drawing, report);
  CheckFraming(drawing, report);
}
} // namespace DwgIO::Audit
