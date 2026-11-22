/***********************************************************************************************************************
 *  @file      dwgio/edit/editor.hpp
 *  @brief     The edits of Edit for a run of them, in a time that does not grow with the drawing.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_EDITOR_HPP
#define DWGIO_EDIT_EDITOR_HPP

#include "dwgio/api.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/block.hpp"

#include <expected>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

namespace DwgIO::Edit
{
class CachedLedger;

/**
 *  The additions and removals of `Edit` for a run of edits on one drawing.
 *
 *  Each function of `Edit` walks the drawing for the handle seed and what it looks up, so a drawing built by them one
 *  entity at a time takes time growing with the square of its size. An editor walks it once, indexing its handles, its
 *  tables' controls and their records' names, keeps that index current through each edit it makes, and so adds in a
 *  time that does not grow with the drawing. Each addition sets, refuses and leaves the drawing as the function of
 *  `Edit` of its name does.
 *
 *  The drawing must outlive the editor and change only through it while the editor is used: a change made otherwise, to
 *  `Drawing::Objects` or to a record's name, needs a new editor.
 */
class DWGIO_API Editor
{
public:
  explicit Editor(Drawing &drawing);
  Editor(const Editor &) = delete;
  Editor(Editor &&) noexcept;
  auto operator=(const Editor &) -> Editor & = delete;
  auto operator=(Editor &&) noexcept -> Editor &;
  ~Editor();

  /**
   *  The handle seed, above every handle the drawing holds; the seed moves past it.
   */
  [[nodiscard]] auto MakeHandle() -> Handle;

  /**
   *  The first object with that handle in file order.
   *
   *  @return Null when none has it, and for the null handle; good until the next edit.
   */
  [[nodiscard]] auto Find(Handle self) const -> const Object *;

  /**
   *  As the const Find, to change the object; a change to a record's name goes through Rename.
   */
  [[nodiscard]] auto Find(Handle self) -> Object *;

  /**
   *  As `Edit::Add` to a space.
   */
  [[nodiscard]] auto Add(Object entity, Space space = Space::Model) -> std::expected<Handle, Failure>;

  /**
   *  As `Edit::Add` to a named block.
   */
  [[nodiscard]] auto Add(Object entity, std::string_view block) -> std::expected<Handle, Failure>;

  /**
   *  As `Edit::AddAttribute`.
   */
  [[nodiscard]] auto AddAttribute(Handle insert, Entities::Attrib attribute) -> std::expected<Handle, Failure>;

  /**
   *  As `Edit::AddVertex`.
   */
  [[nodiscard]] auto AddVertex(Handle polyline, Entities::Vertex vertex) -> std::expected<Handle, Failure>;

  /**
   *  As `Edit::AddBlock`.
   */
  [[nodiscard]] auto AddBlock(std::string_view name) -> std::expected<Handle, Failure>;

  /**
   *  As `Edit::AddRecord`.
   */
  [[nodiscard]] auto AddRecord(Object record) -> std::expected<Handle, Failure>;

  /**
   *  As `Edit::AddRecord`.
   */
  template <class RecordT>
    requires requires(RecordT record) { record.Record.Name; } && (!std::is_same_v<RecordT, Tables::BlockRecord>)
  [[nodiscard]] auto Add(RecordT record) -> std::expected<Handle, Failure>
  {
    return AddRecord(Object{std::move(record)});
  }

  /**
   *  As `Edit::Rename`, which walks the drawing; the editor indexes it again after a rename.
   */
  [[nodiscard]] auto Rename(Handle record, std::string_view name) -> std::expected<void, Failure>;

  /**
   *  As `Edit::Remove`, which walks the drawing; the editor indexes it again after a removal.
   */
  [[nodiscard]] auto Remove(Handle object) -> std::expected<void, Failure>;

private:
  std::unique_ptr<CachedLedger> m_Index;
};
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_EDITOR_HPP */
