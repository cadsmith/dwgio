/***********************************************************************************************************************
 *  @file      source/edit/add.hpp
 *  @brief     The additions of entities, blocks, attributes and records.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#ifndef DWGIO_EDIT_ADD_HPP
#define DWGIO_EDIT_ADD_HPP

#include "edit/ledger.hpp"

#include "dwgio/edit.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/table.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <utility>

/**
 *  The additions of the public functions and of the editor, made through a ledger: each sets, refuses and leaves the
 *  drawing as the public function of its name says.
 */
namespace DwgIO::Edit
{
/**
 *  An edit refused, for the reason given.
 */
[[nodiscard]] inline auto Refuse(FailureCode code, std::string what) -> std::unexpected<Failure>
{
  return std::unexpected(Failure{.Code = code, .What = std::move(what)});
}

[[nodiscard]] auto Add(Ledger &ledger, Object entity, Space space) -> std::expected<Handle, Failure>;

[[nodiscard]] auto Add(Ledger &ledger, Object entity, std::string_view block) -> std::expected<Handle, Failure>;

[[nodiscard]] auto AddAttribute(Ledger &ledger, Handle insert, Entities::Attrib attribute)
    -> std::expected<Handle, Failure>;

[[nodiscard]] auto AddVertex(Ledger &ledger, Handle polyline, Entities::Vertex vertex)
    -> std::expected<Handle, Failure>;

[[nodiscard]] auto AddBlock(Ledger &ledger, std::string_view name) -> std::expected<Handle, Failure>;

[[nodiscard]] auto ListRecord(Ledger &ledger, Tables::Table table, Object record) -> std::expected<Handle, Failure>;

/**
 *  Listed, refused when a record of the table bears the record's name in any case, as the public AddRecord.
 */
[[nodiscard]] auto AddRecord(Ledger &ledger, Object record) -> std::expected<Handle, Failure>;

/**
 *  The word a message names a table by, such as layer, linetype or block.
 */
[[nodiscard]] auto WordOf(Tables::Table table) -> std::string_view;
} // namespace DwgIO::Edit

#endif /* DWGIO_EDIT_ADD_HPP */
