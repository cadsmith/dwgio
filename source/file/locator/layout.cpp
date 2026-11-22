/***********************************************************************************************************************
 *  @file      source/file/locator/layout.cpp
 *  @brief     A locator file cut into its blocks.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/layout.hpp"

#include "bits/reader.hpp"
#include "file/locator/section.hpp"
#include "file/locator/sentinels.hpp"
#include "file/names.hpp"
#include "objects/map.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>

namespace DwgIO::File::Locator
{
namespace
{
constexpr std::uint8_t HeaderRecord = 0;

constexpr std::uint8_t ClassesRecord = 1;

constexpr std::uint8_t MapRecord = 2;

constexpr std::uint8_t MeasurementRecord = 4;

constexpr std::uint8_t AuxiliaryRecord = 5;

constexpr std::array Required{HeaderRecord, ClassesRecord, MapRecord};

constexpr std::size_t Lead = Sentinel + 4;

/**
 *  A block with the extent it covers in the file, from its first byte, a sectioned block's frame included.
 */
struct Piece
{
  Located Block;
  std::size_t Start{};
  std::size_t Extent{};

  [[nodiscard]] auto End() const -> std::size_t { return Start + Extent; }
};

/**
 *  Whether a block is placed by its own sentinel rather than a locator record: a wrong one is dropped, where two
 *  located blocks over each other fail.
 */
auto IsInferred(std::string_view name) -> bool
{
  return name == Names::Preview || name == Names::Second;
}

struct Cutter
{
  std::span<const std::byte> File;
  std::vector<Issue> *Issues{};
  std::vector<Piece> Pieces;

  /**
   *  Whether the block lies inside the file; one beyond it is noted and left out.
   */
  [[nodiscard]] auto Inside(std::string_view name, std::size_t offset, std::size_t size) const -> bool
  {
    if (offset > File.size() || size > File.size() - offset)
    {
      Issues->push_back(
          Issue{.Code = IssueCode::Damaged, .Offset = offset, .What = std::format("{} block beyond the file", name)});
      return false;
    }
    return true;
  }

  /**
   *  Keeps a block inside the file as it is.
   */
  void Raw(std::string name, std::size_t offset, std::size_t size)
  {
    if (!Inside(name, offset, size))
    {
      return;
    }
    const std::span<const std::byte> bytes = File.subspan(offset, size);
    Pieces.push_back(Piece{.Block = {.Name = std::move(name), .Offset = offset, .Bytes = {bytes.begin(), bytes.end()}},
                           .Start = offset,
                           .Extent = size});
  }

  /**
   *  Keeps a sectioned block's body, placed where the body starts; its frame beyond the file fails.
   */
  auto KeepSectioned(const Record &where, const Marks &marks) -> std::expected<void, Failure>
  {
    std::expected<Framed, Failure> framed = Cut(File, where, marks);
    if (!framed)
    {
      return std::unexpected(framed.error());
    }
    Issues->insert(Issues->end(), framed->Issues.begin(), framed->Issues.end());
    const std::size_t extent = framed->Body.size() + marks.Framed;
    Pieces.push_back(Piece{.Block = {.Name = std::string{marks.Name},
                                     .Offset = static_cast<std::size_t>(where.Seeker) + Lead,
                                     .Bytes = std::move(framed->Body)},
                           .Start = where.Seeker,
                           .Extent = extent});
    return {};
  }

  /**
   *  Keeps a block framed by its sentinels whose RL says how long it is, the preview or the second header; one whose
   *  sentinel is missing is noted and left out.
   */
  void KeepMarked(std::string name, std::size_t offset, const Marks &marks)
  {
    if (offset + Lead > File.size() || !Matches(File.subspan(offset), marks.Begin))
    {
      Issues->push_back(
          Issue{.Code = IssueCode::Damaged, .Offset = offset, .What = std::format("{} sentinel missing", marks.Name)});
      return;
    }
    Bits::Reader in{File.subspan(offset + Sentinel)};
    Raw(std::move(name), offset, in.RL() + marks.Framed);
  }

  [[nodiscard]] auto Covers(std::size_t offset) const -> bool
  {
    return std::ranges::any_of(Pieces, [offset](const Piece &piece) -> bool
                               { return piece.Start <= offset && offset < piece.End(); });
  }

  /**
   *  Where the first piece at or after the byte starts, or the file ends.
   */
  [[nodiscard]] auto Next(std::size_t from) const -> std::size_t
  {
    std::size_t next = File.size();
    for (const Piece &piece : Pieces)
    {
      if (piece.Start >= from)
      {
        next = std::min(next, piece.Start);
      }
    }
    return next;
  }
};

auto Find(const Head &head, std::uint8_t number) -> const Record *
{
  const auto found = std::ranges::find(head.Records, number, &Record::Number);
  return found == head.Records.end() ? nullptr : &*found;
}

/**
 *  Drops a block whose sentinel put it over another, with a note; two located blocks over each other fail.
 */
auto Disjoint(Cutter &cut, const Head &head) -> std::expected<void, Failure>
{
  std::ranges::sort(cut.Pieces, {}, &Piece::Start);
  std::vector<Piece> kept{
      Piece{.Block = {.Name = "head", .Offset = 0, .Bytes = {}}, .Start = 0, .Extent = SizeOf(head)}};
  const auto drop = [&cut](const Piece &piece) -> void
  {
    cut.Issues->push_back(Issue{.Code = IssueCode::Damaged,
                                .Offset = piece.Start,
                                .What = std::format("{} block overlaps another", piece.Block.Name)});
  };
  for (Piece &piece : cut.Pieces)
  {
    bool keep = true;
    while (keep && piece.Start < kept.back().End())
    {
      if (IsInferred(piece.Block.Name))
      {
        drop(piece);
        keep = false;
      }
      else if (IsInferred(kept.back().Block.Name))
      {
        drop(kept.back());
        kept.pop_back();
      }
      else
      {
        return std::unexpected(Failure{.Code = FailureCode::Damaged,
                                       .Offset = piece.Start,
                                       .What = std::format("{} block overlaps the one before", piece.Block.Name)});
      }
    }
    if (keep)
    {
      kept.push_back(std::move(piece));
    }
  }
  cut.Pieces.assign(std::make_move_iterator(kept.begin() + 1), std::make_move_iterator(kept.end()));
  return {};
}

/**
 *  The lowest map offset the pieces leave free, where the objects start; nothing when the map gives none.
 */
auto Lowest(const Cutter &cut, const Head &head) -> std::optional<std::size_t>
{
  const auto map = std::ranges::find(cut.Pieces, std::string_view{Names::Map},
                                     [](const Piece &piece) -> std::string_view { return piece.Block.Name; });
  std::optional<std::size_t> lowest;
  if (map == cut.Pieces.end())
  {
    return lowest;
  }
  std::vector<Issue> scratch;
  for (const Objects::Mapped &entry : Objects::ReadMap(map->Block.Bytes, map->Block.Offset, scratch))
  {
    if (entry.Offset >= SizeOf(head) && !cut.Covers(entry.Offset) && entry.Offset < lowest.value_or(cut.File.size()))
    {
      lowest = entry.Offset;
    }
  }
  return lowest;
}

/**
 *  The pieces in file order, with what lies between them: a pad before the objects, unknown bytes elsewhere.
 */
auto Gather(Cutter cut, const Head &head) -> Layout
{
  std::ranges::sort(cut.Pieces, {}, &Piece::Start);
  Layout layout{.Head = head, .Blocks = {}};
  std::size_t cursor = SizeOf(head);
  for (Piece &piece : cut.Pieces)
  {
    if (piece.Start > cursor)
    {
      const std::span<const std::byte> gap = cut.File.subspan(cursor, piece.Start - cursor);
      const bool pad = piece.Block.Name == Names::Objects;
      if (!pad)
      {
        cut.Issues->push_back(Issue{
            .Code = IssueCode::Damaged, .Offset = cursor, .What = std::format("{} bytes in no block", gap.size())});
      }
      layout.Blocks.push_back(Located{.Name = pad ? std::string{Names::Pad} : std::string{Names::Unknown},
                                      .Offset = cursor,
                                      .Bytes = {gap.begin(), gap.end()}});
    }
    cursor = piece.End();
    layout.Blocks.push_back(std::move(piece.Block));
  }
  if (cursor < cut.File.size())
  {
    const std::span<const std::byte> gap = cut.File.subspan(cursor);
    cut.Issues->push_back(
        Issue{.Code = IssueCode::Damaged, .Offset = cursor, .What = std::format("{} bytes in no block", gap.size())});
    layout.Blocks.push_back(
        Located{.Name = std::string{Names::Unknown}, .Offset = cursor, .Bytes = {gap.begin(), gap.end()}});
  }
  return layout;
}
} // namespace

auto ReadLayout(std::span<const std::byte> file, std::vector<Issue> &issues) -> std::expected<Layout, Failure>
{
  const std::expected<Head, Failure> head = ReadHead(file, issues);
  if (!head)
  {
    return std::unexpected(head.error());
  }
  for (const std::uint8_t number : Required)
  {
    if (Find(*head, number) == nullptr)
    {
      return std::unexpected(Failure{
          .Code = FailureCode::Damaged, .Offset = 0x15, .What = std::format("locator record {} missing", number)});
    }
  }
  Cutter cut{.File = file, .Issues = &issues, .Pieces = {}};
  if (const Record *aux = Find(*head, AuxiliaryRecord); aux != nullptr && aux->Size > 0)
  {
    cut.Raw(std::string{Names::Auxiliary}, aux->Seeker, aux->Size);
  }
  if (auto done = cut.KeepSectioned(*Find(*head, HeaderRecord), Sentinels::Header); !done)
  {
    return std::unexpected(done.error());
  }
  if (auto done = cut.KeepSectioned(*Find(*head, ClassesRecord), Sentinels::Classes); !done)
  {
    return std::unexpected(done.error());
  }
  const std::size_t classesEnd = cut.Pieces.back().End();
  const Record &map = *Find(*head, MapRecord);
  if (cut.Inside(Names::Map, map.Seeker, map.Size))
  {
    cut.Raw(std::string{Names::Map}, map.Seeker, map.Size);
    cut.KeepMarked(std::string{Names::Second}, static_cast<std::size_t>(map.Seeker) + map.Size, Sentinels::Second);
  }
  if (const Record *measure = Find(*head, MeasurementRecord); measure != nullptr && measure->Size > 0)
  {
    cut.Raw(std::string{Names::Measurement}, measure->Seeker, measure->Size);
  }
  if (head->Image != 0)
  {
    cut.KeepMarked(std::string{Names::Preview}, head->Image, Sentinels::Preview);
  }
  if (auto done = Disjoint(cut, *head); !done)
  {
    return std::unexpected(done.error());
  }
  const std::size_t start = Lowest(cut, *head).value_or(classesEnd);
  const std::size_t end = cut.Next(start);
  if (end > start)
  {
    cut.Raw(std::string{Names::Objects}, start, end - start);
  }
  return Gather(std::move(cut), *head);
}
} // namespace DwgIO::File::Locator
