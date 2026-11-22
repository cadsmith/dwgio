/***********************************************************************************************************************
 *  @file      source/file/locator/preview.cpp
 *  @brief     The preview block of chapter 14.
 *  @copyright Copyright (c) 2026 EL JABIRI Tarik. Released under the MIT License.
 **********************************************************************************************************************/

#include "file/locator/preview.hpp"

#include "bits/reader.hpp"
#include "bits/writer.hpp"
#include "file/locator/sentinels.hpp"

#include <cstdint>
#include <format>

namespace DwgIO::File::Locator
{
namespace
{
constexpr std::size_t Lead = Sentinel + 4 + 1;

constexpr std::size_t Entry = 9;
} // namespace

auto ReadPreview(std::span<const std::byte> block, std::size_t offset, std::vector<Issue> &issues)
    -> std::vector<PreviewImage>
{
  std::vector<PreviewImage> images;
  if (block.size() < Lead + Sentinel || !Matches(block, Sentinels::Preview.Begin))
  {
    issues.push_back(Issue{.Code = IssueCode::Damaged, .Offset = offset, .What = "preview sentinel mismatch"});
    return images;
  }
  const std::size_t end = block.size() - Sentinel;
  Bits::Reader in{block.subspan(Sentinel, end - Sentinel)};
  static_cast<void>(in.RL());
  const std::uint8_t count = in.RC();
  for (std::uint8_t index = 0; index < count; ++index)
  {
    const std::size_t at = offset + Sentinel + (in.Position() / 8);
    const std::uint8_t code = in.RC();
    const std::uint64_t start = in.RL();
    const std::uint64_t size = in.RL();
    if (in.HasFailed())
    {
      break;
    }
    const bool inside = start >= offset + Lead && start - offset <= end && size <= end - (start - offset);
    if (!inside)
    {
      issues.push_back(Issue{.Code = IssueCode::Damaged,
                             .Offset = at,
                             .What = std::format("preview image {} outside the preview block", index)});
      continue;
    }
    const std::span<const std::byte> bytes = block.subspan(start - offset, size);
    images.push_back(PreviewImage{.Code = code, .Bytes = {bytes.begin(), bytes.end()}});
  }
  if (in.HasFailed())
  {
    issues.push_back(Issue{.Code = IssueCode::Truncated, .Offset = offset, .What = "preview directory cut short"});
  }
  return images;
}

auto WritePreview(std::span<const PreviewImage> images, std::size_t offset) -> std::vector<std::byte>
{
  std::size_t overall = 1 + (Entry * images.size());
  for (const PreviewImage &image : images)
  {
    overall += image.Bytes.size();
  }
  Bits::Writer out;
  out.RL(static_cast<std::uint32_t>(overall));
  out.RC(static_cast<std::uint8_t>(images.size()));
  std::size_t start = offset + Lead + (Entry * images.size());
  for (const PreviewImage &image : images)
  {
    out.RC(static_cast<std::uint8_t>(image.Code));
    out.RL(static_cast<std::uint32_t>(start));
    out.RL(static_cast<std::uint32_t>(image.Bytes.size()));
    start += image.Bytes.size();
  }
  for (const PreviewImage &image : images)
  {
    out.Raw(image.Bytes);
  }
  std::vector<std::byte> bytes;
  Put(bytes, Sentinels::Preview.Begin);
  const std::vector<std::byte> inner = out.Take();
  bytes.insert(bytes.end(), inner.begin(), inner.end());
  Put(bytes, Sentinels::Preview.End);
  return bytes;
}
} // namespace DwgIO::File::Locator
