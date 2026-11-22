/***********************************************************************************************************************
 *  @file      source/edit/remove.cpp
 *  @brief     Removing an object and what it owns, or refusing to.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "audit/common.hpp"
#include "edit/add.hpp"
#include "edit/dependents.hpp"
#include "edit/essential.hpp"
#include "edit/unhook.hpp"
#include "edit/usage.hpp"
#include "making/records.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/attrib.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/entities/endblk.hpp"
#include "dwgio/entities/seqend.hpp"
#include "dwgio/entities/vertex.hpp"
#include "dwgio/failure.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/layout.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tables/control.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/view.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>

namespace DwgIO::Edit
{
namespace
{
using Tables::Table;

/**
 *  A record by its table's word and name, a layout by its name, a dictionary's entry by its type and the name it is
 *  entered under, anything else by its type and handle.
 */
auto Describe(const Drawing &drawing, const Object &object) -> std::string
{
  const Handle self = BaseOf(object).Self;
  if (const std::optional<Table> table = Making::TableIn(object))
  {
    const std::string_view name = Making::NameIn(object);
    return std::format("{} {}", WordOf(*table), name.empty() ? HexOf(self) : name);
  }
  if (const auto *layout = GetIf<Objects::Layout>(&object))
  {
    return "layout " + layout->Name;
  }
  if (const auto *dictionary = GetIf<Objects::Dictionary>(Find(drawing, BaseOf(object).Owner)))
  {
    const auto entry = std::ranges::find(dictionary->Entries, self, &Objects::Entry::Object);
    if (entry != dictionary->Entries.end())
    {
      return std::format("{} {}", TypeOf(object), entry->Name);
    }
  }
  return std::format("{} {}", TypeOf(object), HexOf(self));
}

/**
 *  Whether an entity frames a block or follows a sequence's head, so is removed only with its owner.
 */
auto Member(const Object &object) -> bool
{
  return Holds<Entities::Block>(object) || Holds<Entities::EndBlk>(object) || Holds<Entities::SeqEnd>(object) ||
         Holds<Entities::Vertex>(object) || Holds<Entities::Attrib>(object);
}

/**
 *  Why the object cannot go on its own, before what it owns is looked at; empty when it can.
 */
auto Refusal(const Drawing &drawing, Handle self) -> std::optional<Failure>
{
  const Object *object = Find(drawing, self);
  if (object == nullptr)
  {
    return Failure{.Code = FailureCode::Missing, .What = std::format("handle {} missing", HexOf(self))};
  }
  if (const auto *control = GetIf<Tables::Control>(object))
  {
    return Failure{.Code = FailureCode::Refused, .What = std::format("{} table required", WordOf(control->Table))};
  }
  if (self == drawing.Header.DictionaryNamedObjects)
  {
    return Failure{.Code = FailureCode::Refused, .What = "root dictionary required"};
  }
  if (IsRequired(drawing, *object))
  {
    return Failure{.Code = FailureCode::Refused, .What = Describe(drawing, *object) + " required"};
  }
  const Object *owner = Find(drawing, BaseOf(*object).Owner);
  if (Member(*object) && owner != nullptr)
  {
    return Failure{.Code = FailureCode::Refused,
                   .What = std::format("{} {} belongs to {}", TypeOf(*object), HexOf(self), Describe(drawing, *owner))};
  }
  return std::nullopt;
}
} // namespace

auto Remove(Drawing &drawing, Handle object) -> std::expected<void, Failure>
{
  if (std::optional<Failure> refusal = Refusal(drawing, object))
  {
    return std::unexpected(std::move(*refusal));
  }
  const std::unordered_set<std::uint64_t> removed = CollectRemoved(drawing, object);
  if (const Handle used = FindUsed(drawing, removed))
  {
    return std::unexpected(
        Failure{.Code = FailureCode::InUse, .What = Describe(drawing, *Find(drawing, used)) + " in use"});
  }
  UnhookRemoved(drawing, removed);
  Audit::EraseObjects(drawing, removed);
  return {};
}
} // namespace DwgIO::Edit
