/***********************************************************************************************************************
 *  @file      source/file/locator/second.cpp
 *  @brief     The second header of chapter 26.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/second.hpp"

#include "bits/byte.hpp"
#include "bits/crc.hpp"
#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "file/locator/sentinels.hpp"

namespace DwgIO::File::Locator
{
namespace
{
constexpr std::size_t Code = 6;

constexpr int ConstantBits = 100;

using Bits::Byte;

constexpr std::size_t Crc = 2;

auto LengthOf(std::uint64_t value) -> std::uint8_t
{
  std::uint8_t count = 0;
  for (; value != 0; value >>= Byte)
  {
    ++count;
  }
  return count == 0 ? 1 : count;
}

/**
 *  Whether the next byte is the number of the next record, which tells the records from the handle count.
 */
auto Continues(Bits::Reader &in, std::size_t number) -> bool
{
  const std::size_t at = in.Position();
  const std::uint8_t next = in.RC();
  in.Seek(at);
  return next == number;
}
} // namespace

auto ReadSecond(std::span<const std::byte> block, std::size_t offset, std::vector<Issue> &issues)
    -> std::optional<Second>
{
  if (block.size() < (2 * Sentinel) + 4)
  {
    issues.push_back(Issue{.Code = IssueCode::Truncated, .Offset = offset, .What = "second header cut short"});
    return std::nullopt;
  }
  if (!Matches(block, Sentinels::Second.Begin))
  {
    issues.push_back(Issue{.Code = IssueCode::Damaged, .Offset = offset, .What = "second header sentinel mismatch"});
    return std::nullopt;
  }
  Bits::Reader sizer{block.subspan(Sentinel)};
  const std::size_t size = sizer.RL();
  if (size < 4 + Crc || Sentinel + size + Sentinel > block.size())
  {
    issues.push_back(
        Issue{.Code = IssueCode::Truncated, .Offset = offset + Sentinel, .What = "second header cut short"});
    return std::nullopt;
  }
  const std::span<const std::byte> body = block.subspan(Sentinel, size);
  Bits::Reader in{body};
  in.Seek(std::size_t{4} * Byte);
  Second second;
  second.Location = static_cast<std::uint32_t>(in.BL());
  for (std::size_t at = 0; at < Code; ++at)
  {
    second.Version.push_back(static_cast<char>(in.RC()));
  }
  for (int left = ConstantBits; left > 0; left -= static_cast<int>(Byte))
  {
    const int count = std::min(left, static_cast<int>(Byte));
    second.Constants.push_back(static_cast<std::byte>(in.Bits(count) << (Byte - static_cast<std::size_t>(count))));
  }
  while (!in.HasFailed() && Continues(in, second.Records.size()))
  {
    Record record;
    record.Number = in.RC();
    record.Seeker = static_cast<std::uint32_t>(in.BL());
    record.Size = static_cast<std::uint32_t>(in.BL());
    second.Records.push_back(record);
  }
  const int count = in.BS();
  for (int at = 0; at < count && !in.HasFailed(); ++at)
  {
    const std::uint8_t bytes = in.RC();
    Named named;
    named.Number = in.RC();
    std::uint64_t value = 0;
    for (std::uint8_t index = 0; index < bytes; ++index)
    {
      value = (value << Byte) | in.RC();
    }
    named.Value = Handle{value};
    second.Handles.push_back(named);
  }
  in.Pad();
  const std::size_t crcAt = in.Position() / Byte;
  const std::uint16_t crc = in.RS();
  if (in.HasFailed())
  {
    issues.push_back(
        Issue{.Code = IssueCode::Truncated, .Offset = offset + Sentinel, .What = "second header cut short"});
    return std::nullopt;
  }
  if (Bits::Crc(body.first(crcAt), Bits::Seed) != crc)
  {
    issues.push_back(
        Issue{.Code = IssueCode::Checksum, .Offset = offset + Sentinel + crcAt, .What = "second header CRC mismatch"});
  }
  in.Pad();
  const std::span<const std::byte> junk = body.subspan(in.Position() / Byte);
  second.Junk.assign(junk.begin(), junk.end());
  if (!Matches(block.subspan(Sentinel + size), Sentinels::Second.End))
  {
    issues.push_back(Issue{
        .Code = IssueCode::Damaged, .Offset = offset + Sentinel + size, .What = "second header end sentinel mismatch"});
  }
  return second;
}

auto WriteSecond(const Second &second) -> std::vector<std::byte>
{
  Bits::Writer inner;
  inner.BL(static_cast<std::int32_t>(second.Location));
  for (std::size_t at = 0; at < Code; ++at)
  {
    inner.RC(at < second.Version.size() ? static_cast<std::uint8_t>(second.Version[at]) : 0);
  }
  for (int bit = 0; bit < ConstantBits; ++bit)
  {
    const auto index = static_cast<std::size_t>(bit) / Byte;
    const auto mask = static_cast<std::byte>(0x80U >> (static_cast<std::size_t>(bit) % Byte));
    inner.B(index < second.Constants.size() && (second.Constants[index] & mask) != std::byte{0});
  }
  for (const Record &record : second.Records)
  {
    inner.RC(record.Number);
    inner.BL(static_cast<std::int32_t>(record.Seeker));
    inner.BL(static_cast<std::int32_t>(record.Size));
  }
  inner.BS(static_cast<int>(second.Handles.size()));
  for (const Named &named : second.Handles)
  {
    const std::uint8_t bytes = LengthOf(named.Value.Value);
    inner.RC(bytes);
    inner.RC(named.Number);
    for (std::uint8_t index = bytes; index > 0; --index)
    {
      inner.RC(static_cast<std::uint8_t>(named.Value.Value >> (Byte * (index - 1))));
    }
  }
  inner.Fill();
  const std::vector<std::byte> stream = inner.Take();
  Bits::Writer out;
  out.RL(static_cast<std::uint32_t>(4 + stream.size() + Crc + second.Junk.size()));
  out.Raw(stream);
  out.RS(Bits::Crc(out.Bytes(), Bits::Seed));
  out.Raw(second.Junk);
  std::vector<std::byte> bytes;
  Put(bytes, Sentinels::Second.Begin);
  const std::vector<std::byte> body = out.Take();
  bytes.insert(bytes.end(), body.begin(), body.end());
  Put(bytes, Sentinels::Second.End);
  return bytes;
}
} // namespace DwgIO::File::Locator
