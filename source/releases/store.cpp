/***********************************************************************************************************************
 *  @file      source/releases/store.cpp
 *  @brief     The ACIS bodies a save across R2013 moves into or out of the data store.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/store.hpp"

#include "acis/convert.hpp"
#include "file/names.hpp"
#include "octets.hpp"
#include "releases/acis.hpp"
#include "store/entry.hpp"
#include "store/layout.hpp"
#include "store/read.hpp"
#include "store/write.hpp"

#include "dwgio/drawing.hpp"
#include "dwgio/entities/geometry.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/object.hpp"
#include "dwgio/section.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
constexpr std::string_view DataStore = File::Names::DataStore;
/**
 *  A body's format in an object.
 */
constexpr int Sab = 2;
/**
 *  What a body of ACIS 22300 opens with.
 */
constexpr std::string_view Newer = "ASM BinaryFile4";

auto FindStore(const Drawing &drawing) -> const Section *
{
  const auto found = std::ranges::find(drawing.Kept, DataStore, &Section::Name);
  return found == drawing.Kept.end() ? nullptr : &*found;
}
} // namespace

void TakeOutOfStore(Remake &remade, Version to)
{
  const Drawing &drawing = remade.Now();
  const Section *store = FindStore(drawing);
  if (drawing.Release < Version::R2013 || to >= Version::R2013 || store == nullptr)
  {
    return;
  }
  const std::expected<std::vector<Store::Entry>, std::string> entries = Store::Read(store->Bytes);
  if (!entries)
  {
    return; // Each body stays in the store, and the entity is left out.
  }
  std::unordered_map<std::uint64_t, const Store::Entry *> bodies;
  for (const Store::Entry &entry : *entries)
  {
    if (entry.Schema == Store::Bodies)
    {
      bodies.emplace(entry.Handle, &entry);
    }
  }
  Drawing &unstored = remade.Change();
  for (Object &object : unstored.Objects)
  {
    Entities::Geometry *geometry = GeometryOf(object);
    Base &base = BaseOf(object);
    const auto found = bodies.find(base.Self.Value);
    if (geometry != nullptr && base.Stored && geometry->Body.Version == 0 && found != bodies.end())
    {
      geometry->Body = Entities::Acis{.Version = Sab, .Textual = false, .Data = BytesOf(found->second->Data)};
      base.Stored = false;
    }
  }
}

auto MakeStore(const Drawing &drawing, Version to, std::span<const Handle> leftOut)
    -> std::optional<std::vector<std::byte>>
{
  if (drawing.Release >= Version::R2013 || to < Version::R2013)
  {
    return std::nullopt;
  }
  std::vector<Store::Entry> bodies;
  for (const Object &object : drawing.Objects)
  {
    const Entities::Geometry *geometry = GeometryOf(object);
    const Handle self = BaseOf(object).Self;
    if (geometry == nullptr || geometry->Body.Version == 0 ||
        std::ranges::binary_search(leftOut, self.Value, {}, &Handle::Value))
    {
      continue;
    }
    if (const auto body = Acis::Convert(geometry->Body, to, IsProcedural(object)))
    {
      bodies.push_back(Store::Entry{
          .Handle = self.Value, .Schema = std::string{Store::Bodies}, .Data = TextOf(body->Data), .Ranges = {}});
    }
  }
  if (bodies.empty())
  {
    return std::nullopt;
  }
  return Store::Write(bodies);
}

auto LowerStore(std::span<const std::byte> store, std::vector<Issue> &issues) -> std::vector<std::byte>
{
  std::vector<std::byte> restored{store.begin(), store.end()};
  const std::expected<std::vector<Store::Entry>, std::string> entries = Store::Read(store);
  if (!entries)
  {
    return restored;
  }
  for (const Store::Entry &entry : *entries)
  {
    if (entry.Schema != Store::Bodies || !entry.Data.starts_with(Newer))
    {
      continue;
    }
    const auto body = Acis::Convert(Entities::Acis{.Version = Sab, .Textual = false, .Data = BytesOf(entry.Data)},
                                    Version::R2013, false);
    // The records of ACIS 21800 and 22300 are alike: a body AutoCAD wrote keeps its size, which its place in the
    // store needs; one written otherwise, a text of a byte's length behind two, would not.
    if (!body || body->Data.size() != entry.Data.size())
    {
      issues.push_back(Issue{.Code = IssueCode::NotKept,
                             .Subject = Handle{entry.Handle},
                             .What = std::format("body of {} kept as R2018 holds it", HexOf(Handle{entry.Handle}))});
      continue;
    }
    std::size_t from = 0;
    for (const Store::Range &range : entry.Ranges)
    {
      for (std::size_t at = 0; at < range.Size; ++at)
      {
        restored[range.Offset + at] = static_cast<std::byte>(body->Data[from + at]);
      }
      from += range.Size;
    }
  }
  return restored;
}
} // namespace DwgIO::Releases
