/***********************************************************************************************************************
 *  @file      source/releases/roundtrip.cpp
 *  @brief     What styles keep in a record below the release that gave them a field.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/roundtrip.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "releases/history.hpp"
#include "releases/lookup.hpp"
#include "releases/multileader.hpp"
#include "releases/omission.hpp"
#include "releases/untyped.hpp"
#include "releases/visualstyle.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/modeler.hpp"
#include "dwgio/entities/multileader.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/visualstyle.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/opaque.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/unknown.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
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
 *  The entry of a style's extension dictionary AutoCAD keeps the record under.
 */
constexpr std::string_view Entry = "ACAD_XREC_ROUNDTRIP";

/**
 *  The markers of a view style's display name and flags in its record.
 */
constexpr std::string_view Shown = "DISPLAYNAME";
constexpr std::string_view Flags = "FLAGS";

/**
 *  The group codes of the record: a marker, a text and a whole number.
 */
constexpr int Marker = 102;
constexpr int Text = 1;
constexpr int Whole = 90;

/**
 *  View styles stay untyped.
 */
auto IsViewStyle(const Object &object) -> bool
{
  const auto *unknown = GetIf<Unknown>(&object);
  const std::string_view name = unknown != nullptr ? unknown->Class.View() : std::string_view{};
  return name == "ACDBDETAILVIEWSTYLE" || name == "ACDBSECTIONVIEWSTYLE";
}

/**
 *  Whether a save from one release to the other changes where a style keeps what it holds: a visual style's below
 *  R2013, a view style's across R2018.
 */
auto MovesStyle(const Object &object, Version from, Version to) -> bool
{
  if (Holds<Objects::VisualStyle>(object))
  {
    return from < Version::R2013 || to < Version::R2013;
  }
  return IsViewStyle(object) && (from >= Version::R2018) != (to >= Version::R2018);
}

/**
 *  Whether a save from one release to the other moves a multileader's R2010 values across R2010: downgrading, one whose
 *  lines override anything, whose text attaches vertically or above or below anywhere but at its center, or that may
 *  hold a record; upgrading, one that may hold a record.
 */
auto MovesLines(const Object &object, Version from, Version to) -> bool
{
  const auto *leader = GetIf<Entities::MultiLeader>(&object);
  if (leader == nullptr || (from < Version::R2010) == (to < Version::R2010))
  {
    return false;
  }
  if (leader->Base.Dictionary)
  {
    return true;
  }
  const Entities::MultiLeaders::MLeader &callout = leader->MLeader;
  constexpr int Centered = 9;
  return to < Version::R2010 &&
         (callout.TextAttachmentDirection != 0 || callout.TextTopAttachment != Centered ||
          callout.TextBottomAttachment != Centered ||
          std::ranges::any_of(
              callout.Context.Leaders,
              [](const Entities::MultiLeaders::LeaderRoot &branch) -> bool
              {
                return std::ranges::any_of(branch.Lines, [](const Entities::MultiLeaders::LeaderLine &stem) -> bool
                                           { return stem.Overrides != 0; });
              }));
}

/**
 *  Whether a save from one release to the other moves a solid's history across R2007: downgrading, a solid that has
 *  one; upgrading, one that may hold a record.
 */
auto MovesHistory(const Object &object, Version from, Version to) -> bool
{
  const auto *solid = GetIf<Entities::Solid3d>(&object);
  if (solid == nullptr || (from < Version::R2007) == (to < Version::R2007))
  {
    return false;
  }
  return to < Version::R2007 ? static_cast<bool>(solid->History) : static_cast<bool>(solid->Base.Dictionary);
}

/**
 *  Whether a save from one release to the other reads a NURBS surface's record: upgrading to R2013, where AutoCAD takes
 *  the record its save below R2013 made, and refuses a file whose NURBS surface still has it.
 */
auto ReadsNurbs(const Object &object, Version from, Version to) -> bool
{
  return Holds<Entities::Surface>(object) && from < Version::R2013 && to >= Version::R2013;
}

/**
 *  A drawing's styles made over for a release, and their records.
 */
class RoundTrip
{
public:
  RoundTrip(Drawing &drawing, Version from, Version to)
      : m_Drawing{&drawing}, m_From{from}, m_To{to}, m_Scheme{Encoding::SchemeOf(drawing.Codepage)}
  {
  }

  /**
   *  A visual style in the release's layout, the rest of R2013's in its record below it.
   */
  void Visual(Handle self)
  {
    auto &style = Get<Objects::VisualStyle>(*Find(*m_Drawing, self));
    const Objects::XRecord *record = RecordOf(self);
    const std::vector<Tag> kept = record != nullptr ? record->Data : std::vector<Tag>{};
    StyleLayout styling = FitProperties(RaiseProperties(style, m_From, kept), kept, m_To);
    style.Properties = std::move(styling.Properties);
    if (styling.Kept.empty())
    {
      Drop(self);
    }
    else
    {
      Keep(self, std::move(styling.Kept));
    }
  }

  /**
   *  A view style's bits crossing R2018: downgrading, its display name and flags taken into its record; upgrading,
   *  given from the record, else its name and none. Left as it is where its bits do not read by its layout.
   */
  void View(Handle self)
  {
    auto &unknown = Get<Unknown>(*Find(*m_Drawing, self));
    if (m_From >= Version::R2018)
    {
      std::vector<Content> taken;
      std::optional<Opaque> own =
          Carry(unknown, CarryContext{.From = m_From, .To = m_To, .Text = &m_Scheme, .Given = {}, .Taken = &taken});
      if (!own || taken.size() < 2)
      {
        return;
      }
      unknown.Body = std::move(own);
      unknown.Bytes.clear();
      Keep(self, {Tag{.Code = Marker, .Value = std::string{Shown}}, Tag{.Code = Text, .Value = taken.at(0)},
                  Tag{.Code = Marker, .Value = std::string{Flags}}, Tag{.Code = Whole, .Value = taken.at(1)}});
      return;
    }
    const Objects::XRecord *record = RecordOf(self);
    const Content *shown =
        FindMarked(record != nullptr ? std::span<const Tag>{record->Data} : std::span<const Tag>{}, Shown);
    const Content *flags =
        FindMarked(record != nullptr ? std::span<const Tag>{record->Data} : std::span<const Tag>{}, Flags);
    const std::vector<Content> given{shown != nullptr ? *shown : Content{NameOf(unknown.Base)},
                                     flags != nullptr ? *flags : Content{std::int64_t{0}}};
    std::optional<Opaque> own =
        Carry(unknown, CarryContext{.From = m_From, .To = m_To, .Text = &m_Scheme, .Given = given, .Taken = nullptr});
    if (!own)
    {
      return;
    }
    unknown.Body = std::move(own);
    unknown.Bytes.clear();
    Drop(self);
  }

  /**
   *  A multileader in the release's form: downgrading below R2010, what it and its leader lines hold of R2010's moved
   *  into its record; upgrading, given back from it, the record kept only for what else it holds.
   */
  void Leader(Handle self)
  {
    auto &leader = Get<Entities::MultiLeader>(*Find(*m_Drawing, self));
    const Objects::XRecord *record = RecordOf(self);
    std::vector<Tag> kept = record != nullptr ? record->Data : std::vector<Tag>{};
    if (m_To >= Version::R2010)
    {
      RestoreLines(leader, kept);
    }
    else
    {
      Entities::MultiLeader none;
      RestoreLines(none, kept); // What an earlier upgrade left of the leader's values is dropped.
      std::vector<Tag> lines = MakeLinesRecord(leader);
      kept.insert(kept.end(), lines.begin(), lines.end());
    }
    if (kept.empty())
    {
      Drop(self);
    }
    else
    {
      Keep(self, std::move(kept));
    }
  }

  /**
   *  A solid's history in the release's form: downgrading below R2007, kept in its record, unless the save leaves the
   *  history out; upgrading, given back from the record, the record kept only for what else it holds.
   */
  void Solid(Handle self)
  {
    auto &solid = Get<Entities::Solid3d>(*Find(*m_Drawing, self));
    const Objects::XRecord *record = RecordOf(self);
    std::vector<Tag> kept = record != nullptr ? record->Data : std::vector<Tag>{};
    if (m_To >= Version::R2007)
    {
      RestoreHistory(solid, kept);
      solid.History = Find(*m_Drawing, solid.History) != nullptr ? solid.History : Handle{};
    }
    else
    {
      Entities::Solid3d none;
      RestoreHistory(none, kept); // What an earlier upgrade left of the history is dropped.
      if (!std::ranges::binary_search(CollectGone(), solid.History.Value, {}, &Handle::Value))
      {
        std::vector<Tag> history = MakeHistoryRecord(solid);
        kept.insert(kept.end(), history.begin(), history.end());
      }
    }
    if (kept.empty())
    {
      Drop(self);
    }
    else
    {
      Keep(self, std::move(kept));
    }
  }

  /**
   *  A NURBS surface's record taken away, as AutoCAD reads it when upgrading to R2013.
   */
  void Read(Handle self) { Drop(self); }

  /**
   *  The drawing without the records and dictionaries taken away.
   */
  void Finish()
  {
    std::erase_if(m_Drawing->Objects,
                  [this](const Object &object) -> bool { return m_Gone.contains(BaseOf(object).Self.Value); });
  }

private:
  /**
   *  The style's record, or null.
   */
  auto RecordOf(Handle self) -> Objects::XRecord *
  {
    auto *dictionary = GetIf<Objects::Dictionary>(Find(*m_Drawing, BaseOf(*Find(*m_Drawing, self)).Dictionary));
    if (dictionary == nullptr)
    {
      return nullptr;
    }
    const auto entry = std::ranges::find(dictionary->Entries, Entry, &Objects::Entry::Name);
    return entry == dictionary->Entries.end() ? nullptr : GetIf<Objects::XRecord>(Find(*m_Drawing, entry->Object));
  }

  /**
   *  The name the style's owner lists it under, or empty.
   */
  [[nodiscard]] auto NameOf(const Base &base) const -> std::string
  {
    const auto *owner = GetIf<Objects::Dictionary>(Find(*m_Drawing, base.Owner));
    if (owner == nullptr)
    {
      return {};
    }
    const auto entry = std::ranges::find(owner->Entries, base.Self, &Objects::Entry::Object);
    return entry == owner->Entries.end() ? std::string{} : entry->Name;
  }

  /**
   *  The style's record holding the groups: its own filled, else one made in its extension dictionary, made too where
   *  it has none, as AutoCAD makes them, a hard owner of its record.
   */
  void Keep(Handle self, std::vector<Tag> groups)
  {
    if (Objects::XRecord *record = RecordOf(self))
    {
      record->Data = std::move(groups);
      return;
    }
    Base &base = BaseOf(*Find(*m_Drawing, self));
    auto *dictionary = GetIf<Objects::Dictionary>(Find(*m_Drawing, base.Dictionary));
    const Handle owner = dictionary != nullptr ? base.Dictionary : Next();
    const Handle made = Next();
    if (dictionary != nullptr)
    {
      // An entry of the name that names no record names the one made.
      const auto entry = std::ranges::find(dictionary->Entries, Entry, &Objects::Entry::Name);
      if (entry != dictionary->Entries.end())
      {
        entry->Object = made;
      }
      else
      {
        dictionary->Entries.push_back(Objects::Entry{.Name = std::string{Entry}, .Object = made});
      }
    }
    else
    {
      base.Dictionary = owner;
      m_Drawing->Objects.emplace_back(Objects::Dictionary{.Base = {.Self = owner, .Owner = self},
                                                          .HardOwner = true,
                                                          .Cloning = 1,
                                                          .Entries = {{.Name = std::string{Entry}, .Object = made}},
                                                          .Default = {}});
    }
    m_Drawing->Objects.emplace_back(Objects::XRecord{.Base = {.Self = made, .Owner = owner, .Reactors = {owner}},
                                                     .Cloning = 1,
                                                     .Data = std::move(groups),
                                                     .Objects = {},
                                                     .Rest = {}});
  }

  /**
   *  The style's record taken away, and its extension dictionary with it when that holds nothing else.
   */
  void Drop(Handle self)
  {
    Base &base = BaseOf(*Find(*m_Drawing, self));
    auto *dictionary = GetIf<Objects::Dictionary>(Find(*m_Drawing, base.Dictionary));
    if (dictionary == nullptr)
    {
      return;
    }
    const auto entry = std::ranges::find(dictionary->Entries, Entry, &Objects::Entry::Name);
    if (entry == dictionary->Entries.end())
    {
      return;
    }
    m_Gone.insert(entry->Object.Value);
    dictionary->Entries.erase(entry);
    if (dictionary->Entries.empty())
    {
      m_Gone.insert(base.Dictionary.Value);
      base.Dictionary = {};
    }
  }

  /**
   *  What the save leaves out, as the drawing was before any record was made.
   */
  auto CollectGone() -> const std::vector<Handle> &
  {
    if (!m_Out)
    {
      m_Out = CollectLeftOut(*m_Drawing, m_To);
    }
    return *m_Out;
  }

  /**
   *  A handle no object bears: the first from the seed past every handle, the next one after it.
   */
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
  Version m_From;
  Version m_To;
  Encoding::Scheme m_Scheme;
  std::unordered_set<std::uint64_t> m_Gone;
  std::optional<std::vector<Handle>> m_Out;
  bool m_Seeded{};
};
} // namespace

void KeepRoundTrips(Remake &remade, Version to)
{
  const Drawing &drawing = remade.Now();
  if (drawing.Release == to)
  {
    return;
  }
  std::vector<Handle> styles;
  const Lookup found{drawing};
  for (const Object &object : drawing.Objects)
  {
    // Only an object its handle finds, as the making over finds it: not one of no handle, nor one whose handle
    // another bears first, which a broken file holds.
    if (found.First(object) && (MovesStyle(object, drawing.Release, to) || ReadsNurbs(object, drawing.Release, to) ||
                                MovesLines(object, drawing.Release, to) || MovesHistory(object, drawing.Release, to)))
    {
      styles.push_back(BaseOf(object).Self);
    }
  }
  if (styles.empty())
  {
    return;
  }
  Drawing &made = remade.Change();
  RoundTrip remaking{made, drawing.Release, to};
  for (const Handle self : styles)
  {
    if (Holds<Objects::VisualStyle>(*Find(made, self)))
    {
      remaking.Visual(self);
    }
    else if (Holds<Entities::MultiLeader>(*Find(made, self)))
    {
      remaking.Leader(self);
    }
    else if (Holds<Entities::Solid3d>(*Find(made, self)))
    {
      remaking.Solid(self);
    }
    else if (Holds<Entities::Surface>(*Find(made, self)))
    {
      remaking.Read(self);
    }
    else
    {
      remaking.View(self);
    }
  }
  remaking.Finish();
}

} // namespace DwgIO::Releases
