/***********************************************************************************************************************
 *  @file      source/file/locator/section.cpp
 *  @brief     A sectioned block cut from its frame, and wrapped in one.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/section.hpp"

#include "bits/crc.hpp"
#include "bits/reader.hpp"
#include "bits/writer.hpp"

#include <format>

namespace DwgIO::File::Locator
{
namespace
{
constexpr std::size_t Lead = Sentinel + 4;
} // namespace

auto Cut(std::span<const std::byte> file, const Record &where, const Marks &marks) -> std::expected<Framed, Failure>
{
  const std::size_t at = where.Seeker;
  const std::size_t extra = marks.Wide ? 4 : 0;
  const std::size_t lead = Lead + extra;
  if (at + lead > file.size())
  {
    return std::unexpected(Failure{
        .Code = FailureCode::Damaged, .Offset = at, .What = std::format("{} section beyond the file", marks.Name)});
  }
  Framed out;
  if (!Matches(file.subspan(at), marks.Begin))
  {
    out.Issues.push_back(Issue{.Code = IssueCode::Damaged,
                               .Offset = at,
                               .What = std::format("{} section begin sentinel mismatch", marks.Name)});
  }
  Bits::Reader in{file.subspan(at + Sentinel)};
  const std::size_t size = in.RL();
  if (at + lead + size + 2 + Sentinel > file.size())
  {
    return std::unexpected(Failure{.Code = FailureCode::Damaged,
                                   .Offset = at + Sentinel,
                                   .What = std::format("{} section runs past the end", marks.Name)});
  }
  const std::span<const std::byte> body = file.subspan(at + lead, size);
  in.Seek((4 + extra + size) * 8);
  if (Bits::Crc(file.subspan(at + Sentinel, 4 + extra + size), Bits::Seed) != in.RS())
  {
    out.Issues.push_back(Issue{.Code = IssueCode::Checksum,
                               .Offset = at + lead + size,
                               .What = std::format("{} section CRC mismatch", marks.Name)});
  }
  if (!Matches(file.subspan(at + lead + size + 2), marks.End))
  {
    out.Issues.push_back(Issue{.Code = IssueCode::Damaged,
                               .Offset = at + lead + size + 2,
                               .What = std::format("{} section end sentinel mismatch", marks.Name)});
  }
  out.Body.assign(body.begin(), body.end());
  return out;
}

auto Wrap(std::span<const std::byte> body, const Marks &marks) -> std::vector<std::byte>
{
  Bits::Writer out;
  out.RL(static_cast<std::uint32_t>(body.size()));
  if (marks.Wide)
  {
    out.RL(0);
  }
  out.Raw(body);
  out.RS(Bits::Crc(out.Bytes(), Bits::Seed));
  std::vector<std::byte> bytes;
  Put(bytes, marks.Begin);
  const std::vector<std::byte> inner = out.Take();
  bytes.insert(bytes.end(), inner.begin(), inner.end());
  Put(bytes, marks.End);
  return bytes;
}
} // namespace DwgIO::File::Locator
