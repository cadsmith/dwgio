/***********************************************************************************************************************
 *  @file      source/making/records.hpp
 *  @brief     The objects AutoCAD's new drawing holds, made one at a time.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_MAKING_RECORDS_HPP
#define DWGIO_MAKING_RECORDS_HPP

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

#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 *  The objects AutoCAD's new drawing holds, made one at a time: Create makes them all, Repair the ones a drawing lacks.
 */
namespace DwgIO::Making
{
[[nodiscard]] auto MakeBase(Handle self, Handle owner) -> Base;
/**
 *  A base whose owner is also its persistent reactor, as AutoCAD keeps a dictionary's entries.
 */
[[nodiscard]] auto MakeReactingBase(Handle self, Handle owner) -> Base;
/**
 *  A referenced record of a name, owned by its control.
 */
[[nodiscard]] auto MakeRecord(Handle self, Handle owner, std::string name) -> Tables::Record;
[[nodiscard]] auto ControlOf(Tables::Table table, Handle self) -> Tables::Control;
/**
 *  The layer 0 of a new drawing, on a linetype and a plot style.
 */
[[nodiscard]] auto LayerZero(Tables::Record record, Handle linetype, Handle style) -> Tables::Layer;
/**
 *  The text style Standard, on Arial.
 */
[[nodiscard]] auto StandardStyle(Tables::Record record) -> Tables::Style;
/**
 *  A linetype of no dashes, described.
 */
[[nodiscard]] auto LtypeOf(Tables::Record record, std::string description) -> Tables::LType;
/**
 *  The dimension style Standard, lettered in a text style.
 */
[[nodiscard]] auto StandardDimstyle(Tables::Record record, Handle textstyle) -> Tables::DimStyle;
/**
 *  The dimension style the header's dimension variables describe, lettered in the header's text style.
 */
[[nodiscard]] auto MakeDimStyle(const Header &header, Tables::Record record) -> Tables::DimStyle;
/**
 *  The viewport *Active, looking at the whole of the A4 limits.
 */
[[nodiscard]] auto ActiveVport(Tables::Record record) -> Tables::VPort;
/**
 *  A block record opened by a BLOCK and closed by an ENDBLK, its layout the caller's.
 */
[[nodiscard]] auto BlockOf(Tables::Record record, Handle block, Handle end) -> Tables::BlockRecord;
/**
 *  The common data of a BLOCK or ENDBLK: a space, a layer, ByLayer lineweight.
 */
[[nodiscard]] auto MakeBlockCommon(Space space, Handle layer) -> Common;
/**
 *  The BLOCK of a record, in a space, on a layer.
 */
[[nodiscard]] auto MakeBlock(const Tables::BlockRecord &record, Space space, Handle layer) -> Entities::Block;
/**
 *  The ENDBLK of a record, in a space, on a layer.
 */
[[nodiscard]] auto MakeEndBlk(const Tables::BlockRecord &record, Space space, Handle layer) -> Entities::EndBlk;
/**
 *  A soft dictionary of entries; one owned by another dictionary reacts to it.
 */
[[nodiscard]] auto DictionaryOf(Handle self, Handle owner, std::vector<Objects::Entry> entries) -> Objects::Dictionary;
/**
 *  The plot settings of the model layout: A4 on no device, at 1:1.
 */
[[nodiscard]] auto ModelPlot() -> Objects::Plot;
[[nodiscard]] auto LayoutOf(Base base, std::string name, Handle block) -> Objects::Layout;
/**
 *  The multiline style STANDARD: two lines half a unit apart, ByLayer, capped square.
 */
[[nodiscard]] auto StandardMline(Base base) -> Objects::MLineStyle;
[[nodiscard]] auto ClassOf(int number, std::string cpp, std::string dxf) -> Class;

/**
 *  The space code of an entity owned by a block record: the record's space, or a block's.
 */
[[nodiscard]] auto SpaceOf(const Drawing &drawing, Handle record) -> Space;
/**
 *  Null when the drawing lacks it.
 */
[[nodiscard]] auto ControlIn(Drawing &drawing, Tables::Table table) -> Tables::Control *;
/**
 *  The table a record belongs to, or none for an object that is no table record.
 */
[[nodiscard]] auto TableIn(const Object &object) -> std::optional<Tables::Table>;
/**
 *  The name of a table record, or empty for an object that is none.
 */
[[nodiscard]] auto NameIn(const Object &object) -> std::string_view;
} // namespace DwgIO::Making

#endif /* DWGIO_MAKING_RECORDS_HPP */
