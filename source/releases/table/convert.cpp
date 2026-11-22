/***********************************************************************************************************************
 *  @file      source/releases/table/convert.cpp
 *  @brief     The tables a save writes in the form of each release, and the records they keep.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "releases/table/convert.hpp"

#include "encoding/codepage.hpp"
#include "encoding/encoding.hpp"
#include "releases/lookup.hpp"
#include "releases/table/content.hpp"

#include "dwgio/base.hpp"
#include "dwgio/drawing.hpp"
#include "dwgio/edit.hpp"
#include "dwgio/entities/table.hpp"
#include "dwgio/handle.hpp"
#include "dwgio/object.hpp"
#include "dwgio/objects/cellstylemap.hpp"
#include "dwgio/objects/datatable.hpp"
#include "dwgio/objects/dictionary.hpp"
#include "dwgio/objects/tablecontent.hpp"
#include "dwgio/objects/tablegeometry.hpp"
#include "dwgio/objects/tablestyles/style.hpp"
#include "dwgio/objects/xrecord.hpp"
#include "dwgio/tag.hpp"
#include "dwgio/version.hpp"

#include <algorithm>
#include <array>
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

namespace DwgIO::Releases::Table
{
namespace
{
/**
 *  The entries and markers AutoCAD keeps a table's and a table style's objects under.
 */
constexpr std::string_view Roundtrip = "ACAD_XREC_ROUNDTRIP";
constexpr std::string_view Mapped = "ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP";
constexpr std::string_view Entity = "ACAD_ROUNDTRIP_2008_TABLE_ENTITY";
constexpr std::string_view Older = "ACAD_ROUNDTRIP_PRE2007_TABLE";
constexpr std::string_view Celled = "ACAD_ROUNDTRIP_PRE2007_TABLECELL";
constexpr std::string_view Styled = "ACAD_ROUNDTRIP_PRE2007_TABLESTYLE";

/**
 *  The group codes of the records: a marker, a hard owner, the content's and the geometry's, and the longs of the rows
 *  and columns.
 */
constexpr int Marker = 102;
constexpr int Owner = 360;
constexpr int Shaped = 361;
constexpr int Rows = 90;
constexpr int Columns = 91;

/**
 *  A DATATABLE's version and the types of its columns: a double, a long and a hard owner, with their groups.
 */
constexpr int Version2 = 2;
constexpr int RealKind = 2;
constexpr int LongKind = 1;
constexpr int OwnerKind = 6;
constexpr int RealGroup = 40;
constexpr int LongGroup = 93;

/**
 *  The group codes a style's record gives each of its three cells' data type, unit and format.
 */
constexpr std::array<std::array<int, 3>, 3> Typed{{{90, 91, 1}, {92, 93, 2}, {94, 95, 3}}};

/**
 *  The index of the group marking a part of a record, or its size.
 */
auto MarkerAt(const Objects::XRecord &record, std::string_view name) -> std::size_t
{
  const DwgIO::Content marker{std::string{name}};
  const auto found = std::ranges::find_if(record.Data, [&marker](const Tag &group) -> bool
                                          { return group.Code == Marker && group.Value == marker; });
  return static_cast<std::size_t>(found - record.Data.begin());
}

/**
 *  The handle of the code after a record's marker, or null.
 */
auto HandleAfter(const Objects::XRecord &record, std::string_view name, int code) -> Handle
{
  for (std::size_t at = MarkerAt(record, name); at < record.Data.size(); ++at)
  {
    const auto *handle = std::get_if<Handle>(&record.Data[at].Value);
    if (handle != nullptr && record.Data[at].Code == code)
    {
      return *handle;
    }
  }
  return {};
}

/**
 *  The checksum of a cell of a content, 0 for one with no content.
 */
auto ChecksumOf(const Objects::TableContents::Cell &cell) -> double
{
  return cell.Items.empty() ? 0.0 : Table::Checksum(Table::TextOf(cell.Items.front().Value));
}

/**
 *  An extension dictionary made for an object, owning hard its one entry, as AutoCAD makes it.
 */
auto MakeExtensionDictionary(Handle dictionary, Handle object, Objects::Entry entry) -> Objects::Dictionary
{
  Objects::Dictionary made{
      .Base = {.Self = dictionary, .Owner = object}, .HardOwner = true, .Cloning = 1, .Entries = {}, .Default = {}};
  made.Entries.push_back(std::move(entry));
  return made;
}

/**
 *  A drawing's tables and table styles made over for a release.
 */
class TableConverter
{
public:
  TableConverter(Drawing &drawing, Version to)
      : m_Drawing{&drawing}, m_Span{.From = drawing.Release, .To = to}, m_Scheme{Encoding::SchemeOf(drawing.Codepage)}
  {
  }

  /**
   *  A table in the release's form, read as AutoCAD reads it, and its records made or taken away; a table without its
   *  content and geometry read from its own form, and given them.
   */
  void Table(Handle self)
  {
    auto &table = DwgIO::Get<Entities::AcadTable>(*Find(*m_Drawing, self));
    Objects::XRecord *record = RecordOf(table.Base);
    const auto *content = record != nullptr ? Get<Objects::TableContent>(HandleAfter(*record, Entity, Owner)) : nullptr;
    const auto *geometry =
        record != nullptr ? Get<Objects::TableGeometry>(HandleAfter(*record, Entity, Shaped)) : nullptr;
    const bool held = content != nullptr && geometry != nullptr;
    Objects::TableContents::Data memory;
    if (held)
    {
      memory = Table::ReadContent(
          table, Table::Held{.Content = &content->Data, .Geometry = geometry, .From = m_Span.From}, m_Scheme);
    }
    else
    {
      memory = m_Span.From >= Version::R2010 ? table.Content : Table::MakeContent(table, m_Span.From);
    }
    if (m_Span.To >= Version::R2010)
    {
      Table::ToContentForm(table, memory, m_Span);
    }
    else
    {
      Table::ToGridForm(table, memory, m_Span);
    }
    if (held)
    {
      Records(*record, memory);
      return;
    }
    Objects::XRecord made = MakeTableRecord(table, memory);
    Records(made, memory);
    m_Made.emplace_back(std::move(made));
  }

  /**
   *  A content's values in the release's form.
   */
  void Content(Handle self)
  {
    if ((m_Span.From >= Version::R2007) != (m_Span.To >= Version::R2007))
    {
      Table::Recode(DwgIO::Get<Objects::TableContent>(*Find(*m_Drawing, self)).Data, m_Span.To, m_Scheme);
    }
  }

  /**
   *  A table style's cell style map and record made or taken away.
   */
  void Style(Handle self)
  {
    // Copied before anything is made, which moves the drawing's objects.
    const auto &style = DwgIO::Get<Objects::TableStyle>(*Find(*m_Drawing, self));
    std::vector<Objects::TableStyles::NamedCellStyle> styles = style.Styles;
    std::vector<Tag> types = Types(style);
    if (m_Span.To < Version::R2010 && m_Span.From >= Version::R2010)
    {
      Keep(self, Mapped, Objects::CellStyleMap{.Base = {}, .Styles = std::move(styles)});
    }
    if (m_Span.To >= Version::R2010 && m_Span.From < Version::R2010)
    {
      // By its kind: R14 names the entry otherwise, cut to 26 characters. Its cell styles become the style's, as
      // AutoCAD reads them, a style of the user's among them.
      Drop(self,
           [this, self](const Objects::Entry &entry) -> bool
           {
             const auto *map = Get<Objects::CellStyleMap>(entry.Object);
             if (map == nullptr)
             {
               return false;
             }
             DwgIO::Get<Objects::TableStyle>(*Find(*m_Drawing, self)).Styles = map->Styles;
             return true;
           });
    }
    if (m_Span.To < Version::R2007 && m_Span.From >= Version::R2007)
    {
      Keep(self, Roundtrip,
           Objects::XRecord{.Base = {}, .Cloning = 1, .Data = std::move(types), .Objects = {}, .Rest = {}});
    }
    if (m_Span.To >= Version::R2007 && m_Span.From < Version::R2007)
    {
      Drop(self, [](const Objects::Entry &entry) -> bool { return entry.Name == Roundtrip; });
    }
  }

  /**
   *  The drawing without the objects taken away and with those made.
   */
  void Finish()
  {
    std::erase_if(m_Drawing->Objects,
                  [this](const Object &object) -> bool { return m_Gone.contains(BaseOf(object).Self.Value); });
    for (Object &made : m_Made)
    {
      m_Drawing->Objects.push_back(std::move(made));
    }
  }

private:
  /**
   *  The object of a kind under that handle, or null.
   */
  template <typename KindT>
  auto Get(Handle self) -> KindT *
  {
    return GetIf<KindT>(Find(*m_Drawing, self));
  }

  /**
   *  The extension dictionary of an object, or null.
   */
  auto DictionaryOf(const Base &base) -> Objects::Dictionary * { return Get<Objects::Dictionary>(base.Dictionary); }

  /**
   *  The record of an object's extension dictionary under ACAD_XREC_ROUNDTRIP, or null.
   */
  auto RecordOf(const Base &base) -> Objects::XRecord *
  {
    Objects::Dictionary *dictionary = DictionaryOf(base);
    if (dictionary == nullptr)
    {
      return nullptr;
    }
    const auto entry = std::ranges::find(dictionary->Entries, Roundtrip, &Objects::Entry::Name);
    return entry == dictionary->Entries.end() ? nullptr : Get<Objects::XRecord>(entry->Object);
  }

  /**
   *  The record AutoCAD keeps a table's content and geometry under, both made from the content given, in the table's
   *  extension dictionary, made too where it has none; a record already there is taken away.
   */
  auto MakeTableRecord(Entities::AcadTable &table, const Objects::TableContents::Data &memory) -> Objects::XRecord
  {
    constexpr int VersionCode = 70;
    constexpr int LongCode = 90;
    constexpr int Kept = 2;
    constexpr int LastLong = 5;
    constexpr std::array<int, 3> Point{10, 20, 30};
    Objects::Dictionary *dictionary = DictionaryOf(table.Base);
    const Handle owner = dictionary != nullptr ? table.Base.Dictionary : Next();
    const Handle self = Next();
    if (dictionary == nullptr)
    {
      table.Base.Dictionary = owner;
      m_Made.emplace_back(MakeExtensionDictionary(owner, table.Base.Self,
                                                  Objects::Entry{.Name = std::string{Roundtrip}, .Object = self}));
    }
    else
    {
      std::erase_if(dictionary->Entries,
                    [this](const Objects::Entry &entry) -> bool
                    {
                      if (entry.Name != Roundtrip)
                      {
                        return false;
                      }
                      m_Gone.insert(entry.Object.Value);
                      return true;
                    });
      dictionary->Entries.push_back(Objects::Entry{.Name = std::string{Roundtrip}, .Object = self});
    }
    const Handle stored = Next();
    const Handle placed = Next();
    Objects::TableContent content{.Base = {.Self = stored, .Owner = self},
                                  .Data = Table::MakeStoredContent(memory, table.Style)};
    if (m_Span.To < Version::R2007)
    {
      Table::Recode(content.Data, m_Span.To, m_Scheme);
    }
    Objects::TableGeometry geometry = Table::MakeGeometry(memory);
    geometry.Base = Base{.Self = placed, .Owner = self};
    m_Made.emplace_back(std::move(content));
    m_Made.emplace_back(std::move(geometry));
    std::vector<Tag> groups{Tag{.Code = Marker, .Value = std::string{Entity}}, Tag{.Code = Owner, .Value = stored},
                            Tag{.Code = VersionCode, .Value = std::int64_t{Kept}},
                            Tag{.Code = LongCode, .Value = std::int64_t{1}}};
    for (const int code : Point)
    {
      groups.push_back(Tag{.Code = code, .Value = 0.0});
    }
    groups.push_back(Tag{.Code = LongCode, .Value = std::int64_t{0}});
    groups.push_back(Tag{.Code = LongCode, .Value = std::int64_t{LastLong}});
    groups.push_back(Tag{.Code = Shaped, .Value = placed});
    return Objects::XRecord{.Base = {.Self = self, .Owner = owner, .Reactors = {owner}},
                            .Cloning = 1,
                            .Data = std::move(groups),
                            .Objects = {},
                            .Rest = {}};
  }

  /**
   *  A table's record with the part the releases before R2007 add, and its DATATABLE made from the content, below
   *  R2007; without them from it.
   */
  void Records(Objects::XRecord &record, const Objects::TableContents::Data &memory)
  {
    const Handle older = HandleAfter(record, Celled, Owner);
    if (const auto *data = Get<Objects::DataTable>(older))
    {
      m_Gone.insert(older.Value);
      for (const Objects::DataTables::Column &column : data->Columns)
      {
        for (const Tag &cell : column.Cells)
        {
          if (const auto *handle = std::get_if<Handle>(&cell.Value))
          {
            m_Gone.insert(handle->Value);
          }
        }
      }
    }
    record.Data.erase(record.Data.begin() + static_cast<std::ptrdiff_t>(MarkerAt(record, Older)), record.Data.end());
    if (m_Span.To >= Version::R2007)
    {
      return;
    }
    const Handle table = Next();
    std::vector<std::vector<Tag>> values = Table::MakeDataCells(memory);
    Objects::DataTable data{
        .Base = {.Self = table, .Owner = record.Base.Self},
        .Version = Version2,
        .Rows = static_cast<std::int32_t>(values.size()),
        .Name = {},
        .Columns = {Objects::DataTables::Column{.Kind = RealKind}, Objects::DataTables::Column{.Kind = LongKind},
                    Objects::DataTables::Column{.Kind = LongKind}, Objects::DataTables::Column{.Kind = OwnerKind}}};
    for (const Objects::TableContents::Row &row : memory.Rows)
    {
      for (const Objects::TableContents::Cell &cell : row.Cells)
      {
        data.Columns[0].Cells.push_back(Tag{.Code = RealGroup, .Value = ChecksumOf(cell)});
        data.Columns[1].Cells.push_back(Tag{.Code = LongGroup, .Value = std::int64_t{0}});
        data.Columns[2].Cells.push_back(Tag{.Code = LongGroup, .Value = std::int64_t{0}});
      }
    }
    std::vector<Object> cells;
    for (std::vector<Tag> &groups : values)
    {
      const Handle made = Next();
      data.Columns[3].Cells.push_back(Tag{.Code = Owner, .Value = made});
      cells.emplace_back(Objects::XRecord{
          .Base = {.Self = made, .Owner = table}, .Cloning = 1, .Data = std::move(groups), .Objects = {}, .Rest = {}});
    }
    m_Made.emplace_back(std::move(data));
    std::ranges::move(cells, std::back_inserter(m_Made));
    const auto rows = static_cast<std::int64_t>(memory.Rows.size());
    const auto columns = static_cast<std::int64_t>(memory.Columns.size());
    record.Data.insert(record.Data.end(),
                       {Tag{.Code = Marker, .Value = std::string{Older}}, Tag{.Code = Rows, .Value = rows},
                        Tag{.Code = Columns, .Value = columns}, Tag{.Code = Marker, .Value = std::string{Celled}},
                        Tag{.Code = Owner, .Value = table}});
  }

  /**
   *  The record below R2007 of a style's cells' data types, units and formats, in the order of its cells.
   */
  [[nodiscard]] auto Types(const Objects::TableStyle &style) const -> std::vector<Tag>
  {
    std::vector<Objects::TableStyles::RowStyle> cells = style.Cells;
    if (m_Span.From >= Version::R2010)
    {
      // The cells up to R2007 list the data, title and header rows; the cell styles number them 3, 1 and 2.
      constexpr std::array<int, 3> Ids{3, 1, 2};
      cells.assign(Ids.size(), Objects::TableStyles::RowStyle{});
      for (std::size_t at = 0; at < Ids.size(); ++at)
      {
        const auto named = std::ranges::find(style.Styles, Ids.at(at), &Objects::TableStyles::NamedCellStyle::Id);
        if (named != style.Styles.end())
        {
          cells[at].Type = named->Style.Content.Type;
          cells[at].Unit = named->Style.Content.Unit;
          cells[at].Format = named->Style.Content.Format;
        }
      }
    }
    cells.resize(Typed.size());
    std::vector<Tag> groups{Tag{.Code = Marker, .Value = std::string{Styled}}};
    for (std::size_t at = 0; at < Typed.size(); ++at)
    {
      groups.push_back(Tag{.Code = Typed.at(at)[0], .Value = std::int64_t{cells[at].Type}});
      groups.push_back(Tag{.Code = Typed.at(at)[1], .Value = std::int64_t{cells[at].Unit}});
      groups.push_back(Tag{.Code = Typed.at(at)[2], .Value = cells[at].Format});
    }
    return groups;
  }

  /**
   *  The object of an entry of the style's extension dictionary made, the dictionary made too where the style has none,
   *  as AutoCAD makes them, and put among the drawing's objects at once for the next entry to find; an entry already
   *  there is kept.
   */
  void Keep(Handle self, std::string_view name, Object kind)
  {
    Base &base = BaseOf(*Find(*m_Drawing, self));
    Objects::Dictionary *dictionary = DictionaryOf(base);
    if (dictionary != nullptr && std::ranges::contains(dictionary->Entries, name, &Objects::Entry::Name))
    {
      return;
    }
    const Handle owner = dictionary != nullptr ? base.Dictionary : Next();
    const Handle made = Next();
    BaseOf(kind) = Base{.Self = made, .Owner = owner, .Reactors = {owner}};
    m_Made.emplace_back(std::move(kind));
    if (dictionary != nullptr)
    {
      dictionary->Entries.push_back(Objects::Entry{.Name = std::string{name}, .Object = made});
      return;
    }
    base.Dictionary = owner;
    m_Drawing->Objects.emplace_back(
        MakeExtensionDictionary(owner, self, Objects::Entry{.Name = std::string{name}, .Object = made}));
  }

  /**
   *  The objects of the entries of the style's extension dictionary the predicate picks taken away with their entries;
   *  the dictionary stays, as AutoCAD keeps it.
   */
  template <typename PickT>
  void Drop(Handle self, const PickT &pick)
  {
    Objects::Dictionary *dictionary = DictionaryOf(BaseOf(*Find(*m_Drawing, self)));
    if (dictionary == nullptr)
    {
      return;
    }
    std::erase_if(dictionary->Entries,
                  [this, &pick](const Objects::Entry &entry) -> bool
                  {
                    if (!pick(entry))
                    {
                      return false;
                    }
                    m_Gone.insert(entry.Object.Value);
                    return true;
                  });
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
  Table::Span m_Span;
  Encoding::Scheme m_Scheme;
  std::unordered_set<std::uint64_t> m_Gone;
  std::vector<Object> m_Made;
  bool m_Seeded{};
};
} // namespace

void ConvertTables(Remake &remade, Version to)
{
  const Drawing &drawing = remade.Now();
  if (drawing.Release == to)
  {
    return;
  }
  std::vector<Handle> tables;
  std::vector<Handle> contents;
  std::vector<Handle> styles;
  const Lookup found{drawing};
  for (const Object &object : drawing.Objects)
  {
    const Handle self = BaseOf(object).Self;
    if (!found.First(object))
    {
      continue; // Of no handle, or of one another object bears first, as a broken file holds: not found to make over.
    }
    if (Holds<Entities::AcadTable>(object))
    {
      tables.push_back(self);
    }
    else if (Holds<Objects::TableContent>(object))
    {
      contents.push_back(self);
    }
    else if (Holds<Objects::TableStyle>(object))
    {
      styles.push_back(self);
    }
  }
  if (tables.size() + contents.size() + styles.size() == 0)
  {
    return;
  }
  Drawing &made = remade.Change();
  TableConverter tabling{made, to};
  for (const Handle self : tables)
  {
    tabling.Table(self);
  }
  for (const Handle self : contents)
  {
    tabling.Content(self);
  }
  for (const Handle self : styles)
  {
    tabling.Style(self);
  }
  tabling.Finish();
}

} // namespace DwgIO::Releases::Table
