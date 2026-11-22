/***********************************************************************************************************************
 *  @file      source/audit/names.cpp
 *  @brief     The audit of names: records with none or with one another bears.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "ascii.hpp"
#include "audit/checks.hpp"
#include "audit/common.hpp"
#include "audit/pointers.hpp"
#include "edit/add.hpp"
#include "making/records.hpp"

#include "dwgio/audit.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/tables/style.hpp"
#include "dwgio/tables/table.hpp"
#include "dwgio/view.hpp"
#include "dwgio/xdata.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Audit
{
namespace
{
using Tables::Table;

/**
 *  The record of a table that what named a record left out is moved to, by table; none for the rest.
 */
constexpr std::array<std::pair<Table, std::string_view>, 5> Defaults{{{Table::Layer, "0"},
                                                                      {Table::Style, "Standard"},
                                                                      {Table::LType, "ByLayer"},
                                                                      {Table::DimStyle, "Standard"},
                                                                      {Table::AppId, "ACAD"}}};

auto DefaultOf(const Drawing &drawing, Table table) -> Handle
{
  for (const auto &[which, name] : Defaults)
  {
    if (which == table)
    {
      return Present(drawing, table, name);
    }
  }
  return {};
}

/**
 *  Rewrites what named a record left out, in the objects, their extended data, the header and the controls.
 */
void Redirect(Drawing &drawing, const std::unordered_map<std::uint64_t, Handle> &moved)
{
  const auto followed = [&moved](Handle &target) -> void
  {
    if (const auto to = moved.find(target.Value); to != moved.end())
    {
      target = to->second;
    }
  };
  for (Object &object : drawing.Objects)
  {
    for (const Referring &referring : ReferringOf(object))
    {
      followed(*referring.Target);
    }
    for (Xdata &xdata : BaseOf(object).Extended)
    {
      followed(xdata.AppId);
    }
  }
  for (Handle *rooted : CollectRoots(drawing))
  {
    followed(*rooted);
  }
}
} // namespace

void Names(Drawing &drawing, Report &report)
{
  std::unordered_map<std::uint64_t, Handle> moved;
  std::unordered_set<std::uint64_t> removed;
  for (const Root &root : Controls)
  {
    // A viewport entity header has no name, as a style over a shape file has none.
    if (root.Table == Table::ViewportEntityHeader)
    {
      continue;
    }
    std::vector<std::pair<std::string, Handle>> seen;
    const std::string_view word = Edit::WordOf(root.Table);
    for (const Object &object : drawing.Objects)
    {
      if (Making::TableIn(object) != root.Table)
      {
        continue;
      }
      const Handle self = BaseOf(object).Self;
      const std::string name{Making::NameIn(object)};
      const auto *style = GetIf<Tables::Style>(&object);
      if (name.empty() && (style == nullptr || !style->Shape))
      {
        NoteFailure(report, std::format("{} {} has no name", word, HexOf(self)), self);
        removed.insert(self.Value);
        moved[self.Value] = DefaultOf(drawing, root.Table);
        continue;
      }
      const auto first = std::ranges::find_if(seen, [&name](const std::pair<std::string, Handle> &one) -> bool
                                              { return Ascii::Same(one.first, name); });
      // The tiled viewports of one configuration share its name, and AutoCAD stores every anonymous block, the
      // paper spaces after the first among them, under its bare "*" name.
      if (first == seen.end() || root.Table == Table::VPort || (root.Table == Table::Block && name.starts_with('*')))
      {
        seen.emplace_back(name, self);
        continue;
      }
      NoteFailure(report, std::format("{} {} repeats {}", word, name, first->first), self);
      removed.insert(self.Value);
      moved[self.Value] = first->second;
    }
  }
  if (removed.empty())
  {
    return;
  }
  Redirect(drawing, moved);
  EraseObjects(drawing, removed);
}
} // namespace DwgIO::Audit
