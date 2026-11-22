/***********************************************************************************************************************
 *  @file      source/edit.cpp
 *  @brief     The one-off edits: adding an entity, a block or a record, and a new handle.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "dwgio/edit.hpp"

#include "edit/add.hpp"
#include "edit/direct.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/table.hpp"

#include <array>
#include <cstddef>
#include <expected>
#include <string_view>
#include <utility>

namespace DwgIO::Edit
{
auto MakeHandle(Drawing &drawing) -> Handle
{
  DirectLedger ledger{drawing};
  return ledger.MakeHandle();
}

auto Add(Drawing &drawing, Object entity, Space space) -> std::expected<Handle, Failure>
{
  DirectLedger ledger{drawing};
  return Add(ledger, std::move(entity), space);
}

auto Add(Drawing &drawing, Object entity, std::string_view block) -> std::expected<Handle, Failure>
{
  DirectLedger ledger{drawing};
  return Add(ledger, std::move(entity), block);
}

auto AddRecord(Drawing &drawing, Object record) -> std::expected<Handle, Failure>
{
  DirectLedger ledger{drawing};
  return AddRecord(ledger, std::move(record));
}

auto AddAttribute(Drawing &drawing, Handle insert, Entities::Attrib attribute) -> std::expected<Handle, Failure>
{
  DirectLedger ledger{drawing};
  return AddAttribute(ledger, insert, std::move(attribute));
}

auto AddVertex(Drawing &drawing, Handle polyline, Entities::Vertex vertex) -> std::expected<Handle, Failure>
{
  DirectLedger ledger{drawing};
  return AddVertex(ledger, polyline, std::move(vertex));
}

auto AddBlock(Drawing &drawing, std::string_view name) -> std::expected<Handle, Failure>
{
  DirectLedger ledger{drawing};
  return AddBlock(ledger, name);
}
} // namespace DwgIO::Edit
