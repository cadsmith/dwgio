/***********************************************************************************************************************
 *  @file      source/edit/editor.cpp
 *  @brief     The editor that runs many edits on one drawing.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/edit/editor.hpp"

#include "edit/add.hpp"
#include "edit/cached.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/table.hpp"

#include <expected>
#include <memory>
#include <string_view>
#include <utility>

namespace DwgIO::Edit
{
Editor::Editor(Drawing &drawing) : m_Index{std::make_unique<CachedLedger>(drawing)} {}

Editor::Editor(Editor &&) noexcept = default;

auto Editor::operator=(Editor &&) noexcept -> Editor & = default;

Editor::~Editor() = default;

auto Editor::MakeHandle() -> Handle
{
  return m_Index->MakeHandle();
}

auto Editor::Find(Handle self) const -> const Object *
{
  return m_Index->Find(self);
}

auto Editor::Find(Handle self) -> Object *
{
  return m_Index->Find(self);
}

auto Editor::Add(Object entity, Space space) -> std::expected<Handle, Failure>
{
  return Edit::Add(*m_Index, std::move(entity), space);
}

auto Editor::Add(Object entity, std::string_view block) -> std::expected<Handle, Failure>
{
  return Edit::Add(*m_Index, std::move(entity), block);
}

auto Editor::AddAttribute(Handle insert, Entities::Attrib attribute) -> std::expected<Handle, Failure>
{
  return Edit::AddAttribute(*m_Index, insert, std::move(attribute));
}

auto Editor::AddVertex(Handle polyline, Entities::Vertex vertex) -> std::expected<Handle, Failure>
{
  return Edit::AddVertex(*m_Index, polyline, std::move(vertex));
}

auto Editor::AddBlock(std::string_view name) -> std::expected<Handle, Failure>
{
  return Edit::AddBlock(*m_Index, name);
}

auto Editor::Rename(Handle record, std::string_view name) -> std::expected<void, Failure>
{
  Drawing &drawing = m_Index->Target();
  std::expected<void, Failure> renamed = Edit::Rename(drawing, record, name);
  if (renamed)
  {
    m_Index = std::make_unique<CachedLedger>(drawing);
  }
  return renamed;
}

auto Editor::Remove(Handle object) -> std::expected<void, Failure>
{
  Drawing &drawing = m_Index->Target();
  std::expected<void, Failure> removed = Edit::Remove(drawing, object);
  if (removed)
  {
    m_Index = std::make_unique<CachedLedger>(drawing);
  }
  return removed;
}

auto Editor::AddRecord(Object record) -> std::expected<Handle, Failure>
{
  return Edit::AddRecord(*m_Index, std::move(record));
}
} // namespace DwgIO::Edit
