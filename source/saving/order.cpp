/***********************************************************************************************************************
 *  @file      source/saving/order.cpp
 *  @brief     The order a save writes a file's sections in.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "saving/order.hpp"

#include "file/names.hpp"
#include "saving/kept.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/issue.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace DwgIO::Saving
{
namespace
{
constexpr std::array<std::string_view, 8> Usual{File::Names::Preview, File::Names::Header,     File::Names::Classes,
                                                File::Names::Pad,     File::Names::Objects,    File::Names::Map,
                                                File::Names::Second,  File::Names::Measurement};
/**
 *  The stream order of AutoCAD's paged files, the sections a drawing lacks skipped.
 */
constexpr std::array<std::string_view, 13> PagedUsual{
    File::Names::SummaryInfo, File::Names::Preview,    File::Names::AppInfo, File::Names::AppInfoHistory,
    File::Names::DataStore,   File::Names::RevHistory, File::Names::Objects, File::Names::ObjFreeSpace,
    File::Names::Measurement, File::Names::Map,        File::Names::Classes, File::Names::Auxiliary,
    File::Names::Header};

/**
 *  What a block describes, which it must come after: the map the objects, the second header the located blocks.
 */
constexpr std::array<std::pair<std::string_view, std::string_view>, 5> After{
    {{File::Names::Map, File::Names::Objects},
     {File::Names::Second, File::Names::Auxiliary},
     {File::Names::Second, File::Names::Header},
     {File::Names::Second, File::Names::Classes},
     {File::Names::Second, File::Names::Map}}};

/**
 *  Moves a block after what it describes where the order has it before, and notes the move.
 */
auto Reorder(std::vector<std::string> order, std::vector<Issue> &issues) -> std::vector<std::string>
{
  for (const auto &[block, described] : After)
  {
    const auto at = FindName(order, block);
    const auto need = FindName(order, described);
    if (at != order.end() && need != order.end() && at < need)
    {
      order.erase(at);
      order.insert(FindName(order, described) + 1, std::string{block});
      issues.push_back(
          Issue{.Code = IssueCode::Moved, .What = std::format("{} block moved after the {}", block, described)});
    }
  }
  return order;
}

/**
 *  Whether a drawing's order names the blocks every file has: the header, the objects and the map.
 */
auto HasEvery(const Drawing &drawing) -> bool
{
  constexpr std::array Every{File::Names::Header, File::Names::Objects, File::Names::Map};
  return std::ranges::all_of(Every, [&drawing](std::string_view name) -> bool
                             { return std::ranges::contains(drawing.Order, name); });
}
} // namespace

auto FindName(std::vector<std::string> &order, std::string_view name) -> std::vector<std::string>::iterator
{
  return std::ranges::find(order, name, [](const std::string &one) -> std::string_view { return one; });
}

auto PagedOrder(const Drawing &drawing, std::vector<Issue> &issues) -> std::vector<std::string>
{
  std::vector<std::string> order;
  for (const std::string &name :
       HasEvery(drawing) ? drawing.Order : std::vector<std::string>{PagedUsual.begin(), PagedUsual.end()})
  {
    if (name == File::Names::Pad || name == File::Names::Second)
    {
      continue;
    }
    if (name == File::Names::Unknown)
    {
      issues.push_back(
          Issue{.Code = IssueCode::NotKept, .What = "unknown block left out, a paged file has no place for it"});
      continue;
    }
    order.push_back(name);
  }
  if (!std::ranges::contains(order, File::Names::Measurement))
  {
    order.insert(FindName(order, File::Names::Map), std::string{File::Names::Measurement});
  }
  // The sections AutoCAD requires take their usual places when the drawing has none.
  if (!std::ranges::contains(order, File::Names::RevHistory))
  {
    order.insert(FindName(order, File::Names::Objects), std::string{File::Names::RevHistory});
  }
  if (!std::ranges::contains(order, File::Names::ObjFreeSpace))
  {
    order.insert(std::next(FindName(order, File::Names::Objects)), std::string{File::Names::ObjFreeSpace});
  }
  if (!std::ranges::contains(order, File::Names::Auxiliary))
  {
    order.insert(FindName(order, File::Names::Header), std::string{File::Names::Auxiliary});
  }
  // The application information, which every save makes, after the preview as AutoCAD writes it; the summary and
  // the file dependencies first, where the drawing has them.
  if (!std::ranges::contains(order, File::Names::AppInfo))
  {
    const auto preview = FindName(order, File::Names::Preview);
    order.insert(preview == order.end() ? order.begin() : std::next(preview), std::string{File::Names::AppInfo});
  }
  for (const auto &[name, has] : {std::pair{File::Names::SummaryInfo, drawing.SummaryInfo.has_value()},
                                  std::pair{File::Names::FileDepList, drawing.FileDependencies.has_value()}})
  {
    if (has && !std::ranges::contains(order, name))
    {
      order.insert(order.begin(), std::string{name});
    }
  }
  return order; // A paged file's map counts offsets in the objects section: where it sits is the drawing's choice.
}

auto LocatorOrder(const Drawing &drawing, std::vector<Issue> &issues) -> std::vector<std::string>
{
  // A paged file's order puts the classes and the header after the map, where the text's reader expects the second
  // header; a drawing from one takes AutoCAD's own R2000 order, its auxiliary header first when it has one.
  const bool paged =
      std::ranges::any_of(drawing.Order, [](const std::string &name) -> bool { return name.starts_with("AcDb:"); });
  std::vector<std::string> order;
  if (paged || !HasEvery(drawing))
  {
    if (std::ranges::contains(drawing.Kept, File::Names::Auxiliary, &Section::Name))
    {
      order.emplace_back(File::Names::Auxiliary);
    }
    order.insert(order.end(), Usual.begin(), Usual.end());
    if (drawing.Preview.empty())
    {
      std::erase(order, File::Names::Preview);
    }
    for (const std::string &name : drawing.Order)
    {
      if (IsLost(name) && std::ranges::contains(drawing.Kept, name, &Section::Name))
      {
        issues.push_back(Issue{.Code = IssueCode::NotKept,
                               .What = std::format("section {} left out, a locator file has no place for it", name)});
      }
    }
  }
  else
  {
    order = drawing.Order;
  }
  if (drawing.FileDependencies)
  {
    issues.push_back(
        Issue{.Code = IssueCode::NotKept, .What = "file dependencies left out, a locator file has no place for them"});
  }
  return Reorder(std::move(order), issues);
}
} // namespace DwgIO::Saving
