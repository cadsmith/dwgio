/***********************************************************************************************************************
 *  @file      source/file/locator/head.cpp
 *  @brief     The file header of an R14 or R2000 file.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/head.hpp"

#include "bits/crc.hpp"
#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "file/locator/sentinels.hpp"
#include "octets.hpp"

#include <format>
#include <string>

namespace DwgIO::File::Locator
{
namespace
{
constexpr std::size_t Code = 6;

constexpr std::size_t Zeros = 5;

constexpr std::size_t Count = 0x15;

constexpr std::size_t First = 0x19;

constexpr std::size_t Each = 9;

constexpr std::size_t Tail = 2 + Sentinels::HeaderEnd.size();

} // namespace

auto ReadHead(std::span<const std::byte> file, std::vector<Issue> &issues) -> std::expected<Head, Failure>
{
  if (file.size() < First)
  {
    return std::unexpected(Failure{.Code = FailureCode::NotDwg, .What = "file too short"});
  }
  const std::optional<Version> release = DwgIO::ReleaseOf(TextOf(file.first(Code)));
  if (!release)
  {
    return std::unexpected(Failure{.Code = FailureCode::NotDwg, .What = "not a DWG file"});
  }
  if (*release >= Version::R2004)
  {
    return std::unexpected(Failure{.Code = FailureCode::UnsupportedRelease,
                                   .What = std::format("release {} is not a locator file", NameOf(*release))});
  }
  Head head;
  head.Release = *release;
  Bits::Reader in{file};
  in.Seek(Code * 8);
  bool zero = true;
  for (std::size_t at = 0; at < Zeros; ++at)
  {
    zero = in.RC() == 0 && zero;
  }
  if (!zero)
  {
    issues.push_back(Issue{.Code = IssueCode::Damaged, .Offset = Code, .What = "file header bytes 6 to 10 not zero"});
  }
  head.Maintenance = in.RC();
  head.Flag = in.RC();
  head.Image = in.RL();
  head.DwgVersion = in.RC();
  head.MaintRelease = in.RC();
  head.Codepage = in.RS();
  const std::uint64_t count = in.RL();
  if (count == 0)
  {
    return std::unexpected(Failure{.Code = FailureCode::Damaged, .Offset = Count, .What = "no locator records"});
  }
  if (First + (count * Each) + Tail > file.size())
  {
    return std::unexpected(
        Failure{.Code = FailureCode::Damaged, .Offset = Count, .What = "locator records run past the end"});
  }
  for (std::uint64_t at = 0; at < count; ++at)
  {
    Record record;
    record.Number = in.RC();
    record.Seeker = in.RL();
    record.Size = in.RL();
    head.Records.push_back(record);
  }
  const std::size_t crcAt = in.Position() / 8;
  if (Bits::Crc(file.first(crcAt), Bits::Seed) != in.RS())
  {
    issues.push_back(Issue{.Code = IssueCode::Checksum, .Offset = crcAt, .What = "file header CRC mismatch"});
  }
  if (!Matches(file.subspan(crcAt + 2), Sentinels::HeaderEnd))
  {
    issues.push_back(Issue{.Code = IssueCode::Damaged, .Offset = crcAt + 2, .What = "file header sentinel mismatch"});
  }
  return head;
}

auto WriteHead(const Head &head) -> std::vector<std::byte>
{
  Bits::Writer out;
  for (const char one : AcadverOf(head.Release))
  {
    out.RC(static_cast<std::uint8_t>(one));
  }
  for (std::size_t at = 0; at < Zeros; ++at)
  {
    out.RC(0);
  }
  out.RC(head.Maintenance);
  out.RC(head.Flag);
  out.RL(head.Image);
  out.RC(head.DwgVersion);
  out.RC(head.MaintRelease);
  out.RS(head.Codepage);
  out.RL(static_cast<std::uint32_t>(head.Records.size()));
  for (const Record &record : head.Records)
  {
    out.RC(record.Number);
    out.RL(record.Seeker);
    out.RL(record.Size);
  }
  out.RS(Bits::Crc(out.Bytes(), Bits::Seed));
  std::vector<std::byte> bytes = out.Take();
  Put(bytes, Sentinels::HeaderEnd);
  return bytes;
}

auto SizeOf(const Head &head) -> std::size_t
{
  return First + (head.Records.size() * Each) + Tail;
}
} // namespace DwgIO::File::Locator
