/***********************************************************************************************************************
 *  @file      source/edit/rename.cpp
 *  @brief     Renaming a table record or a block, refused as AutoCAD's RENAME refuses it.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "edit/add.hpp"
#include "edit/direct.hpp"
#include "edit/essential.hpp"
#include "making/records.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/table.hpp"

#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace DwgIO::Edit
{
namespace
{
/**
 *  The characters AutoCAD refuses in a symbol name.
 */
constexpr std::string_view Forbidden = "<>/\\\":;?*|,=`";

auto Dependent(const Object &object) -> bool
{
  bool dependent = false;
  Visit(
      [&dependent](const auto &kind) -> void
      {
        if constexpr (requires { kind.Record.Dependent; })
        {
          dependent = kind.Record.Dependent;
        }
      },
      object);
  return dependent;
}
} // namespace

auto Rename(Drawing &drawing, Handle record, std::string_view name) -> std::expected<void, Failure>
{
  DirectLedger ledger{drawing};
  Object *object = ledger.Find(record);
  if (object == nullptr)
  {
    return Refuse(FailureCode::Missing, std::format("handle {} missing", HexOf(record)));
  }
  const std::optional<Tables::Table> table = Making::TableIn(*object);
  if (!table)
  {
    return Refuse(FailureCode::Refused, std::format("{} {} is no record", TypeOf(*object), HexOf(record)));
  }
  const std::string_view word = WordOf(*table);
  const std::string old{Making::NameIn(*object)};
  if (IsRequired(drawing, *object) || old.starts_with('*') || Dependent(*object))
  {
    return Refuse(FailureCode::Refused, std::format("{} {} not renamed", word, old));
  }
  if (name.empty() || name.starts_with('*') || name.find_first_of(Forbidden) != std::string_view::npos)
  {
    return Refuse(FailureCode::Refused, std::format("{} name \"{}\" not allowed", word, name));
  }
  if (const Object *same = ledger.FindRecord(*table, name); same != nullptr && same != object)
  {
    return Refuse(FailureCode::Exists, std::format("{} {} exists", word, name));
  }
  Visit(
      [name](auto &kind) -> void
      {
        if constexpr (requires { kind.Record.Name; })
        {
          kind.Record.Name = std::string{name};
        }
      },
      *object);
  if (const auto *block = GetIf<Tables::BlockRecord>(object))
  {
    // The BLOCK repeats its record's name.
    if (auto *opening = GetIf<Entities::Block>(ledger.Find(block->Block)))
    {
      opening->Name = std::string{name};
    }
  }
  return {};
}
} // namespace DwgIO::Edit
