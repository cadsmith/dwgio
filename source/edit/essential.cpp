/***********************************************************************************************************************
 *  @file      source/edit/essential.cpp
 *  @brief     The records a drawing cannot do without, which no edit removes or renames.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "edit/essential.hpp"

#include "ascii.hpp"
#include "making/records.hpp"

#include "dwgio/tables/block.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/view.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace DwgIO::Edit
{
namespace
{
using Tables::Table;

constexpr std::array<std::pair<Table, std::string_view>, 8> Records{{{Table::Layer, "0"},
                                                                     {Table::LType, "ByBlock"},
                                                                     {Table::LType, "ByLayer"},
                                                                     {Table::LType, "Continuous"},
                                                                     {Table::Style, "Standard"},
                                                                     {Table::DimStyle, "Standard"},
                                                                     {Table::AppId, "ACAD"},
                                                                     {Table::VPort, "*Active"}}};
} // namespace

auto IsRequired(const Drawing &drawing, const Object &object) -> bool
{
  if (const auto *block = GetIf<Tables::BlockRecord>(&object))
  {
    const Handle self = block->Record.Base.Self;
    return self == drawing.Header.BlockRecordModelSpace || self == drawing.Header.BlockRecordPaperSpace ||
           block->Layout;
  }
  const std::optional<Table> table = Making::TableIn(object);
  const std::string_view name = Making::NameIn(object);
  return std::ranges::any_of(Records, [&](const std::pair<Table, std::string_view> &one) -> bool
                             { return one.first == table && Ascii::Same(one.second, name); });
}
} // namespace DwgIO::Edit
