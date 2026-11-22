/***********************************************************************************************************************
 *  @file      tests/dwgio/releases/table/roundtrip.cpp
 *  @brief     Tables saved across releases, against AutoCAD's own saves of them.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/coded/layout.hpp"
#include "file/paged/layout.hpp"
#include "releases/table/content.hpp"
#include "releases/table/style.hpp"
#include "support/acad.hpp"
#include "support/fixtures.hpp"

#include "dwgio/common.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/entities/table.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/issue.hpp"
#include "dwgio/load.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/cellstylemap.hpp"
#include "dwgio/objects/datatable.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/tablecontent.hpp"
#include "dwgio/objects/tablegeometry.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/save.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace DwgIO::Releases
{
namespace
{
auto LoadClean(std::span<const std::byte> bytes) -> Drawing
{
  const std::expected<DwgIO::Loaded, Failure> loaded = Load(bytes);
  REQUIRE(loaded);
  CHECK(loaded->Issues.empty());
  return loaded->Drawing;
}

auto TableOf(const Drawing &drawing) -> Entities::AcadTable
{
  for (const Object &object : drawing.Objects)
  {
    if (const auto *table = GetIf<Entities::AcadTable>(&object))
    {
      return *table;
    }
  }
  FAIL("no table");
  return {};
}

template <typename KindT>
auto CountOf(const Drawing &drawing) -> std::size_t
{
  return static_cast<std::size_t>(
      std::ranges::count_if(drawing.Objects, [](const Object &one) { return Holds<KindT>(one); }));
}

/**
 *  The sections of two paged or coded files are the same but for the preview, whose images' starts are addresses.
 */
template <typename LayoutT>
void SameSections(const LayoutT &ours, const LayoutT &theirs)
{
  REQUIRE(ours.Sections.size() == theirs.Sections.size());
  for (std::size_t at = 0; at < theirs.Sections.size(); ++at)
  {
    CAPTURE(theirs.Sections[at].Name);
    CHECK(ours.Sections[at].Name == theirs.Sections[at].Name);
    if (theirs.Sections[at].Name != "preview")
    {
      CHECK((RemakesSection(theirs.Sections[at].Name) || ours.Sections[at].Bytes == theirs.Sections[at].Bytes));
    }
  }
}

/**
 *  The objects of a drawing's table and its style, and of what each owns, in the order they are reached from the table,
 *  each handle among them numbered by that order: two saves of one table compare equal whatever handles they gave the
 *  objects they made.
 */
class Normal
{
public:
  explicit Normal(const Drawing &drawing) : m_Drawing{&drawing}
  {
    const Entities::AcadTable table = TableOf(drawing);
    Reach(table.Base.Self);
    Reach(table.Style);
    for (std::size_t at = 0; at < m_Reached.size(); ++at)
    {
      Follow(*Find(drawing, m_Reached[at]));
    }
    for (const Handle self : m_Reached)
    {
      Object object = *Find(drawing, self);
      Renumber(object);
      if (Common *common = CommonOf(object))
      {
        common->Graphics.clear(); // AutoCAD redraws a table's proxy graphics at each save; dwgio keeps them.
      }
      Objects.push_back(std::move(object));
    }
  }

  std::vector<Object> Objects;

private:
  void Reach(Handle self)
  {
    if (self && Find(*m_Drawing, self) != nullptr && !m_Numbers.contains(self.Value))
    {
      m_Numbers.emplace(self.Value, m_Numbers.size() + 1);
      m_Reached.push_back(self);
    }
  }

  void Follow(const Object &object)
  {
    Reach(BaseOf(object).Dictionary);
    if (const auto *dictionary = GetIf<Objects::Dictionary>(&object))
    {
      for (const Objects::Entry &entry : dictionary->Entries)
      {
        Reach(entry.Object);
      }
    }
    const auto groups = [this](const std::vector<Tag> &all) -> void
    {
      for (const Tag &group : all)
      {
        if (const auto *handle = std::get_if<Handle>(&group.Value))
        {
          Reach(*handle);
        }
      }
    };
    if (const auto *record = GetIf<Objects::XRecord>(&object))
    {
      groups(record->Data);
    }
    if (const auto *data = GetIf<Objects::DataTable>(&object))
    {
      for (const Objects::DataTables::Column &column : data->Columns)
      {
        groups(column.Cells);
      }
    }
  }

  [[nodiscard]] auto Number(Handle handle) const -> Handle
  {
    const auto found = m_Numbers.find(handle.Value);
    return found == m_Numbers.end() ? handle : Handle{found->second};
  }

  void Renumber(Object &object)
  {
    Base &base = BaseOf(object);
    base.Self = Number(base.Self);
    base.Owner = Number(base.Owner);
    base.Dictionary = Number(base.Dictionary);
    for (Handle &reactor : base.Reactors)
    {
      reactor = Number(reactor);
    }
    const auto groups = [this](std::vector<Tag> &all) -> void
    {
      for (Tag &group : all)
      {
        if (auto *handle = std::get_if<Handle>(&group.Value))
        {
          *handle = Number(*handle);
        }
      }
    };
    if (auto *dictionary = GetIf<Objects::Dictionary>(&object))
    {
      for (Objects::Entry &entry : dictionary->Entries)
      {
        entry.Object = Number(entry.Object);
      }
    }
    if (auto *record = GetIf<Objects::XRecord>(&object))
    {
      groups(record->Data);
    }
    if (auto *data = GetIf<Objects::DataTable>(&object))
    {
      for (Objects::DataTables::Column &column : data->Columns)
      {
        groups(column.Cells);
      }
    }
    if (auto *table = GetIf<Entities::AcadTable>(&object))
    {
      table->Style = Number(table->Style);
      table->Content.TableStyle = Number(table->Content.TableStyle);
    }
    if (auto *content = GetIf<Objects::TableContent>(&object))
    {
      content->Data.TableStyle = Number(content->Data.TableStyle);
    }
  }

  const Drawing *m_Drawing;
  std::unordered_map<std::uint64_t, std::uint64_t> m_Numbers;
  std::vector<Handle> m_Reached;
};

/**
 *  Checks that a save holds the table, its style and what they own as AutoCAD's own save does.
 */
void SameTable(const Drawing &ours, const Drawing &theirs)
{
  const Normal mine{ours};
  const Normal acad{theirs};
  REQUIRE(mine.Objects.size() == acad.Objects.size());
  for (std::size_t at = 0; at < acad.Objects.size(); ++at)
  {
    CAPTURE(TypeOf(acad.Objects[at]));
    CAPTURE(at);
    CHECK(mine.Objects[at] == acad.Objects[at]);
  }
}

/**
 *  A fixture saved by dwgio as the release, loaded back with no issue.
 */
auto SavedAs(const Drawing &drawing, Version to) -> Drawing
{
  const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = to});
  REQUIRE(saved);
  return LoadClean(saved->Bytes);
}

/**
 *  A drawing whose table has no record, content or geometry, as an application that knows only the grid writes it.
 */
auto WithoutTableContent(Drawing drawing) -> Drawing
{
  std::unordered_set<std::uint64_t> removed;
  for (Object &object : drawing.Objects)
  {
    if (auto *table = GetIf<Entities::AcadTable>(&object))
    {
      removed.insert(table->Base.Dictionary.Value);
      table->Base.Dictionary = {};
    }
  }
  for (std::size_t before = 0; before != removed.size();)
  {
    before = removed.size();
    for (const Object &object : drawing.Objects)
    {
      if (removed.contains(BaseOf(object).Owner.Value))
      {
        removed.insert(BaseOf(object).Self.Value);
      }
    }
  }
  std::erase_if(drawing.Objects,
                [&removed](const Object &object) { return removed.contains(BaseOf(object).Self.Value); });
  return drawing;
}
} // namespace

TEST_CASE("releases/table/roundtrip: every table fixture saves to its own release byte for byte")
{
  for (const std::string_view fixture : {"table", "cells"})
  {
    for (const std::string_view release : FixtureSuffixes)
    {
      CAPTURE(fixture);
      CAPTURE(release);
      const std::vector<std::byte> bytes = ReadFixture(std::format("{}-{}.dwg", fixture, release));
      const std::expected<DwgIO::Loaded, Failure> loaded = Load(bytes);
      REQUIRE(loaded);
      CHECK(CountOf<Entities::AcadTable>(loaded->Drawing) == 1);
      CHECK(CountOf<Objects::TableStyle>(loaded->Drawing) == 1);
      CHECK(CountOf<Objects::TableContent>(loaded->Drawing) == 1);
      CHECK(CountOf<Objects::TableGeometry>(loaded->Drawing) == 1);
      CHECK(CountOf<Objects::CellStyleMap>(loaded->Drawing) == (loaded->Drawing.Release <= Version::R2007 ? 1 : 0));
      CHECK(CountOf<Objects::DataTable>(loaded->Drawing) == (loaded->Drawing.Release <= Version::R2004 ? 1 : 0));
      const std::expected<Saved, Failure> saved = Save(loaded->Drawing);
      REQUIRE(saved);
      CHECK(saved->Issues.empty());
      const std::expected<DwgIO::Loaded, Failure> again = Load(saved->Bytes);
      REQUIRE(again);
      CHECK(again->Issues.empty());
      CHECK(again->Drawing == loaded->Drawing);
      std::vector<Issue> issues;
      const Version version = loaded->Drawing.Release;
      if (version <= Version::R2000)
      {
        CHECK(saved->Bytes == bytes);
      }
      else if (version == Version::R2007)
      {
        SameSections(*File::Coded::ReadLayout(saved->Bytes, issues), *File::Coded::ReadLayout(bytes, issues));
      }
      else
      {
        SameSections(*File::Paged::ReadLayout(saved->Bytes, issues), *File::Paged::ReadLayout(bytes, issues));
      }
      CHECK(issues.empty());
    }
  }
}

TEST_CASE("releases/table/roundtrip: a table fixture saved to another release matches AutoCAD's save")
{
  struct Crossing
  {
    std::string_view Fixture;
    std::string_view From;
    std::string_view To;
    Version Release;
  };

  for (const Crossing crossing :
       {Crossing{.Fixture = "cells", .From = "r2007", .To = "r2018", .Release = Version::R2018},
        Crossing{.Fixture = "cells", .From = "r2004", .To = "r2010", .Release = Version::R2010},
        Crossing{.Fixture = "cells", .From = "r2004", .To = "r2000", .Release = Version::R2000},
        Crossing{.Fixture = "cells", .From = "r2018", .To = "r2007", .Release = Version::R2007},
        Crossing{.Fixture = "cells", .From = "r2018", .To = "r2004", .Release = Version::R2004},
        Crossing{.Fixture = "cells", .From = "r2013", .To = "r2018", .Release = Version::R2018},
        Crossing{.Fixture = "table", .From = "r2004", .To = "r2018", .Release = Version::R2018},
        Crossing{.Fixture = "table", .From = "r2018", .To = "r2000", .Release = Version::R2000},
        Crossing{.Fixture = "table", .From = "r2000", .To = "r2007", .Release = Version::R2007},
        Crossing{.Fixture = "rowheights", .From = "r2004", .To = "r2018", .Release = Version::R2018},
        Crossing{.Fixture = "rowheights", .From = "r2018", .To = "r2004", .Release = Version::R2004}})
  {
    CAPTURE(crossing.Fixture);
    CAPTURE(crossing.From);
    CAPTURE(crossing.To);
    const Drawing ours =
        SavedAs(LoadDrawing(std::format("{}-{}.dwg", crossing.Fixture, crossing.From)), crossing.Release);
    SameTable(ours, LoadDrawing(std::format("{}-{}-as-{}.dwg", crossing.Fixture, crossing.From, crossing.To)));
  }
}

// AutoCAD's saves of both fixtures from every release to every other, named <fixture>-r<from>-as-r<to>.dwg, in the
// folder DWGIO_TABLE_ORACLE names; R14 is left out, whose dictionaries AutoCAD names otherwise.
TEST_CASE("releases/table/roundtrip: every table fixture matches AutoCAD's save in each release" *
          doctest::skip(std::getenv("DWGIO_TABLE_ORACLE") == nullptr)) // NOLINT(concurrency-mt-unsafe): one thread
{
  const char *folder = std::getenv("DWGIO_TABLE_ORACLE"); // NOLINT(concurrency-mt-unsafe): one thread
  for (const std::string_view fixture : {"table", "cells"})
  {
    for (std::size_t from = 1; from < FixtureSuffixes.size(); ++from)
    {
      for (std::size_t to = 1; to < FixtureSuffixes.size(); ++to)
      {
        const std::filesystem::path oracle =
            std::filesystem::path{folder} /
            std::format("{}-{}-as-{}.dwg", fixture, FixtureSuffixes.at(from), FixtureSuffixes.at(to));
        if (from == to || !std::filesystem::exists(oracle))
        {
          continue;
        }
        CAPTURE(oracle.string());
        const Drawing ours =
            SavedAs(LoadDrawing(std::format("{}-{}.dwg", fixture, FixtureSuffixes.at(from))), FixtureReleases.at(to));
        SameTable(ours, LoadClean(ReadBytes(oracle.string())));
      }
    }
  }
}

TEST_CASE("releases/table/roundtrip: a table without content gets one built from its grid")
{
  for (const std::string_view from : {"r2007", "r2004"})
  {
    CAPTURE(from);
    const Drawing bare = WithoutTableContent(LoadDrawing(std::format("cells-{}.dwg", from)));
    CHECK(CountOf<Objects::TableContent>(bare) == 0);
    const Drawing ours = SavedAs(bare, Version::R2018);
    SameTable(ours, LoadDrawing(std::format("cells-{}-bare-as-r2018.dwg", from)));
  }
}

TEST_CASE("releases/table/roundtrip: every table fixture saved to every other release loads back with its table")
{
  for (const std::string_view fixture : {"table", "cells"})
  {
    for (std::size_t from = 0; from < FixtureSuffixes.size(); ++from)
    {
      const Drawing drawing = LoadDrawing(std::format("{}-{}.dwg", fixture, FixtureSuffixes.at(from)));
      for (std::size_t to = 0; to < FixtureSuffixes.size(); ++to)
      {
        if (from == to)
        {
          continue;
        }
        CAPTURE(fixture);
        CAPTURE(FixtureSuffixes.at(from));
        CAPTURE(FixtureSuffixes.at(to));
        const std::expected<Saved, Failure> saved = Save(drawing, SaveOptions{.Release = FixtureReleases.at(to)});
        REQUIRE(saved);
        CHECK(std::ranges::none_of(saved->Issues, [](const Issue &issue)
                                   { return issue.What.contains("TABLE") || issue.What.contains("table"); }));
        const Drawing back = LoadClean(saved->Bytes);
        CHECK(CountOf<Entities::AcadTable>(back) == 1);
        CHECK(CountOf<Objects::TableContent>(back) == 1);
        CHECK(CountOf<Objects::TableGeometry>(back) == 1);
        CHECK(CountOf<Objects::CellStyleMap>(back) == (FixtureReleases.at(to) <= Version::R2007 ? 1 : 0));
        CHECK(CountOf<Objects::DataTable>(back) == (FixtureReleases.at(to) <= Version::R2004 ? 1 : 0));
        QueueAudit(saved->Bytes,
                   std::format("dwgio-{}-{}-as-{}.dwg", fixture, FixtureSuffixes.at(from), FixtureSuffixes.at(to)));
      }
    }
  }
}

TEST_CASE("releases/table/roundtrip: a table upgraded from R2007 to R2018 keeps every unmerged cell's text")
{
  const Drawing from = LoadDrawing("cells-r2007.dwg");
  const Entities::AcadTable grid = TableOf(from);
  const Entities::AcadTable upgraded = TableOf(SavedAs(from, Version::R2018));
  REQUIRE(upgraded.Content.Rows.size() == grid.Heights.size());
  std::size_t texts = 0;
  for (std::size_t at = 0; at < grid.Cells.size(); ++at)
  {
    const Entities::AcadTables::Cell &cell = grid.Cells[at];
    if (cell.Merged)
    {
      continue;
    }
    const Objects::TableContents::Cell &made =
        upgraded.Content.Rows[at / grid.Widths.size()].Cells[at % grid.Widths.size()];
    REQUIRE(made.Items.size() == 1);
    CHECK(Table::TextOf(made.Items.front().Value) == Table::TextOf(cell.Value));
    texts += Table::TextOf(cell.Value).empty() ? 0U : 1U;
  }
  CHECK(texts == 8);
}

TEST_CASE("releases/table/roundtrip: the drawings the cases saved pass AutoCAD's AUDIT" * doctest::skip(!IsVerifying()))
{
  CheckQueuedAudits();
}
} // namespace DwgIO::Releases
