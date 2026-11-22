/***********************************************************************************************************************
 *  @file      source/file/paged/head.cpp
 *  @brief     The file header of a paged file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/paged/head.hpp"

#include "bits/crc32.hpp"
#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "file/paged/magic.hpp"
#include "octets.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace DwgIO::File::Paged
{
namespace
{
constexpr std::string_view FileId = "AcFssFcAJMB";
constexpr std::size_t IdSize = 12;
/**
 *  The id, a zero long, 0x6c and 4 open the record.
 */
constexpr std::size_t Opening = IdSize + 12;
constexpr std::size_t MaintenanceAt = 0x0B;
constexpr std::size_t SecurityAt = 0x18;
constexpr std::size_t Zeros = 0x54;
/**
 *  Longs AutoCAD writes that the specification leaves unnamed: one after the record size, three before the page map's
 *  id, and one after the VBA project's address.
 */
constexpr std::uint32_t AfterSize = 4;
constexpr std::array<std::uint32_t, 3> BeforeMaps{0x20, 0x80, 0x40};
constexpr std::uint32_t AfterVba = 0x80;

auto MakeUnreadable() -> std::unexpected<Failure>
{
  return std::unexpected(Failure{.Code = FailureCode::Damaged, .What = "paged header unreadable"});
}

/**
 *  The record's bytes, XORed with the magic sequence either way.
 */
auto Mask(std::span<const std::byte> bytes) -> std::vector<std::byte>
{
  const std::vector<std::byte> magic = MakeMagic(RecordSize);
  std::vector<std::byte> out(RecordSize);
  for (std::size_t at = 0; at < RecordSize; ++at)
  {
    out[at] = bytes[at] ^ magic[at];
  }
  return out;
}

auto ReadRecord(std::span<const std::byte> plain) -> std::expected<Record, Failure>
{
  if (!std::ranges::equal(plain.first(FileId.size()), std::as_bytes(std::span{FileId})))
  {
    return MakeUnreadable();
  }
  Bits::Reader in{plain.subspan(Opening)};
  Record record;
  record.RootGap = in.RL();
  record.LeftGap = in.RL();
  record.RightGap = in.RL();
  record.Unknown1 = in.RL();
  record.LastPageId = in.RL();
  record.LastPageEnd = in.RLL();
  record.SecondHeader = in.RLL();
  record.GapCount = in.RL();
  record.PageCount = in.RL();
  static_cast<void>(in.RL());
  static_cast<void>(in.RL());
  static_cast<void>(in.RL());
  record.PageMapId = in.RL();
  record.PageMapAddress = in.RLL();
  record.SectionMapId = in.RL();
  record.PageArraySize = in.RL();
  record.GapArraySize = in.RL();
  record.Crc = in.RL();
  return record;
}

/**
 *  The record's plain bytes with the CRC the fields give, or zero in its place when asked.
 */
auto WriteRecord(const Record &record, bool crc) -> std::vector<std::byte>
{
  Bits::Writer out;
  out.Raw(std::as_bytes(std::span{FileId}));
  out.RC(0);
  out.RL(0);
  out.RL(static_cast<std::uint32_t>(RecordSize));
  out.RL(AfterSize);
  out.RL(record.RootGap);
  out.RL(record.LeftGap);
  out.RL(record.RightGap);
  out.RL(record.Unknown1);
  out.RL(record.LastPageId);
  out.RLL(record.LastPageEnd);
  out.RLL(record.SecondHeader);
  out.RL(record.GapCount);
  out.RL(record.PageCount);
  for (const std::uint32_t one : BeforeMaps)
  {
    out.RL(one);
  }
  out.RL(record.PageMapId);
  out.RLL(record.PageMapAddress);
  out.RL(record.SectionMapId);
  out.RL(record.PageArraySize);
  out.RL(record.GapArraySize);
  out.RL(crc ? record.Crc : 0);
  return out.Take();
}

auto CrcOf(const Record &record) -> std::uint32_t
{
  return Bits::Crc32(WriteRecord(record, false), 0);
}
} // namespace

auto ReadHead(std::span<const std::byte> file, std::vector<Issue> &issues) -> std::expected<Head, Failure>
{
  if (file.size() < Pages)
  {
    return MakeUnreadable();
  }
  const std::optional<Version> release = ReleaseOf(TextOf(file.first(6)));
  if (!release)
  {
    return MakeUnreadable();
  }
  Head head;
  head.Release = *release;
  Bits::Reader in{file.subspan(MaintenanceAt)};
  head.Maintenance = in.RC();
  head.Flag = in.RC();
  head.Image = in.RL();
  head.DwgVersion = in.RC();
  head.MaintRelease = in.RC();
  head.Codepage = in.RS();
  for (std::uint8_t &one : head.Repeat)
  {
    one = in.RC();
  }
  head.Security = in.RL();
  head.Unknown1 = in.RL();
  head.Summary = in.RL();
  head.Vba = in.RL();
  static_cast<void>(in.RL());
  const std::vector<std::byte> extra = in.Bytes(Zeros);
  std::ranges::copy(extra, head.Extra.begin());
  if (head.Security != 0)
  {
    return std::unexpected(
        Failure{.Code = FailureCode::Encrypted, .Offset = SecurityAt, .What = "encrypted drawing not read"});
  }
  const std::vector<std::byte> plain = Mask(file.subspan(RecordAt, RecordSize));
  std::expected<Record, Failure> record = ReadRecord(plain);
  if (!record)
  {
    return std::unexpected(record.error());
  }
  head.Record = *record;
  if (CrcOf(*record) != record->Crc)
  {
    issues.push_back(Issue{.Code = IssueCode::Checksum, .Offset = RecordAt, .What = "header CRC mismatch"});
  }
  const std::uint64_t at = record->SecondHeader;
  if (at > file.size() || file.size() - at < RecordSize ||
      !std::ranges::equal(file.subspan(at, RecordSize), file.subspan(RecordAt, RecordSize)))
  {
    issues.push_back(Issue{.Code = IssueCode::Damaged,
                           .Offset = static_cast<std::size_t>(std::min<std::uint64_t>(at, file.size())),
                           .What = "second header copy differs"});
  }
  return head;
}

auto WriteCopy(const Head &head) -> std::vector<std::byte>
{
  Record record = head.Record;
  record.Crc = CrcOf(record);
  return Mask(WriteRecord(record, true));
}

auto WriteHead(const Head &head) -> std::vector<std::byte>
{
  Bits::Writer out;
  out.Raw(std::as_bytes(std::span{AcadverOf(head.Release)}));
  out.Raw(std::array<std::byte, 5>{});
  out.RC(head.Maintenance);
  out.RC(head.Flag);
  out.RL(head.Image);
  out.RC(head.DwgVersion);
  out.RC(head.MaintRelease);
  out.RS(head.Codepage);
  for (const std::uint8_t one : head.Repeat)
  {
    out.RC(one);
  }
  out.RL(head.Security);
  out.RL(head.Unknown1);
  out.RL(head.Summary);
  out.RL(head.Vba);
  out.RL(AfterVba);
  out.Raw(head.Extra);
  out.Raw(WriteCopy(head));
  const std::vector<std::byte> magic = MakeMagic(Pages);
  out.Raw(std::span{magic}.subspan(RecordAt + RecordSize));
  return out.Take();
}
} // namespace DwgIO::File::Paged
