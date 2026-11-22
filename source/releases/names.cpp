/***********************************************************************************************************************
 *  @file      source/releases/names.cpp
 *  @brief     The names R14 cannot hold, changed by its save and kept in a record.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/names.hpp"

#include "ascii.hpp"
#include "releases/lookup.hpp"

#include "dwgio/base.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/block.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/tables/block.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/view.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
/**
 *  The entry of an extension dictionary AutoCAD keeps its round trip record under.
 */
constexpr std::string_view Roundtrip = "ACAD_XREC_ROUNDTRIP";

/**
 *  The record's marker, and the codes of a name as R14 holds it and as it was.
 */
constexpr int Marker = 102;
constexpr std::string_view Extnames = "EXTNAMES";
constexpr int Abridged = 1;
constexpr int Whole = 2;

/**
 *  The longest name R14 keeps whole, and the length it cuts a longer one to.
 */
constexpr std::size_t Longest = 31;
constexpr std::size_t Cut = 26;

/**
 *  The entry under which a dynamic block keeps a dictionary whose names AutoCAD's R14 keeps as they are.
 */
constexpr std::string_view Dynamic = "ACDBBLOCKTABLERECORDDATA";

/**
 *  A name R14 holds and the name it was.
 */
using Pair = std::pair<std::string, std::string>;

/**
 *  Not a UTF-8 continuation byte.
 */
auto IsLeadByte(char byte) -> bool
{
  constexpr unsigned Continuation = 0xC0;
  constexpr unsigned Following = 0x80;
  return (static_cast<unsigned char>(byte) & Continuation) != Following;
}

/**
 *  The groups of a record's EXTNAMES part: where it starts and where the part after it starts.
 */
auto FindExtNames(std::span<const Tag> data) -> std::pair<std::size_t, std::size_t>
{
  const Tag marker{.Code = Marker, .Value = std::string{Extnames}};
  const auto start = std::ranges::find(data, marker);
  const auto end = std::find_if(start == data.end() ? start : std::next(start), data.end(),
                                [](const Tag &group) -> bool { return group.Code == Marker; });
  return {static_cast<std::size_t>(start - data.begin()), static_cast<std::size_t>(end - data.begin())};
}

/**
 *  The names an EXTNAMES part pairs, a 1 then a 2 each.
 */
auto PairsOf(std::span<const Tag> part) -> std::vector<Pair>
{
  std::vector<Pair> pairs;
  for (std::size_t at = 0; at + 1 < part.size(); at += 2)
  {
    const auto *narrow = std::get_if<std::string>(&part[at].Value);
    const auto *whole = std::get_if<std::string>(&part[at + 1].Value);
    if (part[at].Code == Abridged && part[at + 1].Code == Whole && narrow != nullptr && whole != nullptr)
    {
      pairs.emplace_back(*narrow, *whole);
    }
  }
  return pairs;
}

/**
 *  An object's round trip record and the entry of its extension dictionary that names it.
 */
struct Held
{
  Objects::Dictionary *Dictionary{};
  Objects::XRecord *Record{};
};

auto FindHolder(Drawing &drawing, const Base &base) -> Held
{
  auto *dictionary = GetIf<Objects::Dictionary>(Find(drawing, base.Dictionary));
  if (dictionary == nullptr)
  {
    return {};
  }
  const auto entry = std::ranges::find_if(dictionary->Entries, [](const Objects::Entry &one) -> bool
                                          { return Ascii::Same(one.Name, Roundtrip); });
  auto *record = entry == dictionary->Entries.end() ? nullptr : GetIf<Objects::XRecord>(Find(drawing, entry->Object));
  return {.Dictionary = dictionary, .Record = record};
}

/**
 *  Whether an object's round trip record holds an EXTNAMES part.
 */
auto HasExtNames(const Lookup &found, const Object &object) -> bool
{
  const auto *dictionary = GetIf<Objects::Dictionary>(found.Of(BaseOf(object).Dictionary));
  if (dictionary == nullptr)
  {
    return false;
  }
  const auto entry = std::ranges::find_if(dictionary->Entries, [](const Objects::Entry &one) -> bool
                                          { return Ascii::Same(one.Name, Roundtrip); });
  const auto *record = entry == dictionary->Entries.end() ? nullptr : GetIf<Objects::XRecord>(found.Of(entry->Object));
  return record != nullptr && FindExtNames(record->Data).first < record->Data.size();
}

/**
 *  An object named again as the pairs say: a dictionary's entries, a table record and its block's BLOCK.
 */
void Rename(Drawing &drawing, Object &object, std::span<const Pair> pairs)
{
  const auto whole = [pairs](const std::string &name) -> const std::string *
  {
    const auto found = std::ranges::find(pairs, name, &Pair::first);
    return found == pairs.end() ? nullptr : &found->second;
  };
  if (auto *dictionary = GetIf<Objects::Dictionary>(&object))
  {
    for (Objects::Entry &entry : dictionary->Entries)
    {
      if (const std::string *name = whole(entry.Name))
      {
        entry.Name = *name;
      }
    }
    return;
  }
  Visit(
      [&whole](auto &one) -> void
      {
        if constexpr (requires { one.Record.Name; })
        {
          if (const std::string *name = whole(one.Record.Name))
          {
            one.Record.Name = *name;
          }
        }
      },
      object);
  if (const auto *record = GetIf<Tables::BlockRecord>(&object))
  {
    if (auto *begin = GetIf<Entities::Block>(Find(drawing, record->Block)))
    {
      begin->Name = record->Record.Name;
    }
  }
}

/**
 *  Whether AutoCAD's R14 keeps a dictionary's names as they are: one with a default, or one a dynamic block keeps.
 */
auto KeepsNames(const Drawing &drawing, const Objects::Dictionary &dictionary) -> bool
{
  if (dictionary.Default)
  {
    return true;
  }
  const auto *owner = GetIf<Objects::Dictionary>(Find(drawing, dictionary.Base.Owner));
  return owner != nullptr &&
         std::ranges::any_of(owner->Entries, [&dictionary](const Objects::Entry &entry) -> bool
                             { return entry.Object == dictionary.Base.Self && Narrow(entry.Name) == Dynamic; });
}

/**
 *  The records of names R14 keeps, made in a drawing: their extension dictionaries and records under handles no object
 *  bears, the first from the seed past every handle, the next ones after it.
 */
class RecordMaker
{
public:
  explicit RecordMaker(Drawing &drawing) : m_Drawing{&drawing} {}

  /**
   *  The EXTNAMES part put first in an object's round trip record, the record made where it has none, and its extension
   *  dictionary with it, as AutoCAD makes them.
   */
  void Keep(Handle self, std::vector<Tag> part)
  {
    Base &base = BaseOf(*Find(*m_Drawing, self));
    const Held held = FindHolder(*m_Drawing, base);
    if (held.Record != nullptr)
    {
      held.Record->Data.insert(held.Record->Data.begin(), part.begin(), part.end());
      return;
    }
    const Handle owner = held.Dictionary != nullptr ? base.Dictionary : Next();
    const Handle made = Next();
    if (held.Dictionary != nullptr)
    {
      held.Dictionary->Entries.push_back(Objects::Entry{.Name = std::string{Roundtrip}, .Object = made});
    }
    else
    {
      base.Dictionary = owner;
      m_Drawing->Objects.emplace_back(Objects::Dictionary{.Base = {.Self = owner, .Owner = self},
                                                          .HardOwner = true,
                                                          .Cloning = 1,
                                                          .Entries = {{.Name = std::string{Roundtrip}, .Object = made}},
                                                          .Default = {}});
    }
    m_Drawing->Objects.emplace_back(Objects::XRecord{.Base = {.Self = made, .Owner = owner, .Reactors = {owner}},
                                                     .Cloning = 1,
                                                     .Data = std::move(part),
                                                     .Objects = {},
                                                     .Rest = {}});
  }

private:
  auto Next() -> Handle
  {
    if (!m_Seeded)
    {
      m_Seeded = true;
      return Edit::MakeHandle(*m_Drawing);
    }
    Handle &seed = m_Drawing->Header.Handseed;
    const Handle given = seed;
    seed = Handle{.Value = seed.Value + 1};
    return given;
  }

  Drawing *m_Drawing;
  bool m_Seeded{};
};
} // namespace

auto Narrow(std::string_view name) -> std::string
{
  constexpr unsigned Ascii = 0x80;
  std::string narrow;
  for (const char one : name)
  {
    const auto letter = static_cast<unsigned char>(one);
    if (letter < Ascii && std::isalnum(letter) != 0)
    {
      narrow.push_back(DwgIO::Ascii::Upper(one));
    }
    else if (one == '$' || one == '-' || one == '_' || one == '*')
    {
      narrow.push_back(one);
    }
    else if (IsLeadByte(one))
    {
      narrow.push_back('_'); // One for each character, however many bytes it takes.
    }
  }
  if (narrow.size() > Longest)
  {
    narrow.resize(Cut);
  }
  return narrow;
}

void WidenNames(Remake &remade, Version to)
{
  const Drawing &drawing = remade.Now();
  if (drawing.Release != Version::R14 || to == Version::R14)
  {
    return;
  }
  std::vector<Handle> named;
  const Lookup found{drawing};
  for (const Object &object : drawing.Objects)
  {
    // Only an object its handle finds: not one of no handle, nor one whose handle another bears first.
    if (found.First(object) && HasExtNames(found, object))
    {
      named.push_back(BaseOf(object).Self);
    }
  }
  if (named.empty())
  {
    return;
  }
  Drawing &made = remade.Change();
  std::unordered_set<std::uint64_t> erased;
  const Lookup again{made}; // Good through the loop, which renames and empties but adds or erases nothing.
  for (const Handle self : named)
  {
    Object &object = *Find(made, self);
    if (!HasExtNames(again, object))
    {
      continue; // A record another object shares, as a broken file holds, read already.
    }
    const Held held = FindHolder(made, BaseOf(object));
    std::vector<Tag> &data = held.Record->Data;
    const auto [start, end] = FindExtNames(data);
    const std::vector<Pair> pairs = PairsOf(std::span{data}.subspan(start + 1, end - start - 1));
    data.erase(data.begin() + static_cast<std::ptrdiff_t>(start), data.begin() + static_cast<std::ptrdiff_t>(end));
    if (data.empty())
    {
      // The record holds nothing more: taken away, and the extension dictionary with it when it holds nothing else.
      erased.insert(held.Record->Base.Self.Value);
      std::erase_if(held.Dictionary->Entries,
                    [](const Objects::Entry &entry) -> bool { return entry.Name == Roundtrip; });
      if (held.Dictionary->Entries.empty())
      {
        erased.insert(BaseOf(object).Dictionary.Value);
        BaseOf(object).Dictionary = {};
      }
    }
    Rename(made, object, pairs);
  }
  std::erase_if(made.Objects,
                [&erased](const Object &object) -> bool { return erased.contains(BaseOf(object).Self.Value); });
}

void NarrowNames(Remake &remade, Version to)
{
  const Drawing &drawing = remade.Now();
  if (to != Version::R14 || drawing.Release == Version::R14)
  {
    return;
  }
  std::vector<Handle> named;
  const Lookup found{drawing};
  for (const Object &object : drawing.Objects)
  {
    const auto *dictionary = GetIf<Objects::Dictionary>(&object);
    if (dictionary != nullptr &&
        std::ranges::any_of(dictionary->Entries,
                            [](const Objects::Entry &entry) -> bool { return Narrow(entry.Name) != entry.Name; }) &&
        found.First(object) && !KeepsNames(drawing, *dictionary))
    {
      named.push_back(dictionary->Base.Self);
    }
  }
  if (named.empty())
  {
    return;
  }
  Drawing &made = remade.Change();
  RecordMaker making{made};
  for (const Handle self : named)
  {
    std::vector<Tag> part{{.Code = Marker, .Value = std::string{Extnames}}};
    for (Objects::Entry &entry : Get<Objects::Dictionary>(*Find(made, self)).Entries)
    {
      std::string narrow = Narrow(entry.Name);
      if (narrow != entry.Name)
      {
        part.push_back({.Code = Abridged, .Value = narrow});
        part.push_back({.Code = Whole, .Value = entry.Name});
        entry.Name = std::move(narrow);
      }
    }
    making.Keep(self, std::move(part));
  }
}

} // namespace DwgIO::Releases
