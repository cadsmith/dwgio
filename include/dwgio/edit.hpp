/***********************************************************************************************************************
 *  @file      dwgio/edit.hpp
 *  @brief     Additions to a drawing and removals from it that keep every link right.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_HPP
#define DWGIO_EDIT_HPP

#include "dwgio/api.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/space.hpp"
#include "dwgio/tables/block.hpp"

#include <expected>
#include <string_view>
#include <type_traits>
#include <utility>

/**
 *  An addition sets what the file needs and nothing a caller gave: a handle, a layer or a text's style only when null,
 *  the space from the block, the links always. A refused edit leaves the drawing as it was. Each function walks the
 *  drawing, as a single edit needs; an `Editor` makes the same edits for a run of them, as building a large drawing
 *  needs.
 */
namespace DwgIO::Edit
{
/**
 *  The handle seed, above every handle the drawing holds; the seed moves past it.
 */
[[nodiscard]] DWGIO_API auto MakeHandle(Drawing &drawing) -> Handle;

/**
 *  Appends an entity to a space, last in its block's chain.
 *
 *  An ATTDEF marks its block as holding attribute definitions; from R2000 an INSERT is listed in the record of the
 *  block it places.
 *
 *  @return The entity's handle; a failure for an object that is no entity, the owner space, or a drawing without that
 *          space.
 */
[[nodiscard]] DWGIO_API auto Add(Drawing &drawing, Object entity, Space space = Space::Model)
    -> std::expected<Handle, Failure>;

/**
 *  Appends an entity to the named block, matched in any case, last in its chain, as the Add above does.
 *
 *  @return The entity's handle; a failure for an object that is no entity, or a block the drawing lacks.
 */
[[nodiscard]] DWGIO_API auto Add(Drawing &drawing, Object entity, std::string_view block)
    -> std::expected<Handle, Failure>;

/**
 *  Appends an attribute to an insert, after the ones it has.
 *
 *  The attribute takes a handle when it has none, the insert as owner, the insert's layer when it names none, and
 *  $TEXTSTYLE when it has no style; the insert takes the SEQEND that closes its attributes when it has none, and then
 *  says attributes follow it.
 *
 *  @return The attribute's handle; a failure for a handle that names no INSERT or MINSERT, or a handle the attribute
 *          gives that is in use.
 */
[[nodiscard]] DWGIO_API auto AddAttribute(Drawing &drawing, Handle insert, Entities::Attrib attribute)
    -> std::expected<Handle, Failure>;

/**
 *  Appends a vertex to a POLYLINE, after the ones it has, as AddAttribute appends an attribute to an insert.
 *
 *  A vertex that gives none of the type bits 32, 64 and 128 takes its polyline's: 32 for a 3D polyline, 64 for a
 *  polygon mesh, 192 for a polyface mesh's vertex. A mesh's Rows and Columns are the caller's to keep.
 *
 *  @return The vertex's handle; a failure for a handle that names no POLYLINE, type bits that do not fit it, or a
 *          handle the vertex gives that is in use.
 */
[[nodiscard]] DWGIO_API auto AddVertex(Drawing &drawing, Handle polyline, Entities::Vertex vertex)
    -> std::expected<Handle, Failure>;

/**
 *  Makes a block record with its BLOCK and ENDBLK and lists it in the block table.
 *
 *  @return The record's handle; a failure for a name a block already bears, in any case, or a drawing without a block
 *          table.
 */
[[nodiscard]] DWGIO_API auto AddBlock(Drawing &drawing, std::string_view name) -> std::expected<Handle, Failure>;

/**
 *  Renames a table record or a block, and a block's BLOCK with it; whatever names the record does so by its handle and
 *  follows.
 *
 *  @return A failure for a handle that names no record, a record the drawing requires (layer 0, the ByBlock, ByLayer
 *          and Continuous linetypes, the Standard styles, the ACAD application, *Active, a space), an anonymous block,
 *          a record from an external reference, a name that is empty, starts with * or holds one of <>/\":;?*|,=`, or a
 *          name another record of the table bears, in any case.
 */
[[nodiscard]] DWGIO_API auto Rename(Drawing &drawing, Handle record, std::string_view name)
    -> std::expected<void, Failure>;

/**
 *  Removes an object and all it owns, as AutoCAD's ERASE and PURGE do: an entity with its attributes, vertices and
 *  extension dictionary; a block with its BLOCK, ENDBLK and entities; a table record; a named object.
 *
 *  What named it drops it: chains and lists, dictionaries, groups, draw orders, reactors. A dimension association bound
 *  to it is removed too, and a hatch bound to it loses its boundary.
 *
 *  @return A failure for an unknown handle, a table control, the root dictionary, a space, layer 0, the ByBlock,
 *          ByLayer and Continuous linetypes, the Standard styles, the ACAD application, *Active, a block frame or
 *          sequence member apart from its owner, and whatever is still in use, as a layer an entity is on.
 */
[[nodiscard]] DWGIO_API auto Remove(Drawing &drawing, Handle object) -> std::expected<void, Failure>;

/**
 *  Appends a table record and lists it in its control, giving it a handle when it has none.
 *
 *  A block record is made through AddBlock.
 *
 *  @return The record's handle; a failure for an object that is no table record, a block record, a name a record of
 *          that table already bears, in any case, or a drawing without that table.
 */
[[nodiscard]] DWGIO_API auto AddRecord(Drawing &drawing, Object record) -> std::expected<Handle, Failure>;

/**
 *  As AddRecord.
 */
template <class RecordT>
  requires requires(RecordT record) { record.Record.Name; } && (!std::is_same_v<RecordT, Tables::BlockRecord>)
[[nodiscard]] auto Add(Drawing &drawing, RecordT record) -> std::expected<Handle, Failure>
{
  return AddRecord(drawing, Object{std::move(record)});
}
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_HPP */
